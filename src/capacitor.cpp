#include "capacitor.hpp"
#include "assert.h"
#include "analog-block.hpp"
#include "opamp.hpp"
#include "io-port.hpp"
#include "error.hpp"
#include "cassert"
#include <sstream>

Capacitor::Capacitor() 
        : m_id{}, m_module{}, m_value{}, m_switch_cfg{} {}

Capacitor::Capacitor(int id)
        : m_id{id}, m_module{}, m_value{0x0}, m_switch_cfg{} {}

SwitchConfiguration Capacitor::from_input(InputPort &in, 
                                          int sg_phase,
                                          Clock::Select select) {
    assert(sg_phase || select != Clock::B);
    
    uint8_t n = in.switch_connection_selector();
    uint8_t s = select == Clock::A ? 0x01 : 0x02;

    switch (sg_phase) {
        case 0: return { 0x00, from_nibbles(n, 0x0) };
        case 1: return { s,    from_nibbles(0x1, n) };
        case 2: return { s,    from_nibbles(n, 0x1) };
    }

    throw DesignError("Invalid phase");
}

SwitchConfiguration Capacitor::from_opamp(OpAmp const &opamp, 
                                          int sg_phase,
                                          Clock::Select select) {
    assert(sg_phase || select != Clock::B);
                                            
    uint8_t n = opamp.id() == 1 ? 0x3 : 0x2;
    uint8_t s = select == Clock::A ? 0x01 : 0x02;

    switch (sg_phase) {
        case 0: return { 0x00, from_nibbles(n, 0x0) };
        case 1: return { s,    from_nibbles(0x1, n) };
        case 2: return { s,    from_nibbles(n, 0x1) };
    }
    
    throw DesignError("Invalid phase");
}

SwitchConfiguration Capacitor::to_opamp(OpAmp const &opamp, 
                                        int sg_phase,
                                        Clock::Select select) {
    assert(sg_phase || select != Clock::B);
    
    uint8_t n = opamp.id() == 1 ? 0x1 : 0x2;
    uint8_t s = select == Clock::A ? 0x01 : 0x02;

    switch (sg_phase) {
        case 0: return { 0x00, from_nibbles(n, 0x0) };
        case 1: return { s,    from_nibbles(0x8, n) };
        case 2: return { s,    from_nibbles(n, 0x8) };
    }
    
    throw DesignError("Invalid phase");
}

SwitchConfiguration Capacitor::switched_in(InSwitch phase1,
                                           InSwitch phase2,
                                           Clock::Select select) {
    uint8_t s = select == Clock::A ? 0x01 : 0x02;
    return { s, from_nibbles(phase1.b, phase2.b) };
}

Capacitor &Capacitor::claim(AnalogModule &module) {                     
    if (m_module) {
        std::stringstream ss;
        ss << "Capacitor " << m_id << " is already in use";
        throw DesignError(ss.str());
    }

    m_module = &module;

    return *this;
}

Capacitor &Capacitor::set_value(uint8_t value) {
    m_value = value;

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
