#ifndef OBC_UTIL_HPP
#define OBC_UTIL_HPP

#include <cinttypes>

struct GainEncodingTriple {
    uint8_t C_1;
    uint8_t C_2;
    uint8_t C_out;
};

int round_and_clamp(double number, int lower, int upper);

GainEncodingTriple compute_gain_encoding(double lgain, double ugain);

#endif
