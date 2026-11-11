#include <Arduino.h>
#include <BenchProtocol.h>
#include <BenchAddress.h>
#include <BenchRadio.h>
#include <RoutingStore.h>
#include <PenFlow.h>
#include <NodeHistory.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>

// Treat an unconfirmed sleep command as potentially applied until a WAKE ACK.
static bool asleep[bench::MAX_NODES+1];
static uint32_t wakeUntil=0;
static bool ready = false, pending = false, transmitting = false;
static bench::Confirmations confirmations;
static bench::Confirmations commanded;
static routing::Record routingRecord;
static routing::Store<decltype(api.system.flash)> routingStore(api.system.flash);
static bool storageOk=false;
static bool maintenanceEnabled=true;
static uint32_t maintenanceAt=0;
// Recovery and new-auction work share the existing single radio transaction.
enum Task { NO_TASK, RECOVERY, NEW_AUCTION, FLOW, STARTUP };
static startup::Assessment startupAssessment;
static startup::Decision startupDecision=startup::COLLECTING;
static uint32_t startupUntil=0;
static bool startupBlocked(){return startupDecision!=startup::RESUME && startupDecision!=startup::NEW_AUCTION;}
enum Stage { NEED_QUERY, NEED_SET, NEED_REBOOT, WAIT_REBOOT, VERIFY_QUERY, VERIFY_SET, WAIT_BOOT };
static Task task=NO_TASK;
static Stage stage=NEED_QUERY, afterBoot=NEED_QUERY;
static uint8_t taskNode=1;
static uint32_t taskAt=0, bootDeadline=0, generatedToken=0;
static bool taskInFlight=false, rebootTried=false;
static uint8_t taskFaults=0;
static bool persist() {
    storageOk=routingStore.save(routingRecord);
    if(!storageOk)Serial.println("MS_STORAGE=FAILED;ACTION=BLOCK_WRITES");
    return storageOk;
}
static bool busy(){return pending || task!=NO_TASK;}
static bool scanning = false;
static uint8_t scanOk = 0, scanFailed = 0;
static void cached(uint8_t node) {
    const auto &c = confirmations.get(node);
    const auto &d = commanded.get(node);
    Serial.printf("MS_CACHE_FAULT_NODE=%u;FAULT=%u\r\n",node,routingRecord.faults[node]);
    Serial.printf("MS_CACHE_NODE=%u;COMMANDED=%s;CONFIRMED=%s;KNOWN=%d;AGE_MS=%lu;SOURCE=CACHE\r\n",
        node, d.known ? bench::name(d.state) : "UNKNOWN",
        c.known ? bench::name(c.state) : "UNKNOWN", c.known,
        c.known ? (unsigned long)(millis()-c.at) : 0UL);
}
static bool dropArmed = false, dropThisTransaction = false;
static uint16_t attempts = 0;
static uint32_t deadline = 0;
static const char *result = "IDLE";
static bench::Frame request = {bench::SET, 1, bench::OFF, bench::APPLIED, 0};

