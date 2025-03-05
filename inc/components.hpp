#ifndef OBC_COMPONENTS_HPP
#define OBC_COMPONENTS_HPP

#include "shadow-sram.hpp"
#include "cab.hpp"

class InvertingSum {
public:
    InvertingSum(double lgain, double ugain, CAB::ID cab);

    void emit(ShadowSRam &ssram) const;

private:
    double m_lgain;
    double m_ugain;

    CAB::ID m_cab;
};

#endif
