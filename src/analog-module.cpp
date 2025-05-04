#include "analog-module.hpp"
#include "util.hpp"
#include "analog-block.hpp"
#include "error.hpp"
#include <sstream>

AnalogModule::AnalogModule(std::string const &name, std::size_t in_n)
        : m_cab{}, m_name{name}, m_in_n{in_n},
          m_ins{}, m_caps{}, m_opamps{}, m_comp{} {
    for (std::size_t i = 0; i < m_in_n; i++) {
        m_ins.emplace_back(*this);
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

void AnalogModule::claim_capacitors(size_t n) {
    for (size_t i = 0; i < n; i++) {
        m_caps[i] = &m_cab->claim_cap(0); // FIXME
    }
}

void AnalogModule::claim_opamps(size_t n) {
    for (size_t i = 0; i < n; i++) {
        m_opamps[i] = &m_cab->claim_opamp(0); // FIXME
    }
}

void AnalogModule::claim_comparator() {
    m_comp = &m_cab->claim_comp();
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

OutputPort &AnalogModule::out(std::size_t) {
    throw DesignError("out() not implemented");
}

OutputPort &AnalogModule::out() {
    throw DesignError("out() not implemented");
}

Capacitor &AnalogModule::cap(int i) {
    Capacitor *cap = m_caps.at(i);
    if (cap == nullptr) {
        std::stringstream ss;
        ss << "Capacitor # " << i << " was not claimed by " << m_name;
        throw DesignError(ss.str());
    }
    return *cap;
}

OpAmp &AnalogModule::opamp(int i) {
    OpAmp *opamp = m_opamps.at(i);
    if (opamp == nullptr) {
        std::stringstream ss;
        ss << "OpAmp #" << i << " was not claimed by " << m_name;
        throw DesignError(ss.str());
    }
    return *opamp;
}

Comparator &AnalogModule::comp() {
    if (!m_comp) {
        std::stringstream ss;
        ss << "Comparator was not claimed by " << m_name;
        throw DesignError(ss.str());
    }
    return *m_comp;
}

void AnalogModule::set_cab(AnalogBlock &cab) {
    m_cab = &cab; 
}

GainInv::GainInv()
        : AnalogModule{"GainInv", 1}, m_gain{} {}

GainInv::GainInv(double gain)
        : AnalogModule{"GainInv", 1}, m_gain{gain} {}

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

void GainInv::claim_components() {
    claim_capacitors(4);
    claim_opamps(1);
}

void GainInv::finalize() {

}

SumInv::SumInv()
        : AnalogModule{"SumInv", 2}, m_lgain{}, m_ugain{} {}

SumInv::SumInv(double lgain, double ugain)
        : AnalogModule{"SumInv", 2}, m_lgain{lgain}, m_ugain{ugain} {}

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

void SumInv::claim_components() {
    claim_capacitors(6);
    claim_opamps(1);
}

void SumInv::finalize() {
    
}

Integrator::Integrator()
        : AnalogModule("Integrator", 1), 
          comp_in{*this},
          m_integ_const{}, m_gnd_reset{},
          m_comp{} {}

Integrator::Integrator(double integ_const, bool gnd_reset)
        : AnalogModule{"Integrator", 1}, 
          comp_in{*this},
          m_integ_const{integ_const}, m_gnd_reset{gnd_reset},
          m_comp{} {
    /*if (m_gnd_reset) {
        m_comp = &cab().claim_comp();
    }*/
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
         .set_in(Capacitor::from_input(in(), 1, Clock::B))
         .set_out(Capacitor::to_opamp(opamp, 2, Clock::B));
    cab().claim_cap(den)
         .set_in(Capacitor::from_opamp(opamp))
         .set_out(Capacitor::to_opamp(opamp));

    if (m_gnd_reset) {
        cab().claim_comp();
    }
}

void Integrator::claim_components() {
    claim_capacitors(2);
    claim_opamps(1);
    if (m_gnd_reset) {
        claim_comparator();
    }
}

void Integrator::finalize() {
    
}