static void report() {
    Serial.printf("MS_STARTUP=%u;RESPONDED=%u;SURVIVORS=%u\r\n",startupDecision,startupAssessment.replies,startupAssessment.survivors);
    const auto &confirmation = confirmations.get(request.node);
    benchRadio::printConfig("CENTRAL", ready, "0.7.1");
    Serial.printf("MS_LINK=%s;TOKEN=%lu;COMMANDED=%s;CONFIRMED=%s;ATTEMPTS=%u;AGE_MS=%lu\r\n",
        result, (unsigned long)request.token, request.type == bench::QUERY ? "UNCHANGED" : bench::name(request.state),
        confirmation.known ? bench::name(confirmation.state) : "UNKNOWN", attempts,
        confirmation.known ? (unsigned long)(millis() - confirmation.at) : 0UL);
    Serial.printf("MS_TARGET_NODE=%u;MAX_NODES=50\r\n", request.node);
    Serial.printf("MS_DROP_ARMED=%d;TRANSACTION_DROP_PENDING=%d\r\n", dropArmed, dropThisTransaction);
    Serial.printf("MS_STORAGE=%s;AUCTION=%lu;RECOVERY=%u;ACTIVE=%u;LAST=%u\r\n",
        storageOk?"OK":"FAILED",(unsigned long)routingRecord.auction,task,routingRecord.active,routingRecord.last);
}
int dropCommand(SERIAL_PORT, char *, stParam *p) {
    if (p->argc == 1 && !strcmp(p->argv[0], "?")) { report(); return AT_OK; }
    if (busy()) return AT_ERROR;
    if (p->argc != 1 || (strcmp(p->argv[0], "0") && strcmp(p->argv[0], "1")))
        return AT_PARAM_ERROR;
    dropArmed = !strcmp(p->argv[0], "1");
    report(); return AT_OK;
}
int penCommand(SERIAL_PORT, char *, stParam *p) {
    if (p->argc == 1 && !strcmp(p->argv[0], "?")) { report(); return AT_OK; }
    if (!ready || busy()) return AT_ERROR;
    const char *arguments[3] = {nullptr, nullptr, nullptr};
    if (p->argc != 2 && p->argc != 3) return AT_PARAM_ERROR;
    for (int i = 0; i < p->argc; ++i) arguments[i] = p->argv[i];
    bench::Frame parsed;
    if (!bench::parseSet(p->argc, arguments, parsed)) return AT_PARAM_ERROR;
    if(!storageOk || startupBlocked() || routingRecord.flowPending)return AT_ERROR;
    if (asleep[parsed.node]) return AT_ERROR;
    routing::Record before=routingRecord;
    routingRecord.configured[parsed.node]=1;
    routingRecord.desired[parsed.node]=parsed.state;
    if(parsed.state==bench::ACTIVE || parsed.state==bench::HOLD_OPEN)routingRecord.active=parsed.node;
    if(parsed.state==bench::LAST)routingRecord.last=parsed.node;
    if(parsed.state==bench::OFF){
        if(routingRecord.active==parsed.node)routingRecord.active=0;
        if(routingRecord.last==parsed.node)routingRecord.last=0;
    }
    if(!persist()){routingRecord=before;return AT_ERROR;}
    request = parsed;request.auction=routingRecord.auction;
    auto &desired = commanded.nodes[request.node];
    desired.known = true; desired.state = request.state; desired.at = millis();
    pending = true; transmitting = false; attempts = 0;
    dropThisTransaction = dropArmed;
    dropArmed = false;
    deadline = millis(); result = "PENDING";
    report(); return AT_OK;
}
static void startPoll(const bench::Frame &frame) {
    request = frame;
    pending = true; transmitting = false; attempts = 0;
    dropThisTransaction = false;
    deadline = millis(); result = "PENDING";
}
int powerCommand(SERIAL_PORT, char *, stParam *p) {
    uint32_t token, node;
    if (p->argc != 3 || !bench::decimal(p->argv[0],UINT32_MAX,token) || !token ||
        !bench::decimal(p->argv[1],bench::MAX_NODES,node) || !bench::validNode(node))
        return AT_PARAM_ERROR;
    bool enable = !strcmp(p->argv[2],"SLEEP");
    if (!enable && strcmp(p->argv[2],"WAKE")) return AT_PARAM_ERROR;
    if (!ready || pending || scanning || (task!=NO_TASK && task!=STARTUP) ||
        !storageOk || !routingRecord.configured[node]) return AT_ERROR;
    if (enable && (startupBlocked() || routingRecord.flowPending ||
        routingRecord.desired[node]!=bench::OFF)) return AT_ERROR;
    bench::Frame frame={uint8_t(enable?bench::SLEEP:bench::WAKE),uint8_t(node),
        bench::OFF,bench::APPLIED,token};
    frame.auction=routingRecord.auction;
    if (enable) asleep[node]=true;
    wakeUntil=millis()+bench::WAKE_LIMIT_MS;
    startPoll(frame);
    report();
    return AT_OK;
}
int pollCommand(SERIAL_PORT, char *, stParam *p) {
    const char *args[2] = {nullptr, nullptr};
    if (p->argc != 2) return AT_PARAM_ERROR;
    args[0] = p->argv[0]; args[1] = p->argv[1];
    bench::Frame frame;
    if (!bench::parsePoll(2, args, frame)) return AT_PARAM_ERROR;
    if (!ready || busy()) return AT_ERROR;
    startPoll(frame); report(); return AT_OK;
}
int pollLastCommand(SERIAL_PORT, char *, stParam *p) {
    uint32_t node;
    if (p->argc != 1 || !bench::decimal(p->argv[0], bench::MAX_NODES, node) ||
        !bench::validNode(node)) return AT_PARAM_ERROR;
    cached(uint8_t(node)); return AT_OK;
}
int pollAllLastCommand(SERIAL_PORT, char *, stParam *p) {
    if (p->argc != 1 || strcmp(p->argv[0], "?")) return AT_PARAM_ERROR;
    for (uint8_t node = 1; node <= bench::MAX_NODES; ++node) cached(node);
    Serial.println("MS_CACHE_ALL=DONE;COUNT=50");
    return AT_OK;
}
int pollAllCommand(SERIAL_PORT, char *, stParam *p) {
    uint32_t token;
    if (p->argc != 1 || !bench::decimal(p->argv[0], UINT32_MAX, token) || !token)
        return AT_PARAM_ERROR;
    if (!ready || busy()) return AT_ERROR;
    scanning = true; scanOk = scanFailed = 0;
    startPoll({bench::QUERY, 1, bench::OFF, bench::APPLIED, token});
    Serial.println("MS_POLL_ALL=START;COUNT=50");
    return AT_OK;
}
static void finishPoll(bool success,bool booting=false) {
    Serial.printf("MS_POLL_NODE=%u;TOKEN=%lu;RESULT=%s;STATE=%s;ATTEMPTS=%u;SOURCE=LIVE\r\n",
        request.node, (unsigned long)request.token, success ? "OK" : (booting ? "BOOTING" : "TIMEOUT"),
        success ? bench::name(confirmations.get(request.node).state) : "UNKNOWN", attempts);
    if (!scanning) return;
    if (success) ++scanOk; else ++scanFailed;
    if (request.node == bench::MAX_NODES) {
        scanning = false;
        Serial.printf("MS_POLL_ALL=DONE;OK=%u;FAILED=%u;COUNT=50\r\n", scanOk, scanFailed);
    } else {
        uint32_t token = request.token + 1;
        if (!token) token = 1;
        startPoll({bench::QUERY, uint8_t(request.node + 1), bench::OFF, bench::APPLIED, token});
    }
}

