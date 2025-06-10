#include "comparator.hpp"
#include "error.hpp"
#include "analog-block.hpp"

Comparator::Comparator()
        : m_module{}, m_in{}, m_cfg{} {}

Comparator &Comparator::claim(AnalogModule &module) {
    if (m_module) {
        throw DesignError("Comparator already claimed");
    }

    m_module = &module;
    m_in = InputPort(module);

    return *this;
}

Comparator &Comparator::set_configuration(std::array<uint8_t, 2> cfg) {
    m_cfg = cfg;

    return *this;
}

void Comparator::compile(AnalogBlock const &cab, ShadowSRam &ssram) const {
    ssram.set(cab.bank_b(), 0x09, { m_cfg[0], m_cfg[1] });

    if (m_module) {
        ssram.set(cab.bank_a(), 0x0E, 0x08);
        ssram.set(cab.bank_b(), 0x06, 0x80);
        //ssram.set(cab.bank_b(), 0x1A, 0x6C);
    } else {
        ssram.set(cab.bank_a(), 0x0E, 0x00);
        ssram.set(cab.bank_b(), 0x06, 0x00);
        //ssram.set(cab.bank_b(), 0x1A, 0x00);
    }
}
