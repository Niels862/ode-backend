#ifndef OBC_COMPONENTS_HPP
#define OBC_COMPONENTS_HPP

#include "shadow-sram.hpp"
#include "io-port.hpp"
#include "defs.hpp"
#include <bitset>
#include <vector>

class AnalogBlock;

class AnalogModule {
public:
    AnalogModule(std::string const &name, std::size_t n_in, std::size_t n_out);

    /* Delete copy/move semantics as this breaks links with Ports. */
    AnalogModule(AnalogModule const &) = delete;
    AnalogModule &operator=(AnalogModule const &) = delete;
    
    AnalogModule(AnalogModule &&) = delete;
    AnalogModule &operator=(AnalogModule &&) = delete;

    virtual ~AnalogModule() = default;

    virtual uint8_t connection_nibble(AnalogModule &to); 

    virtual void configure() = 0;

    InputPort &in(std::size_t i);
    InputPort &in();
    OutputPort &out(std::size_t i);
    OutputPort &out();

    void set_cab(AnalogBlock &cab);
    AnalogBlock &cab() { return *m_cab; }

    std::string const &name() const { return m_name; }

protected:
    AnalogBlock *m_cab;

    std::string m_name;
    std::size_t m_in_n;
    std::size_t m_out_n;

    std::vector<InputPort> m_ins;
    std::vector<OutputPort> m_outs;
};

class InvGain : public AnalogModule {
public:
    InvGain(double gain);

    void configure() override;

private:
    double m_gain;
};

class InvSum : public AnalogModule {
public:
    InvSum(double lgain, double ugain);

    void configure() override;

private:
    double m_lgain;
    double m_ugain;
};

#endif
