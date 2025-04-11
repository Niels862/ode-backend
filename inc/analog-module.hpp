#ifndef OBC_COMPONENTS_HPP
#define OBC_COMPONENTS_HPP

#include "shadow-sram.hpp"
#include "io-port.hpp"
#include "defs.hpp"
#include <bitset>

class AnalogBlock;

class AnalogModule {
public:
    AnalogModule(std::string const &name);

    /* Delete copy/move semantics as this breaks links with Ports. */
    AnalogModule(AnalogModule const &) = delete;
    AnalogModule &operator=(AnalogModule const &) = delete;
    
    AnalogModule(AnalogModule &&) = delete;
    AnalogModule &operator=(AnalogModule &&) = delete;

    virtual ~AnalogModule() = default;

    virtual uint8_t connection_nibble(AnalogModule &to); 

    virtual void configure() = 0;

    void set_cab(AnalogBlock &cab);
    AnalogBlock &cab() { return *m_cab; }

    std::string const &name() const { return m_name; }

protected:
    AnalogBlock *m_cab;

    std::string m_name;
};

class InvGain : public AnalogModule {
public:
    InvGain(double gain);

    void configure() override;

    InputPort &in() { return m_in; }
    OutputPort &out() { return m_out; }

private:
    double m_gain;

    InputPort m_in;
    OutputPort m_out;
};

class InvSum : public AnalogModule {
public:
    InvSum(double lgain, double ugain);

    void configure() override;

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
