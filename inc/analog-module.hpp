#ifndef OBC_COMPONENTS_HPP
#define OBC_COMPONENTS_HPP

#include "shadow-sram.hpp"
#include "io-port.hpp"

class AnalogBlock;

class AnalogModule {
public:
    AnalogModule();

    virtual ~AnalogModule() = default;

    virtual void setup() = 0;

    void set_cab(AnalogBlock &cab);
    AnalogBlock &cab() { return *m_cab; }

protected:
    AnalogBlock *m_cab;
};

class InvSum : public AnalogModule {
public:
    InvSum(double lgain, double ugain);

    void setup() override;

    InputPort &in_x() { return m_in_x; }
    InputPort &in_y() { return m_in_y; }
    OutputPort &out() { return m_out; }

private:
    double m_lgain;
    double m_ugain;

    InputPort m_in_x;
    InputPort m_in_y;
    OutputPort m_out;
};

#endif
