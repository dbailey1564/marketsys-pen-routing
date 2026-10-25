#pragma once
#include "BenchProtocol.h"
#include <string.h>

namespace bench {
static const uint8_t MAX_NODES = 50;
inline bool validNode(unsigned value) { return value >= 1 && value <= MAX_NODES; }
inline bool decimal(const char *text, uint32_t limit, uint32_t &value) {
    if (!text || !*text) return false;
    value = 0;
    for (; *text; ++text) {
        if (*text < '0' || *text > '9') return false;
        uint32_t digit = uint32_t(*text - '0');
        if (value > limit / 10 || (value == limit / 10 && digit > limit % 10)) return false;
        value = value * 10 + digit;
    }
    return true;
}
// Legacy token:state targets node 1; addressed form is token:node:state.
inline bool parseSet(int argc, const char *const *argv, Frame &frame) {
    if (argc != 2 && argc != 3) return false;
    uint32_t token, node = 1;
    if (!decimal(argv[0], UINT32_MAX, token) || !token) return false;
    if (argc == 3 && (!decimal(argv[1], MAX_NODES, node) || !validNode(node))) return false;
    const char *state = argv[argc - 1];
    if (!state) return false;
    for (uint8_t s = OFF; s <= FAULT; ++s) {
        if (validState(s) && !strcmp(state, name(s))) {
            frame = {SET, uint8_t(node), s, APPLIED, token};
            return true;
        }
    }
    return false;
}
inline bool forNode(const Frame &frame, uint8_t node) {
    return validNode(node) && frame.type == SET && frame.node == node;
}
inline bool matchesAck(const Frame &ack, const Frame &request) {
    return ack.type == ACK && ack.node == request.node && ack.token == request.token;
}
struct Confirmation { bool known = false; uint8_t state = OFF; uint32_t at = 0; };
struct Confirmations {
    Confirmation nodes[MAX_NODES + 1];
    const Confirmation &get(uint8_t node) const { return nodes[validNode(node) ? node : 0]; }
    void remember(const Frame &ack, uint32_t at) {
        if (validNode(ack.node) && ack.type == ACK) {
            nodes[ack.node].known = true; nodes[ack.node].state = ack.state; nodes[ack.node].at = at;
        }
    }
};
}
