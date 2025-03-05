#ifndef OBC_COMPONENTS_HPP
#define OBC_COMPONENTS_HPP

#include "shadow-sram.hpp"
#include "analog-block.hpp"
#include "io-port.hpp"

class AnalogModule {
public:
    AnalogModule(AnalogBlock &cab);

    AnalogBlock &cab() { return m_cab; }

protected:
    AnalogBlock &m_cab;
};

class InvertingSum : public AnalogModule {
public:
    InvertingSum(AnalogBlock &block, double lgain, double ugain);

    void emit(ShadowSRam &ssram) const;

    InputPort &in_x() { return m_in_x; }
    InputPort &in_y() { return m_in_y; }
    OutputPort &out() { return m_out; }

private:
    double m_lgain;
    double m_ugain;

    InputPort m_in_x{*this};
    InputPort m_in_y{*this};
    OutputPort m_out{*this};
};

#endif
