#ifndef OBC_SWITCH_HPP
#define OBC_SWITCH_HPP

#include "opamp.hpp"
#include "io-port.hpp"
#include <cstdint>

struct Null {};
struct Ground {};

struct InSwitch {
public:
    InSwitch(Null) : b{0x0} {}
    InSwitch(Ground) : b{0x1} {}
    InSwitch(OpAmp &opamp) : b{opamp.id() == 1 ? OpAmp::In1 : OpAmp::In2} {}
    InSwitch(InputPort &port) : b{port.input_connection_selector()} {}

    uint8_t b;
};

#endif