static uint32_t nextToken(){if(!++generatedToken)++generatedToken;return generatedToken;}
static void flowReport(){
    Serial.printf("MS_FLOW=%s;ACTIVE=%u;LAST=%u;STEP=%u;AUCTION=%lu\r\n",
        routingRecord.flowPending?(routingRecord.flowFailed?"PAUSED":"PENDING"):"IDLE",
        routingRecord.active,routingRecord.last,routingRecord.flowStep,(unsigned long)routingRecord.auction);
}
static void finishFlowStep(bool ok){
    taskInFlight=false;
    if(!ok){
        routingRecord.flowFailed=true;routingRecord.faults[taskNode]=1;
        persist();task=NO_TASK;
        Serial.printf("MS_FLOW=PAUSED;NODE=%u;REASON=UNCONFIRMED;STEP=%u\r\n",taskNode,routingRecord.flowStep);
        return;
    }
    uint8_t nodes[3];const uint8_t count=routing::flowNodes(routingRecord,nodes);
    ++routingRecord.flowStep;
    if(routingRecord.flowStep>=count)routingRecord.flowPending=false;
    if(!persist()){task=NO_TASK;Serial.println("MS_FLOW=PAUSED;REASON=STORAGE");return;}
    if(!routingRecord.flowPending){
        Serial.printf("MS_FLOW=DONE;ACTIVE=%u;LAST=%u\r\n",routingRecord.active,routingRecord.last);
        task=NO_TASK;maintenanceAt=millis()+10000;
    } else {stage=NEED_QUERY;taskAt=millis();bootDeadline=0;}
}
static void finishNode(bool ok){
    if(task==FLOW){finishFlowStep(ok);return;}

    if(!ok){
        ++taskFaults;
        if(!routingRecord.faults[taskNode]){routingRecord.faults[taskNode]=1;persist();}
        Serial.printf("MS_CONTROLLER_FAULT_NODE=%u;CAUSE=UNCONFIRMED;AUCTION=%lu\r\n",
            taskNode,(unsigned long)routingRecord.auction);
    }
    Serial.printf("MS_RECONCILE_NODE=%u;RESULT=%s\r\n",taskNode,ok?"CONFIRMED":"UNCONFIRMED");
    ++taskNode;stage=task==NEW_AUCTION?NEED_SET:NEED_QUERY;taskAt=millis();rebootTried=false;bootDeadline=0;
    taskInFlight=false;
}
static void taskResult(bool ok,const bench::Frame*reply){
    taskInFlight=false;
    if(task==STARTUP){
        if(ok && reply && reply->result==bench::BOOTING && int32_t(millis()-startupUntil)<0){taskAt=millis()+1000;return;}
        if(ok && reply && reply->result!=bench::BOOTING)startupAssessment.observe(*reply,routingRecord.auction,routingRecord.desired[taskNode]);
        else if(ok && reply)startupAssessment.conflict=true;
        Serial.printf("MS_STARTUP_NODE=%u;RESULT=%s;BOOT_ID=%lu;UPTIME_MS=%lu;AUCTION=%lu;LAST_TOKEN=%lu;STATE=%s;FLAGS=%u\r\n",
            taskNode,ok?"REPLY":"TIMEOUT",reply?(unsigned long)reply->bootId:0UL,reply?(unsigned long)reply->uptime:0UL,
            reply?(unsigned long)reply->auction:0UL,reply?(unsigned long)reply->lastToken:0UL,reply?bench::name(reply->state):"UNKNOWN",reply?reply->flags:0);
        ++taskNode;taskAt=millis();return;
    }
    if(request.type==bench::REBOOT){
        // A reboot ACK means queued, never proof that restart or OFF happened.
        Serial.printf("MS_REBOOT_NODE=%u;QUEUED_ACK=%d\r\n",taskNode,ok);
        stage=WAIT_REBOOT;taskAt=millis()+20000;return;
    }
    if(request.type==bench::QUERY){
        if(ok && reply && reply->result==bench::BOOTING){
            if(!bootDeadline)bootDeadline=millis()+25000;
            if(int32_t(millis()-bootDeadline)>=0){finishNode(false);return;}
            afterBoot=stage;stage=WAIT_BOOT;taskAt=millis()+1000;return;
        }
        if(ok && reply && reply->result==bench::APPLIED && (reply->flags&bench::APPLIED_THIS_BOOT) && reply->auction==routingRecord.auction && confirmations.get(taskNode).state==routingRecord.desired[taskNode]){finishNode(true);return;}
        if(stage==VERIFY_QUERY && !ok){finishNode(false);return;}
        stage=stage==VERIFY_QUERY?VERIFY_SET:NEED_SET;taskAt=millis();return;
    }
    if(ok){finishNode(true);return;}
    if(task==NEW_AUCTION && !rebootTried){stage=NEED_REBOOT;taskAt=millis();return;}
    finishNode(false);
}
static bool beginNewAuction();
static void maintain(){
    if(!maintenanceEnabled || !storageOk || pending || scanning)return;
    if(task==NO_TASK){
        if(startupBlocked())return;
        if(routingRecord.flowPending){
            if(routingRecord.flowFailed)return;
            task=FLOW;stage=NEED_QUERY;taskAt=millis();bootDeadline=0;
        } else {
            if(int32_t(millis()-maintenanceAt)<0)return;
            task=routingRecord.pendingReset?NEW_AUCTION:RECOVERY;taskNode=1;stage=NEED_QUERY;taskAt=millis();taskFaults=0;bootDeadline=0;rebootTried=false;
        }
    }
    if(int32_t(millis()-taskAt)<0)return;
    if(task==FLOW){
        uint8_t nodes[3];uint8_t count=routing::flowNodes(routingRecord,nodes);
        if(routingRecord.flowStep>=count){routingRecord.flowPending=false;persist();task=NO_TASK;return;}
        taskNode=nodes[routingRecord.flowStep];
    } else while(taskNode<=bench::MAX_NODES && (!routingRecord.configured[taskNode] || asleep[taskNode]))++taskNode;
    if(task==STARTUP && taskNode>bench::MAX_NODES){
        task=NO_TASK;startupDecision=startupAssessment.decision();
        // An interrupted, durably recorded clear must finish without incrementing auction again.
        if(routingRecord.pendingReset && startupAssessment.replies && !startupAssessment.storageFault)startupDecision=startup::RESUME;
        Serial.printf("MS_STARTUP_DECISION=%s;RESPONDED=%u;SURVIVORS=%u\r\n",startupDecision==startup::RESUME?"RESUME":startupDecision==startup::NEW_AUCTION?"NEW_AUCTION":startupDecision==startup::CONFLICT?"CONFLICT":"NO_RESPONSE",startupAssessment.replies,startupAssessment.survivors);
        if(startupDecision==startup::NEW_AUCTION){if(!beginNewAuction())startupDecision=startup::CONFLICT;}
        else if(startupBlocked()){
            for(uint8_t n=1;n<=bench::MAX_NODES;++n)if(routingRecord.configured[n])routingRecord.faults[n]=1;
            persist();Serial.println("MS_STARTUP=BLOCKED;ACTION=CHECK_RECORDS_OR_EXPLICIT_AUCTION_CLEAR");
        }
        return;
    }
    if(task!=FLOW && taskNode>bench::MAX_NODES){
        Serial.printf("MS_RECONCILE_DONE=%s;FAULTS=%u;AUCTION=%lu\r\n",
            task==NEW_AUCTION?"NEW_AUCTION":"RECOVERY",taskFaults,(unsigned long)routingRecord.auction);
        if(task==NEW_AUCTION){routingRecord.pendingReset=false;persist();}
        task=NO_TASK;maintenanceAt=millis()+10000;return;
    }
    if(stage==WAIT_REBOOT){stage=VERIFY_QUERY;bootDeadline=millis()+25000;}
    if(stage==WAIT_BOOT)stage=afterBoot;
    bench::Frame f={bench::QUERY,taskNode,bench::OFF,bench::APPLIED,nextToken()};
    if(stage==NEED_REBOOT){f.type=bench::REBOOT;rebootTried=true;}
    else if(stage==NEED_SET || stage==VERIFY_SET){f.type=bench::SET;f.state=routingRecord.desired[taskNode];}
    f.auction=routingRecord.auction;startPoll(f);taskInFlight=true;
}
static bool beginNewAuction(){
    const uint32_t base=routingRecord.auction>startupAssessment.maxAuction?routingRecord.auction:startupAssessment.maxAuction;
    if(base==UINT32_MAX){Serial.println("MS_AUCTION=EXHAUSTED");return false;}
    routing::Record before=routingRecord;
    routingRecord.pendingReset=true;
    routingRecord.flowPending=routingRecord.flowFailed=false;
    routingRecord.flowOldActive=routingRecord.flowOldLast=routingRecord.flowStep=0;
    routingRecord.auction=base+1;routingRecord.active=routingRecord.last=0;
    memset(routingRecord.desired,0,sizeof routingRecord.desired);
    memset(routingRecord.faults,0,sizeof routingRecord.faults);
    if(!persist()){routingRecord=before;return false;}
    for(uint8_t n=1;n<=bench::MAX_NODES;++n){
        commanded.nodes[n].known=routingRecord.configured[n];commanded.nodes[n].state=bench::OFF;
        confirmations.nodes[n].known=false;
    }
    task=NEW_AUCTION;taskNode=1;stage=NEED_SET;taskAt=millis();taskFaults=0;rebootTried=false;bootDeadline=0;
    Serial.printf("MS_NEW_AUCTION=SAVED;AUCTION=%lu\r\n",(unsigned long)routingRecord.auction);
    startupDecision=startup::NEW_AUCTION;return true;
}
int auctionCommand(SERIAL_PORT,char*,stParam*p){
    if(p->argc==1 && !strcmp(p->argv[0],"?")){report();return AT_OK;}
    if(p->argc!=1 || strcmp(p->argv[0],"CLEAR"))return AT_PARAM_ERROR;
    if(!ready || busy() || !storageOk)return AT_ERROR;
    return beginNewAuction()?AT_OK:AT_ERROR;
}
int faultsCommand(SERIAL_PORT,char*,stParam*p){
    if(p->argc!=1 || strcmp(p->argv[0],"?"))return AT_PARAM_ERROR;
    for(uint8_t n=1;n<=bench::MAX_NODES;++n)if(routingRecord.configured[n])
        Serial.printf("MS_FAULT_NODE=%u;FAULT=%u;DESIRED=%s;SOURCE=CONTROLLER\r\n",
            n,routingRecord.faults[n],bench::name(routingRecord.desired[n]));
    return AT_OK;
}
int rosterCommand(SERIAL_PORT,char*,stParam*p){
    if(p->argc==1 && !strcmp(p->argv[0],"?")){
        for(uint8_t n=1;n<=bench::MAX_NODES;++n)if(routingRecord.configured[n])Serial.printf("MS_ROSTER_NODE=%u\r\n",n);
        return AT_OK;
    }
    // Explicit one-node add/remove; no inferred discovery or deletion of intent.
    uint32_t node,value;
    if(p->argc!=2 || !bench::decimal(p->argv[0],50,node) || !bench::validNode(node) ||
        !bench::decimal(p->argv[1],1,value))return AT_PARAM_ERROR;
    if(busy() || startupBlocked() || !storageOk || routingRecord.flowPending || asleep[node])return AT_ERROR;
    routing::Record before=routingRecord;routingRecord.configured[node]=value;
    if(!persist()){routingRecord=before;return AT_ERROR;}return AT_OK;
}


