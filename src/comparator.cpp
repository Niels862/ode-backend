#include "comparator.hpp"
#include "error.hpp"
#include "analog-block.hpp"

Comparator::Comparator()
        : m_is_used{false} {}

Comparator &Comparator::claim() {
    if (m_is_used) {
        throw DesignError("Comparator already claimed");
    }
    m_is_used = true;
    
    return *this;
}

void Comparator::compile(AnalogBlock const &cab, ShadowSRam &ssram) const {
    if (m_is_used) {
        ssram.set(cab.bank_b(), 0x09, { 0x07, 0xC9 });
    } else {
        ssram.set(cab.bank_b(), 0x09, { 0x00, 0x00 });
    }
}
