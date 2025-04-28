#ifndef OBC_COMPONENTS_HPP
#define OBC_COMPONENTS_HPP

#include "shadow-sram.hpp"
#include "io-port.hpp"
#include "comparator.hpp"
#include "defs.hpp"
#include <bitset>
#include <vector>
#include <fstream>

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

    static AnalogModule *Build(std::string const &name);

    virtual uint8_t connection_nibble(AnalogModule &to); 

    virtual void parse(std::ifstream &file) = 0;
    virtual void configure() = 0;

    virtual InputPort &in(std::size_t i);
    virtual InputPort &in();
    virtual OutputPort &out(std::size_t i);
    virtual OutputPort &out();

    std::vector<InputPort> &ins() { return m_ins; }
    std::vector<OutputPort> &outs() { return m_outs; }

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

class GainInv : public AnalogModule {
public:
    GainInv();
    GainInv(double gain);

    void parse(std::ifstream &file) override;
    void configure() override;

private:
    double m_gain;
};

class SumInv : public AnalogModule {
public:
    SumInv();
    SumInv(double lgain, double ugain);

    void parse(std::ifstream &file) override;
    void configure() override;

private:
    double m_lgain;
    double m_ugain;
};

class Integrator : public AnalogModule {
public:
    Integrator();
    Integrator(double integ_const, bool m_gnd_reset);

    void parse(std::ifstream &file) override;
    void configure() override;

private:
    double m_integ_const;
    bool m_gnd_reset;

    Comparator *m_comp;
};

#endif