int destinationCommand(SERIAL_PORT,char*,stParam*p){
    uint32_t destination;
    if(p->argc==1 && !strcmp(p->argv[0],"?")){flowReport();return AT_OK;}
    if(p->argc!=1 || !bench::decimal(p->argv[0],50,destination) || !bench::validNode(destination))return AT_PARAM_ERROR;
    if(!ready || busy() || startupBlocked() || !storageOk || routingRecord.flowPending)return AT_ERROR;
    if (asleep[destination]) return AT_ERROR;
    routing::Record before=routingRecord,next;
    if(!routing::plan(before,uint8_t(destination),next)){
        Serial.println("MS_FLOW=REJECTED;REASON=UNSUPPORTED_DESTINATION_OR_BASELINE");return AT_PARAM_ERROR;
    }
    routingRecord=next;
    if(!persist()){routingRecord=before;return AT_ERROR;}
    for(uint8_t n=1;n<=bench::MAX_NODES;++n){commanded.nodes[n].known=routingRecord.configured[n];commanded.nodes[n].state=routingRecord.desired[n];}
    task=FLOW;stage=NEED_QUERY;taskAt=millis();bootDeadline=0;taskInFlight=false;
    flowReport();return AT_OK;
}
int flowCommand(SERIAL_PORT,char*,stParam*p){
    if(p->argc==1 && !strcmp(p->argv[0],"?")){flowReport();return AT_OK;}
    if(p->argc!=1 || strcmp(p->argv[0],"RETRY"))return AT_PARAM_ERROR;
    if(!ready || busy() || startupBlocked() || !storageOk || !routingRecord.flowPending || !routingRecord.flowFailed)return AT_ERROR;
    routingRecord.flowFailed=false;
    if(!persist()){routingRecord.flowFailed=true;return AT_ERROR;}
    task=FLOW;stage=NEED_QUERY;taskAt=millis();bootDeadline=0;taskInFlight=false;
    flowReport();return AT_OK;
}

