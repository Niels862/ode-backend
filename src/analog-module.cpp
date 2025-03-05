#include "analog-module.hpp"
#include "util.hpp"

AnalogModule::AnalogModule(AnalogBlock &cab)
        : m_cab{cab} {}

InvertingSum::InvertingSum(AnalogBlock &cab, 
                           double lgain, double ugain)
        : AnalogModule{cab}, m_lgain{lgain}, m_ugain{ugain} {}

void InvertingSum::emit(ShadowSRam &ssram) const {
    std::size_t A = m_cab.bank_a();

    auto triple = compute_gain_encoding(m_lgain, m_ugain);
    for (std::size_t i = 0; i < 2; i++) {
        ssram.set(A, 2 + i, triple.denominator);
        ssram.set(A, 4 + i, triple.ugain_numerator);
        ssram.set(A, 6 + i, triple.lgain_numerator);
    }

    ssram.set(A, 20, {
        0x01, 0x13, 0x01, 0x81, 
        0x01, 0x10, 0x01, 0x81, 
        0x01, 0x10, 0x01, 0x81
    });
}
