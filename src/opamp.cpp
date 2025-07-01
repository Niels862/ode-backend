#include "opamp.hpp"
#include "analog-block.hpp"
#include "error.hpp"
#include <sstream>

OpAmp::OpAmp()
        : m_id{}, m_module{}, m_switch_cfg{} {}

OpAmp::OpAmp(AnalogBlock &cab, int id)
        : m_id{id}, m_module{}, m_switch_cfg{},
          m_out{cab, id == 1 
                     ? OutPortSource::OpAmp1
                     : OutPortSource::OpAmp2} {} 

OpAmp &OpAmp::claim(AnalogModule &module) {
    if (m_module) {
        std::stringstream ss;
        ss << "OpAmp " << m_id << " is already in use";
        throw DesignError(ss.str());
    }

    m_module = &module;
    if (m_id == 1) {
        m_out = OutputPort(module.cab(), OutPortSource::OpAmp1);
    } else {
        m_out = OutputPort(module.cab(), OutPortSource::OpAmp2);
    }

    return set_feedback({ 0x00, 0x05 });
}

OpAmp &OpAmp::set_feedback(SwitchConfiguration switch_cfg) {
    m_switch_cfg[0] = switch_cfg.b1;
    m_switch_cfg[1] = switch_cfg.b2;

    return *this;
}

void OpAmp::compile(AnalogBlock const &cab, ShadowSRam &ssram) const {
    std::size_t m_byte_addr;
    if (m_id == 1) {
        m_byte_addr = 0x1A;
    } else if (m_id == 2) {
        m_byte_addr = 0x14;
    } else {
        throw DesignError("Invalid ID");
    }
    
    for (std::size_t i = 0; i < 2; i++) {
        ssram.set(cab.bank_b(), m_byte_addr + i, m_switch_cfg[i]);
    }
}
