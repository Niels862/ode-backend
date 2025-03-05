#include "components.hpp"
#include "util.hpp"

InvertingSum::InvertingSum(double lgain, double ugain, CAB::ID cab)
        : m_lgain{lgain}, m_ugain{ugain}, m_cab{cab} {}

void InvertingSum::emit(ShadowSRam &ssram) const {
    std::size_t A = ShadowSRam::cab_bank_a(m_cab);

    auto triple = compute_gain_encoding(m_lgain, m_ugain);
    for (std::size_t i = 0; i < 2; i++) {
        ssram.set(A, 2 + i, triple.denominator);
        ssram.set(A, 4 + i, triple.ugain_numerator);
        ssram.set(A, 6 + i, triple.lgain_numerator);
    }

    ssram.set(A, 20, {
        0x01, 0x13, 0x01, 0x81, 0x01, 0x10, 0x01, 0x81, 0x01, 0x10, 0x01, 0x81
    });
}
