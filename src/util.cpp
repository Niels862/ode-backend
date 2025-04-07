#include "util.hpp"
#include <limits>
#include <cmath>
#include <algorithm>
#include <iostream>

void compute_ratios(std::vector<double> const &values,
                    std::vector<uint8_t> &nums, uint8_t den) {
    nums.clear();
    for (std::size_t i = 0; i < values.size(); i++) {
        double v = values[i];
        double num = v * den;

        nums.push_back(std::clamp(static_cast<int>(num), 0, 255));
    }
}

void approximate_ratios(std::vector<double> const &values, 
                        std::vector<uint8_t> &nums, uint8_t &den) {
    uint8_t best_den = 0;
    double best_den_delta = std::numeric_limits<double>::infinity();
    
    for (uint8_t d = 255; d > 0; d--) {
        compute_ratios(values, nums, d);

        double delta = 0;
        for (std::size_t i = 0; i < values.size(); i++) {
            delta += std::abs(static_cast<double>(nums[i]) / d - values[i]);
        }

        if (delta < best_den_delta) {
            best_den = d;
            best_den_delta = delta;
        }
    }

    compute_ratios(values, nums, best_den);
    den = best_den;
}

void approximate_ratio(double value, uint8_t &num, uint8_t &den) {
    std::vector<double> values = { value };
    std::vector<uint8_t> nums;
    approximate_ratios(values, nums, den);
    num = nums.front();
}

int round_and_clamp(double number, int lower, int upper) {
    return std::clamp(static_cast<int>(std::round(number)), lower, upper);
}

GainEncodingTriple compute_gain_encoding(double lgain, double ugain) {
    GainEncodingTriple best;
    double best_delta = std::numeric_limits<double>::infinity();

    /* First, an initial denominator is approximated */
    int estimated_denominator = std::clamp(
        static_cast<int>(std::round(255 / std::max(lgain, ugain))), 
        1, 255
    );

    /* Then, a local search around the approximation is performed */
    for (int denominator = std::max(1, estimated_denominator + 16);
            denominator >= std::min(255, estimated_denominator - 16);
            denominator--) {
        int lgain_num = round_and_clamp(lgain * denominator, 0, 255);
        int ugain_num = round_and_clamp(ugain * denominator, 0, 255);

        double res_lgain = static_cast<double>(lgain_num) / denominator;
        double res_ugain = static_cast<double>(ugain_num) / denominator;

        double delta = std::abs(res_lgain - lgain) 
                     + std::abs(res_ugain - ugain);
        
        if (delta < best_delta) {
            best.C_out = denominator;
            best.C_1 = lgain_num;
            best.C_2 = ugain_num;
            best_delta = delta;
        }
    }

    return best;
}
