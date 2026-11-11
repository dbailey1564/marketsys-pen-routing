#include <cassert>
#include <iostream>
#include "../firmware/radio_central/radio_central.ino"

int main() {
    const uint8_t golden[] = {0xaa,0x55,2,0x10,0x78,0x56,0x34,0x12,3,0,0,0,0x14,0,0x35,0x0b};
    host::Receiver receiver;
    host::Destination event{};
    assert(receiver.feed(0xaa,event)==host::Receiver::MORE);
    for(size_t i=0;i<sizeof golden;++i)
        assert(receiver.feed(golden[i],event)==(i+1==sizeof golden?host::Receiver::VALID:host::Receiver::MORE));
    assert(event.session==0x12345678 && event.transaction==3 && event.pen==20 && event.alpha==0);
    for(size_t i=0;i<sizeof golden;++i)
        assert(receiver.feed(golden[i]^(i==12?1:0),event)==(i+1==sizeof golden?host::Receiver::INVALID:host::Receiver::MORE));

    setup();
    startupDecision=startup::RESUME;
    task=NO_TASK;
    maintenanceEnabled=false;
    char session[]="305419896";
    stParam p{};
    p.argc=1;p.argv[0]=session;
    assert(hostCommand(0,nullptr,&p)==AT_OK);
    assert(routingRecord.hostSession==event.session);
    event.pen=1;
    handleHost(event);
    assert(routingRecord.hostTransaction==3 && routingRecord.active==1 &&
           routingRecord.desired[1]==bench::ACTIVE && routingRecord.flowPending);
    const auto seq=routingRecord.sequence;
    handleHost(event);
    assert(routingRecord.sequence==seq && Serial.out.find("MS_HOST=DUPLICATE")!=std::string::npos);
    event.pen=2;
    handleHost(event);
    assert(routingRecord.sequence==seq && Serial.out.find("MS_HOST=CONFLICT")!=std::string::npos);
    routing::Store<FlashMock> restarted(api.system.flash);
    routing::Record saved;
    assert(restarted.load(saved) && saved.hostTransaction==3 && saved.hostPen==1 && saved.flowPending);
    taskNode=1;
    finishFlowStep(true);
    event.pen=1;
    ++event.transaction;
    handleHost(event);
    assert(routingRecord.hostTransaction==4 && routingRecord.desired[1]==bench::HOLD_OPEN);
    std::cout << "PASS: golden frame, CRC rejection, durable duplicate protection and repeated destination\n";
}
