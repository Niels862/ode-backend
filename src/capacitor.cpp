#include "capacitor.hpp"
#include "assert.h"
#include "analog-block.hpp"
#include "opamp.hpp"
#include "io-port.hpp"
#include "error.hpp"
#include <sstream>

Capacitor::Capacitor() 
        : m_id{}, m_is_used{false}, m_value{}, m_switch_cfg{} {}

Capacitor::Capacitor(int id)
        : m_id{id}, m_is_used{false}, m_value{0x0}, m_switch_cfg{} {}

SwitchConfiguration Capacitor::from_input(InputPort &port, 
                                          int sg_phase) {
    uint8_t nibble = port.connection_nibble();
    if (sg_phase) {
        return { 0x01, from_nibbles(0x1, nibble) };
    }
    return { 0x00, from_nibbles(nibble, 0x0) };
}

SwitchConfiguration Capacitor::from_opamp(OpAmp const &opamp, 
                                          int sg_phase) {
    uint8_t n = opamp.id() == 1 ? 0x3 : 0x2;
    
    switch (sg_phase) {
        case 0: return { 0x00, from_nibbles(n, 0x0) };
        case 1: return { 0x01, from_nibbles(0x1, n) };
        case 2: return { 0x01, from_nibbles(n, 0x1) };
    }
    
    throw DesignError("Invalid phase");
}

SwitchConfiguration Capacitor::to_opamp(OpAmp const &opamp, 
                                        int sg_phase) {
    uint8_t n = opamp.id() == 1 ? 0x1 : 0x2;

    switch (sg_phase) {
        case 0: return { 0x00, from_nibbles(n, 0x0) };
        case 1: return { 0x01, from_nibbles(0x8, n) };
        case 2: return { 0x01, from_nibbles(n, 0x8) };
    }
    
    throw DesignError("Invalid phase");
}

Capacitor &Capacitor::claim(int value) {                     
    if (m_is_used) {
        std::stringstream ss;
        ss << "Capacitor " << m_id << " is already in use";
        throw DesignError(ss.str());
    }

    m_value = value;
    m_is_used = true;

    return *this;
}

Capacitor &Capacitor::set_in(SwitchConfiguration switch_cfg) {
    m_switch_cfg[0] = switch_cfg.b1;
    m_switch_cfg[1] = switch_cfg.b2;

    return *this;
}

Capacitor &Capacitor::set_out(SwitchConfiguration switch_cfg) {
    m_switch_cfg[2] = switch_cfg.b1;
    m_switch_cfg[3] = switch_cfg.b2;

    return *this;
}

void Capacitor::compile(AnalogBlock const &cab, 
                        ShadowSRam &ssram) const {
    MemoryAddress switch_addr = switch_address(cab);
    MemoryAddress gain_addr = value_address(cab);

    ssram.set(gain_addr.bank_addr, 
              gain_addr.byte_addr, 
              m_value);

    for (std::size_t i = 0; i < 4; i++) {
        ssram.set(switch_addr.bank_addr, 
                  switch_addr.byte_addr + i, 
                  m_switch_cfg[i]);
    }
}

MemoryAddress Capacitor::switch_address(AnalogBlock const &cab) const {
    MemoryAddress addr;

    switch (m_id) {
        case 1:
            return MemoryAddress(cab.bank_b(), 0x1C);

        case 2:
            return MemoryAddress(cab.bank_a(), 0x1C);
        
        case 3:
            return MemoryAddress(cab.bank_b(), 0x16);
        
        case 4:
            return MemoryAddress(cab.bank_a(), 0x18);

        case 5:
            return MemoryAddress(cab.bank_b(), 0x10);

        case 6:
            return MemoryAddress(cab.bank_a(), 0x14);

        case 7:
            return MemoryAddress(cab.bank_b(), 0x0C);

        case 8:
            return MemoryAddress(cab.bank_a(), 0x10);

        default:
            return MemoryAddress(); // todo error
    }
}

MemoryAddress Capacitor::value_address(AnalogBlock const &cab) const {
    return MemoryAddress(cab.bank_a(), 0x08 - m_id);
}
