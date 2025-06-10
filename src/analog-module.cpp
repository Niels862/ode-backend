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

InputPort &AnalogModule::in(std::size_t i) {
    if (i < 1 || i > m_in_n) {
        std::stringstream ss;
        ss << "`" << m_name << "` does not implement in(" << i << ")";
        throw DesignError(ss.str());
    }

    return m_ins[i - 1];
}

OutputPort &AnalogModule::out(std::size_t) {
    throw DesignError("out() not implemented");
}

Capacitor &AnalogModule::cap(int i) {
    Capacitor *cap = m_caps.at(i - 1);
    if (cap == nullptr) {
        std::stringstream ss;
        ss << "Capacitor # " << i << " was not claimed by " << m_name;
        throw DesignError(ss.str());
    }
    return *cap;
}

OpAmp &AnalogModule::opamp(int i) {
    OpAmp *opamp = m_opamps.at(i - 1);
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

void AnalogModule::claim_capacitors(size_t n) {
    for (size_t i = 0; i < n; i++) {
        m_caps[i] = &m_cab->claim_cap(*this); 
    }
}

void AnalogModule::claim_opamps(size_t n) {
    for (size_t i = 0; i < n; i++) {
        m_opamps[i] = &m_cab->claim_opamp(*this); 
    }
}

void AnalogModule::claim_comparator() {
    m_comp = &m_cab->claim_comp(*this);
}

GainInv::GainInv()
        : AnalogModule{"GainInv", 1}, m_gain{} {}

GainInv::GainInv(double gain)
        : AnalogModule{"GainInv", 1}, m_gain{gain} {}

void GainInv::parse(std::ifstream &file) {
    file >> m_gain;
}

void GainInv::claim_components() {
    claim_capacitors(4);
    claim_opamps(1);
}

void GainInv::finalize() {
    uint8_t num, den;
    approximate_ratio(m_gain, num, den);

    OpAmp &_opamp = opamp(1);

    cap(1).set_value(num)
          .set_in(Capacitor::from_input(in()))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(2).set_value(num)
          .set_in(Capacitor::from_input(in(), 1))
          .set_out(Capacitor::to_opamp(_opamp, 1));
    cap(3).set_value(den)
          .set_in(Capacitor::from_opamp(_opamp))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(4).set_value(den)
          .set_in(Capacitor::from_opamp(_opamp, 1))
          .set_out(Capacitor::to_opamp(_opamp, 1));

}

SumInv::SumInv()
        : AnalogModule{"SumInv", 2}, m_lgain{}, m_ugain{} {}

SumInv::SumInv(double lgain, double ugain)
        : AnalogModule{"SumInv", 2}, m_lgain{lgain}, m_ugain{ugain} {}

void SumInv::parse(std::ifstream &file) {
    file >> m_lgain;
    file >> m_ugain;
}

void SumInv::claim_components() {
    claim_capacitors(6);
    claim_opamps(1);
}

void SumInv::finalize() {
    std::vector<double> gains = { m_lgain, m_ugain };
    std::vector<uint8_t> nums;
    uint8_t den;
    approximate_ratios(gains, nums, den);

    OpAmp &_opamp = opamp(1);

    cap(1).set_value(nums[0])
          .set_in(Capacitor::from_input(in(1)))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(2).set_value(nums[0])
          .set_in(Capacitor::from_input(in(1), 1))
          .set_out(Capacitor::to_opamp(_opamp, 1));
    cap(3).set_value(nums[1])
          .set_in(Capacitor::from_input(in(2)))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(4).set_value(nums[1])
          .set_in(Capacitor::from_input(in(2), 1))
          .set_out(Capacitor::to_opamp(_opamp, 1));
    cap(5).set_value(den)
          .set_in(Capacitor::from_opamp(_opamp))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(6).set_value(den)
          .set_in(Capacitor::from_opamp(_opamp, 1))
          .set_out(Capacitor::to_opamp(_opamp, 1));
}

Integrator::Integrator()
        : AnalogModule("Integrator", 1), 
          m_integ_const{}, m_gnd_reset{} {}

Integrator::Integrator(double integ_const, bool gnd_reset)
        : AnalogModule{"Integrator", 1}, 
          m_integ_const{integ_const}, m_gnd_reset{gnd_reset} {}

void Integrator::parse(std::ifstream &file) {
    file >> m_integ_const;
    file >> m_gnd_reset;
}

void Integrator::claim_components() {
    claim_capacitors(2);
    claim_opamps(1);
    if (m_gnd_reset) {
        claim_comparator();
    }
}

void Integrator::finalize() {
    uint8_t num, den;
    approximate_ratio(m_integ_const / 4, num, den);

    OpAmp &_opamp = opamp(1);
    if (m_gnd_reset) {
        comp().set_configuration({ 0x07, 0xC9 });
        _opamp.set_feedback({ 0x6C, 0x05 });
    }

    cap(1).set_value(num)
          .set_in(Capacitor::from_input(in(), 1, Clock::B))
          .set_out(Capacitor::to_opamp(_opamp, 2, Clock::B));
    cap(2).set_value(den)
          .set_in(Capacitor::from_opamp(_opamp))
          .set_out(Capacitor::to_opamp(_opamp));
}

GainSwitch::GainSwitch()
        : AnalogModule{"GainSwitch", 2} {}

void GainSwitch::claim_components() {
    claim_capacitors(3);
    claim_opamps(1);
    claim_comparator();
}

void GainSwitch::finalize() {
    OpAmp &_opamp = opamp(1);
    comp().set_configuration({ 0x81, 0x05 });

    cap(1).set_value(255)
          .set_in(Capacitor::from_input(in(1)))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(2).set_value(255)
          .set_in(Capacitor::from_input(in(2)))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(3).set_value(255)
          .set_in(Capacitor::from_opamp(_opamp, 1))
          .set_out(Capacitor::to_opamp(_opamp));
}

SingleGainInv::SingleGainInv()
        : AnalogModule{"SingleGainInv", 1}, m_gain{} {}

SingleGainInv::SingleGainInv(double gain)
        : AnalogModule{"SingleGainInv", 1}, m_gain{gain} {}

void SingleGainInv::parse(std::ifstream &file) {
    file >> m_gain;
}

void SingleGainInv::claim_components() {
    claim_capacitors(2);
    claim_opamps(1);
}

void SingleGainInv::finalize() {
    uint8_t num, den;
    approximate_ratio(m_gain, num, den);

    OpAmp &_opamp = opamp(1);

    cap(1).set_value(num)
          .set_in(Capacitor::from_input(in(), 1))
          .set_out(Capacitor::to_opamp(_opamp, 2));
    cap(2).set_value(den)
          .set_in(Capacitor::from_opamp(_opamp, 1))
          .set_out(Capacitor::to_opamp(_opamp, 2));
}

SingleSumInv::SingleSumInv()
        : AnalogModule{"SingleSumInv", 2}, m_lgain{}, m_ugain{} {}

SingleSumInv::SingleSumInv(double lgain, double ugain)
        : AnalogModule{"SingleSumInv", 2}, m_lgain{lgain}, m_ugain{ugain} {}

void SingleSumInv::parse(std::ifstream &file) {
    file >> m_lgain;
    file >> m_ugain;
}

void SingleSumInv::claim_components() {
    claim_capacitors(3);
    claim_opamps(1);
}

void SingleSumInv::finalize() {
    std::vector<double> gains = { m_lgain, m_ugain };
    std::vector<uint8_t> nums;
    uint8_t den;
    approximate_ratios(gains, nums, den);

    OpAmp &_opamp = opamp(1);
    
    cap(1).set_value(nums[0])
          .set_in(Capacitor::from_input(in(1), 1))
          .set_out(Capacitor::to_opamp(_opamp, 2));
    cap(2).set_value(nums[1])
          .set_in(Capacitor::from_input(in(2), 1))
          .set_out(Capacitor::to_opamp(_opamp, 2));
    cap(3).set_value(den)
          .set_in(Capacitor::from_opamp(_opamp, 1))
          .set_out(Capacitor::to_opamp(_opamp, 2));
}
