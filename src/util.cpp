#include "util.hpp"
#include <limits>
#include <cmath>
#include <algorithm>
#include <iostream>

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
