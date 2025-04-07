#ifndef OBC_UTIL_HPP
#define OBC_UTIL_HPP

#include <vector>
#include <cinttypes>

void compute_ratios(std::vector<double> const &values,
                    std::vector<uint8_t> &nums, uint8_t den);

void approximate_ratios(std::vector<double> const &values, 
                        std::vector<uint8_t> &nums, uint8_t &den);

void approximate_ratio(double value, uint8_t &num, uint8_t &den);

struct GainEncodingTriple {
    uint8_t C_1;
    uint8_t C_2;
    uint8_t C_out;
};

int round_and_clamp(double number, int lower, int upper);

GainEncodingTriple compute_gain_encoding(double lgain, double ugain);

template <typename T>
constexpr bool is_odd(T const n) { return n % 2 == 1; }

template <typename T>
constexpr bool is_even(T const n) { return n % 2 == 1; }

#endif