void setup() {
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000);
    api.system.lpm.set(0);
    api.system.firmwareVersion.set("MS-Central-RF-0.7.1");
    bool commandReady = api.system.atMode.add((char *)"PEN",
        (char *)"Bench token:node(1-50):OFF|ACTIVE|LAST|HOLD_OPEN|FULL|FAULT",
        (char *)"PEN", penCommand, RAK_ATCMD_PERM_READ | RAK_ATCMD_PERM_WRITE);
    bool testReady = api.system.atMode.add((char *)"DROP",
        (char *)"Bench only: 1 ignores next transaction's first successful ACK; 0 disables",
        (char *)"DROP", dropCommand, RAK_ATCMD_PERM_READ | RAK_ATCMD_PERM_WRITE);
    bool pollReady = api.system.atMode.add((char *)"POLL", (char *)"Live read-only token:node",
        (char *)"POLL", pollCommand, RAK_ATCMD_PERM_WRITE);
    pollReady = api.system.atMode.add((char *)"POLL_LAST", (char *)"Cached state by node",
        (char *)"POLL_LAST", pollLastCommand, RAK_ATCMD_PERM_WRITE) && pollReady;
    pollReady = api.system.atMode.add((char *)"POLL_ALL", (char *)"Live read-only all 50; initial token",
        (char *)"POLL_ALL", pollAllCommand, RAK_ATCMD_PERM_WRITE) && pollReady;
    pollReady = api.system.atMode.add((char *)"POLL_ALL_LAST", (char *)"Cached states, ?",
        (char *)"POLL_ALL_LAST", pollAllLastCommand, RAK_ATCMD_PERM_READ) && pollReady;
    bool recoveryReady=api.system.atMode.add((char*)"AUCTION",(char*)"CLEAR: durable new auction; ? status",(char*)"AUCTION",auctionCommand,RAK_ATCMD_PERM_READ|RAK_ATCMD_PERM_WRITE);
    recoveryReady=api.system.atMode.add((char*)"FAULTS",(char*)"? controller fault records",(char*)"FAULTS",faultsCommand,RAK_ATCMD_PERM_READ)&&recoveryReady;
    recoveryReady=api.system.atMode.add((char*)"ROSTER",(char*)"? or node:1 add / node:0 exclude",(char*)"ROSTER",rosterCommand,RAK_ATCMD_PERM_READ|RAK_ATCMD_PERM_WRITE)&&recoveryReady;
    recoveryReady=api.system.atMode.add((char*)"DEST",(char*)"Bench new-head destination 1..50; ? status",(char*)"DEST",destinationCommand,RAK_ATCMD_PERM_READ|RAK_ATCMD_PERM_WRITE)&&recoveryReady;
    recoveryReady=api.system.atMode.add((char*)"FLOW",(char*)"? status; RETRY paused flow",(char*)"FLOW",flowCommand,RAK_ATCMD_PERM_READ|RAK_ATCMD_PERM_WRITE)&&recoveryReady;
    recoveryReady=api.system.atMode.add((char*)"POWER",(char*)"token:node:SLEEP or token:node:WAKE",
        (char*)"POWER",powerCommand,RAK_ATCMD_PERM_WRITE)&&recoveryReady;
    storageOk=routingStore.load(routingRecord);
    generatedToken=routingRecord.sequence*1000UL+millis();
    for(uint8_t n=1;n<=bench::MAX_NODES;++n){commanded.nodes[n].known=routingRecord.configured[n];commanded.nodes[n].state=routingRecord.desired[n];}
    maintenanceAt=millis()+1000;
    startupAssessment=startup::Assessment();startupDecision=startup::COLLECTING;
    task=STARTUP;taskNode=1;stage=NEED_QUERY;taskAt=millis()+16000;startupUntil=millis()+60000;
    taskInFlight=false;pending=transmitting=scanning=false;
    ready = commandReady && testReady && pollReady && recoveryReady && benchRadio::begin();
    report();
}
void loop() {
    if (!ready) { delay(5); return; }
    bench::Frame ack;
    if (benchRadio::take(ack) && pending && (bench::matchesAck(ack, request) || bench::matchesStatus(ack, request) || bench::matchesReboot(ack,request) || bench::matchesPower(ack,request))) {
        if (bench::powerRequest(request.type)) {
            if (request.type == bench::WAKE && int32_t(millis()-wakeUntil)>=0) return;
            const bool sleepFlag = (ack.flags & bench::SLEEP_ENABLED) != 0;
            const bool ok = ack.result == bench::APPLIED && sleepFlag == (request.type == bench::SLEEP);
            asleep[request.node] = sleepFlag;
            pending = transmitting = false;
            result = ok ? "ACK" : "REJECTED";
            Serial.printf("MS_POWER_NODE=%u;TOKEN=%lu;RESULT=%s;MODE=%s\r\n",
                request.node,(unsigned long)request.token,result,sleepFlag?"SLEEP":"AWAKE");
            report();
            return;
        }
        const auto &confirmation = confirmations.get(request.node);
        if (request.type == bench::SET && dropThisTransaction && ack.state == request.state && ack.result <= bench::DUPLICATE) {
            dropThisTransaction = false;
            Serial.printf("MS_TEST_ACK_IGNORED=1;TOKEN=%lu;RESULT=%u;CONFIRMED=%s\r\n",
                (unsigned long)ack.token, ack.result,
                confirmation.known ? bench::name(confirmation.state) : "UNKNOWN");
            // Leave transaction, deadline and last confirmation untouched.
            return;
        }
        dropThisTransaction = false;
        if(ack.type!=bench::STATUS || ack.result!=bench::BOOTING)confirmations.remember(ack, millis());
        result = (request.type == bench::QUERY || request.type == bench::REBOOT || (ack.state == request.state && ack.result <= bench::DUPLICATE)) ? "ACK" : "REJECTED";
        pending = false; transmitting = false;
        Serial.printf("MS_ACK_RESULT=%u;RSSI=%d;SNR=%d\r\n", ack.result,
            benchRadio::lastRssi, benchRadio::lastSnr);
        report();
        if(taskInFlight){taskResult(!strcmp(result,"ACK"),&ack);return;}
        if (request.type == bench::QUERY) { finishPoll(ack.result!=bench::BOOTING,ack.result==bench::BOOTING); return; }
    }
    if (transmitting && benchRadio::sent) {
        transmitting = false;
        if (request.type != bench::WAKE) deadline = millis() + 1000;
    }
    if (pending && int32_t(millis() - deadline) >= 0) {
        if (request.type == bench::WAKE ? int32_t(millis()-wakeUntil)>=0 : attempts>=3) {
            pending = false; transmitting = false; dropThisTransaction = false;
            result = "FAILED"; report();
            if (bench::powerRequest(request.type))
                Serial.printf("MS_POWER_NODE=%u;TOKEN=%lu;RESULT=FAILED;MODE=UNKNOWN\r\n",request.node,(unsigned long)request.token);
            if(taskInFlight){taskResult(false,nullptr);return;}
            if (request.type == bench::QUERY) { finishPoll(false); return; }
        } else {
            ++attempts;
            transmitting = benchRadio::send(request);
            // Bound a missing TX callback as well as missing acknowledgements.
            deadline = millis() + (request.type == bench::WAKE ? bench::WAKE_RETRY_MS :
                (transmitting ? 2000 : 1000));
            Serial.printf("MS_TX_ATTEMPT=%u;TOKEN=%lu;QUEUED=%d\r\n",
                attempts, (unsigned long)request.token, transmitting);
        }
    }
    maintain();
    delay(5);
}
