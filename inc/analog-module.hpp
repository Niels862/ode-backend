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

class AnalogBlock;

class AnalogModule {
public:
    AnalogModule(std::string const &name, std::size_t n_in);

    /* Delete copy/move semantics as this breaks links with Ports. */
    AnalogModule(AnalogModule const &) = delete;
    AnalogModule &operator=(AnalogModule const &) = delete;
    
    AnalogModule(AnalogModule &&) = delete;
    AnalogModule &operator=(AnalogModule &&) = delete;

    virtual ~AnalogModule() = default;

    static AnalogModule *Build(std::string const &name);

    virtual uint8_t connection_nibble(AnalogModule &to); 

    virtual void parse(std::ifstream &file) = 0;

    virtual void claim_components() = 0;
    virtual void finalize() = 0;

    virtual InputPort &in(std::size_t i = 1);
    virtual OutputPort &out(std::size_t i = 1);

    std::vector<InputPort> &ins() { return m_ins; }
    Capacitor &cap(int i);
    OpAmp &opamp(int i);
    Comparator &comp();

    void set_cab(AnalogBlock &cab);
    AnalogBlock &cab() { return *m_cab; }

    std::string const &name() const { return m_name; }

protected:
    void claim_capacitors(size_t n);
    void claim_opamps(size_t n);
    void claim_comparator();

    AnalogBlock *m_cab;

    std::string m_name;
    std::size_t m_in_n;

    std::vector<InputPort> m_ins;
    std::array<Capacitor *, NCapacitorsPerBlock> m_caps;
    std::array<OpAmp *, NOpAmpsPerBlock> m_opamps;
    Comparator *m_comp;
};

/* AD20 Modules */

class GainInv : public AnalogModule {
public:
    GainInv();
    GainInv(double gain);

    void parse(std::ifstream &file) override;

    void claim_components() override;
    void finalize() override;

private:
    double m_gain;
};

class SumInv : public AnalogModule {
public:
    SumInv();
    SumInv(double lgain, double ugain);

    void parse(std::ifstream &file) override;

    void claim_components() override;
    void finalize() override;

private:
    double m_lgain;
    double m_ugain;
};

class Integrator : public AnalogModule {
public:
    Integrator();
    Integrator(double integ_const, bool m_gnd_reset);

    void parse(std::ifstream &file) override;

    void claim_components() override;
    void finalize() override;

private:
    double m_integ_const;
    bool m_gnd_reset;
};

class GainSwitch : public AnalogModule {
public:
    GainSwitch();

    void parse(std::ifstream &) override {}

    void claim_components() override;
    void finalize() override;
};

class SampleAndHold : public AnalogModule {
public:
    SampleAndHold();

    void parse(std::ifstream &) override {}

    void claim_components() override;
    void finalize() override;
};

/* Custom Modules */

class SingleGainInv : public AnalogModule {
public:
    SingleGainInv();
    SingleGainInv(double gain);

    void parse(std::ifstream &file) override;

    void claim_components() override;
    void finalize() override;

private:
    double m_gain;
};

class SingleSumInv : public AnalogModule {
public:
    SingleSumInv();
    SingleSumInv(double lgain, double ugain);

    void parse(std::ifstream &file) override;

    void claim_components() override;
    void finalize() override;

private:
    double m_lgain;
    double m_ugain;
};    

#endif
