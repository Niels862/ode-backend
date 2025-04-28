#include "analog-module.hpp"
#include "util.hpp"
#include "analog-block.hpp"
#include "error.hpp"
#include <sstream>

AnalogModule::AnalogModule(std::string const &name, std::size_t in_n, 
                           std::size_t out_n)
        : m_cab{}, m_name{name}, m_in_n{in_n}, m_out_n{out_n},
          m_ins{}, m_outs{} {
    for (std::size_t i = 0; i < m_in_n; i++) {
        m_ins.emplace_back(*this);
    }
    for (std::size_t i = 0; i < m_out_n; i++) {
        m_outs.emplace_back(*this); // todo should interact with CAB to assign unique output port
    }
}

AnalogModule *AnalogModule::Build(std::string const &name) {
    if (name == "GainInv")      return new GainInv();
    if (name == "SumInv")       return new SumInv();
    if (name == "Integrator")   return new Integrator();
    return nullptr;
}

uint8_t AnalogModule::connection_nibble(AnalogModule &to) {
    int id_from = cab().id();
    int id_to = to.cab().id();

    switch (id_from) {
        case 1:
            switch (id_to) {
                case 1: return 0x3;
                case 2: return 0xB;
                case 3: return 0xB;
                case 4: return 0xB;
            }
            break;
        
        case 2:
            switch (id_to) {
                case 1: return 0xF;
                case 2: return 0x3;
                case 3: return 0x9;
                case 4: return 0x9;
            }
            break;

        case 3:
            switch (id_to) {
                case 1: return 0xB;
                case 2: return 0xD;
                case 3: return 0x7;
                case 4: return 0xD;
            }
            break;

        case 4:
            switch (id_to) {
                case 1: return 0xD;
                case 2: return 0xF;
                case 3: return 0xF;
                case 4: return 0x3;
            }
            break;
    }

    throw DesignError("Unreached todo");
}

InputPort &AnalogModule::in(std::size_t i) {
    if (i < 1 || i > m_in_n) {
        std::stringstream ss;
        ss << "`" << m_name << "` does not implement in(" << i << ")";
        throw DesignError(ss.str());
    }

    return m_ins[i - 1];
}

InputPort &AnalogModule::in() {
    if (m_in_n != 1) {
        std::stringstream ss;
        ss << "`" << m_name << "` does not support in()";
        throw DesignError(ss.str());
    }

    return in(1);
}

OutputPort &AnalogModule::out(std::size_t i) {
    if (i < 1 || i > m_out_n) {
        std::stringstream ss;
        ss << "`" << m_name << "` does not implement out(" << i << ")";
        throw DesignError(ss.str());
    }

    return m_outs[i - 1];
}

OutputPort &AnalogModule::out() {
    if (m_out_n != 1) {
        std::stringstream ss;
        ss << "`" << m_name << "` does not support out()";
        throw DesignError(ss.str());
    }

    return out(1);
}

void AnalogModule::set_cab(AnalogBlock &cab) {
    m_cab = &cab; 
}

GainInv::GainInv()
        : AnalogModule{"GainInv", 1, 1}, m_gain{} {}

GainInv::GainInv(double gain)
        : AnalogModule{"GainInv", 1, 1}, m_gain{gain} {}

void GainInv::parse(std::ifstream &file) {
    file >> m_gain;
}

void GainInv::configure() {
    uint8_t num, den;
    approximate_ratio(m_gain, num, den);

    OpAmp &opamp = cab().claim_opamp(true);

    cab().claim_cap(num)
         .set_in(Capacitor::from_input(in()))
         .set_out(Capacitor::to_opamp(opamp));
    cab().claim_cap(num)
         .set_in(Capacitor::from_input(in(), 1))
         .set_out(Capacitor::to_opamp(opamp, 1));
    cab().claim_cap(den)
         .set_in(Capacitor::from_opamp(opamp))
         .set_out(Capacitor::to_opamp(opamp));
    cab().claim_cap(den)
         .set_in(Capacitor::from_opamp(opamp, 1))
         .set_out(Capacitor::to_opamp(opamp, 1));
}

SumInv::SumInv()
        : AnalogModule{"SumInv", 2, 1}, m_lgain{}, m_ugain{} {}

SumInv::SumInv(double lgain, double ugain)
        : AnalogModule{"SumInv", 2, 1}, m_lgain{lgain}, m_ugain{ugain} {}

void SumInv::parse(std::ifstream &file) {
    file >> m_lgain;
    file >> m_ugain;
}

void SumInv::configure() {
    std::vector<double> gains = { m_lgain, m_ugain };
    std::vector<uint8_t> nums;
    uint8_t den;
    approximate_ratios(gains, nums, den);

    OpAmp &opamp = cab().claim_opamp(true);

    cab().claim_cap(nums[0])
         .set_in(Capacitor::from_input(in(1)))
         .set_out(Capacitor::to_opamp(opamp));
    cab().claim_cap(nums[0])
         .set_in(Capacitor::from_input(in(1), 1))
         .set_out(Capacitor::to_opamp(opamp, 1));
    cab().claim_cap(nums[1])
         .set_in(Capacitor::from_input(in(2)))
         .set_out(Capacitor::to_opamp(opamp));
    cab().claim_cap(nums[1])
         .set_in(Capacitor::from_input(in(2), 1))
         .set_out(Capacitor::to_opamp(opamp, 1));
    cab().claim_cap(den)
         .set_in(Capacitor::from_opamp(opamp))
         .set_out(Capacitor::to_opamp(opamp));
    cab().claim_cap(den)
         .set_in(Capacitor::from_opamp(opamp, 1))
         .set_out(Capacitor::to_opamp(opamp, 1));
}

Integrator::Integrator()
        : AnalogModule("Integrator", 1, 1), m_integ_const{}, m_gnd_reset{},
          m_comp{} {}

Integrator::Integrator(double integ_const, bool gnd_reset)
        : AnalogModule{"Integrator", 1, 1}, 
          m_integ_const{integ_const}, m_gnd_reset{gnd_reset},
          m_comp{} {
    if (m_gnd_reset) {
        m_comp = &cab().claim_comp();
    }
}

void Integrator::parse(std::ifstream &file) {
    file >> m_integ_const;
    file >> m_gnd_reset;
}

void Integrator::configure() {
    uint8_t num, den;
    approximate_ratio(m_integ_const / 4, num, den);

    OpAmp &opamp = cab().claim_opamp(true);

    cab().claim_cap(num)
         .set_in(Capacitor::from_input(in(), 1))
         .set_out(Capacitor::to_opamp(opamp, 2));
    cab().claim_cap(den)
         .set_in(Capacitor::from_opamp(opamp))
         .set_out(Capacitor::to_opamp(opamp));

    if (m_gnd_reset) {
        cab().claim_comp();
    }
}
