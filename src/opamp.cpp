#include "opamp.hpp"
#include "analog-block.hpp"
#include "error.hpp"
#include <sstream>

OpAmp::OpAmp()
        : m_id{}, m_is_used{false}, m_closed_loop{} {}

OpAmp::OpAmp(int id)
        : m_id{id}, m_is_used{false} {}

OpAmp &OpAmp::claim(bool closed_loop) {
    if (m_is_used) {
        std::stringstream ss;
        ss << "OpAmp " << m_id << " is already in use";
        throw DesignError(ss.str());
    }

    m_is_used = true;
    m_closed_loop = closed_loop;

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
    
    if (!m_is_used) {
        ssram.set(cab.bank_b(), m_byte_addr, { 0x00, 0x00 });
    } else if (m_closed_loop) {
        ssram.set(cab.bank_b(), m_byte_addr, { 0x00, 0x05 });
    } else {
        throw DesignError("Not implemented");
    }
}
