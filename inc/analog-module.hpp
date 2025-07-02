#ifndef OBC_COMPONENTS_HPP
#define OBC_COMPONENTS_HPP

#include "shadow-sram.hpp"
#include "io-port.hpp"
#include "capacitor.hpp"
#include "opamp.hpp"
#include "comparator.hpp"
#include "defs.hpp"
#include <bitset>
#include <vector>
#include <fstream>
#include <cmath>

class AnalogBlock;

class Parameter {
public:
    Parameter(double value)
            : m_value{value} {}

    double as_double() { return m_value; }
    int64_t as_int() { return std::llround(m_value); }
    bool as_bool() { return as_int(); }

private:
    double m_value;
};

class AnalogModule {
public:
    AnalogModule(std::string const &name);

    /* Delete copy/move semantics as this breaks links with Ports. */
    AnalogModule(AnalogModule const &) = delete;
    AnalogModule &operator=(AnalogModule const &) = delete;
    
    AnalogModule(AnalogModule &&) = delete;
    AnalogModule &operator=(AnalogModule &&) = delete;

    virtual ~AnalogModule() = default;

    static AnalogModule *Build(std::string_view const &name);

    virtual bool set_parameter(std::string_view param, Parameter value) = 0;

    virtual void claim_components() = 0;
    virtual void finalize() = 0;

    virtual InputPort &in(std::size_t i = 0);
    virtual OutputPort &out(std::size_t i = 0);

    std::array<InputPort, 8> &ins() { return m_ins; }
    Capacitor &cap(int i = 0);
    OpAmp &opamp(int i);
    Comparator &comp();

    void set_cab(AnalogBlock &cab);
    AnalogBlock &cab() { return *m_cab; }

    std::string const &name() const { return m_name; }

protected:
    void claim_capacitors(std::size_t n);
    void claim_opamps(std::size_t n);
    void claim_comparator();
    void claim_inputs(std::size_t n);

    AnalogBlock *m_cab;

    std::string m_name;

    std::array<InputPort, 8> m_ins;
    std::array<Capacitor *, NCapacitorsPerBlock> m_caps;
    std::array<OpAmp *, NOpAmpsPerBlock> m_opamps;
    Comparator *m_comp;

    std::size_t m_curr_cap;
    std::size_t m_n_ins;
};

class GainInv : public AnalogModule {
public:
    GainInv();
    GainInv(double gain);

    bool set_parameter(std::string_view param, Parameter value) override;

    void claim_components() override;
    void finalize() override;

private:
    double m_gain;
};

class SumInv : public AnalogModule {
public:
    SumInv();
    SumInv(double gain1, double gain2, std::size_t n_inputs = 2);

    bool set_parameter(std::string_view param, Parameter value) override;

    void claim_components() override;
    void finalize() override;

private:
    std::array<double, 3> m_gains;
    std::size_t m_n_inputs;
};

class Integrator : public AnalogModule {
public:
    Integrator();
    Integrator(double integ_const, bool m_gnd_reset);

    bool set_parameter(std::string_view param, Parameter value) override;

    void claim_components() override;
    void finalize() override;

private:
    std::array<double, 3> m_integ_consts;
    std::array<bool, 3> m_invert;
    bool m_gnd_reset;
    std::size_t m_n_inputs;
};

class GainSwitch : public AnalogModule {
public:
    GainSwitch();

    bool set_parameter(std::string_view param, Parameter value) override;

    void claim_components() override;
    void finalize() override;
};

class SampleAndHold : public AnalogModule {
public:
    SampleAndHold();

    bool set_parameter(std::string_view param, Parameter value) override;

    void claim_components() override;
    void finalize() override;
};

#endif
