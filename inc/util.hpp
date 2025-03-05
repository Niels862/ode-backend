#ifndef OBC_UTIL_HPP
#define OBC_UTIL_HPP

#include <cstddef>

struct GainEncodingTriple {
    std::byte lgain_numerator;
    std::byte ugain_numerator;
    std::byte denominator;
};

int round_and_clamp(double number, int lower, int upper);

GainEncodingTriple compute_gain_encoding(double lgain, double ugain);

#endif
