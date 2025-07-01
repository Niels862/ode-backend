#include "analog-module.hpp"
#include "util.hpp"
#include "analog-block.hpp"
#include "error.hpp"
#include <sstream>

AnalogModule::AnalogModule(std::string const &name)
        : m_cab{}, m_name{name},
          m_ins{}, m_caps{}, m_opamps{}, m_comp{}, 
          m_curr_cap{0}, m_n_ins{0} {}

AnalogModule *AnalogModule::Build(std::string_view const &name) {
    if (name == "GainInv")              return new GainInv();
    if (name == "SumInv")               return new SumInv();
    if (name == "Integrator")           return new Integrator();
    if (name == "GainSwitch")           return new GainSwitch();
    if (name == "SampleAndHold")        return new SampleAndHold();
    return nullptr;
}

InputPort &AnalogModule::in(std::size_t i) {
    if (i == 0 && m_n_ins == 1) {
        return in(1);
    }

    if (i < 1 || i > m_n_ins) {
        std::stringstream ss;
        ss << "`" << m_name << "` does not implement in(" << i << ")";
        throw DesignError(ss.str());
    }

    return m_ins[i - 1];
}

OutputPort &AnalogModule::out(std::size_t i) {
    if (i == 0 && !m_opamps[1]) {
        return out(1);
    }
    
    return opamp(i).out();
}

Capacitor &AnalogModule::cap(int i) {
    if (i == 0) {
        m_curr_cap++;
        return cap(m_curr_cap);
    }

    Capacitor *cap = m_caps.at(i - 1);
    if (cap == nullptr) {
        std::stringstream ss;
        ss << "Capacitor #" << i << " was not claimed by " << m_name;
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

void AnalogModule::claim_capacitors(std::size_t n) {
    for (size_t i = 0; i < n; i++) {
        m_caps[i] = &m_cab->claim_cap(*this); 
    }
}

void AnalogModule::claim_opamps(std::size_t n) {
    for (size_t i = 0; i < n; i++) {
        m_opamps[i] = &m_cab->claim_opamp(*this); 
    }
}

void AnalogModule::claim_comparator() {
    m_comp = &m_cab->claim_comp(*this);
}

void AnalogModule::claim_inputs(std::size_t n) {
    m_n_ins = n;
    for (std::size_t i = 0; i < n; i++) {
        m_ins[i] = InputPort(cab(), InPortSource::Local);
    }
}

GainInv::GainInv()
        : AnalogModule{"GainInv"}, m_gain{1.0} {}

GainInv::GainInv(double gain)
        : AnalogModule{"GainInv"}, m_gain{gain} {}

bool GainInv::set_parameter(std::string_view param, Parameter value) {
    if (param == "gain") {
        m_gain = value.as_double();
    } else {
        return false;
    }
    return true;
}

void GainInv::claim_components() {
    claim_capacitors(4);
    claim_opamps(1);
    claim_inputs(1);
}

void GainInv::finalize() {
    uint8_t num, den;
    approximate_ratio(m_gain, num, den);

    OpAmp &_opamp = opamp(1);

    cap().set_value(num)
         .set_in(Capacitor::from_input(in()))
         .set_out(Capacitor::to_opamp(_opamp));
    cap().set_value(num)
         .set_in(Capacitor::from_input(in(), 1))
         .set_out(Capacitor::to_opamp(_opamp, 1));
    cap().set_value(den)
         .set_in(Capacitor::from_opamp(_opamp))
         .set_out(Capacitor::to_opamp(_opamp));
    cap().set_value(den)
         .set_in(Capacitor::from_opamp(_opamp, 1))
         .set_out(Capacitor::to_opamp(_opamp, 1));
}

SumInv::SumInv()
        : AnalogModule{"SumInv"}, m_gains{{1.0, 1.0, 1.0}},
          m_n_inputs{2} {}

SumInv::SumInv(double gain1, double gain2, std::size_t n_inputs)
        : AnalogModule{"SumInv"}, m_gains{{gain1, gain2, 1.0}},
          m_n_inputs{n_inputs} {}

bool SumInv::set_parameter(std::string_view param, Parameter value) {
    if (param == "gain1") {
        m_gains[0] = value.as_double();
    } else if (param == "gain2") {
        m_gains[1] = value.as_double();
    } else if (param == "gain3") {
        m_gains[2] = value.as_double();
    } else if (param == "inputs") {
        m_n_inputs = value.as_int();
    } else {
        return false;
    }
    return true;
}

void SumInv::claim_components() {
    claim_capacitors(2 + 2 * m_n_inputs);
    claim_opamps(1);
    claim_inputs(m_n_inputs);
}

void SumInv::finalize() {
    std::vector<double> gains(3);
    for (std::size_t i = 0; i < m_n_inputs; i++) {
        gains[i] = m_gains[i];
    }
    std::vector<uint8_t> nums;
    uint8_t den;
    approximate_ratios(gains, nums, den);

    OpAmp &_opamp = opamp(1);

    for (std::size_t i = 0; i < m_n_inputs; i++) {
        cap().set_value(nums[i])
             .set_in(Capacitor::from_input(in(i + 1)))
             .set_out(Capacitor::to_opamp(_opamp));
        cap().set_value(nums[i])
             .set_in(Capacitor::from_input(in(i + 1), 1))
             .set_out(Capacitor::to_opamp(_opamp, 1));
    }
    cap().set_value(den)
         .set_in(Capacitor::from_opamp(_opamp))
         .set_out(Capacitor::to_opamp(_opamp));
    cap().set_value(den)
         .set_in(Capacitor::from_opamp(_opamp, 1))
         .set_out(Capacitor::to_opamp(_opamp, 1));
}

Integrator::Integrator()
        : AnalogModule("Integrator"), 
          m_integ_const{}, m_gnd_reset{} {}

Integrator::Integrator(double integ_const, bool gnd_reset)
        : AnalogModule{"Integrator"}, 
          m_integ_const{integ_const}, m_gnd_reset{gnd_reset} {}

bool Integrator::set_parameter(std::string_view param, Parameter value) {
    if (param == "integ_const") {
        m_integ_const = value.as_double();
    } else if (param == "reset") {
        m_gnd_reset = value.as_bool();
    } else {
        return false;
    }
    return true;
}

void Integrator::claim_components() {
    claim_capacitors(2);
    claim_opamps(1);
    if (m_gnd_reset) {
        claim_comparator();
    }
    claim_inputs(1);
}

void Integrator::finalize() {
    uint8_t num, den;
    approximate_ratio(m_integ_const / 4, num, den);

    OpAmp &_opamp = opamp(1);

    Clock::Select clk = Clock::A;
    if (m_gnd_reset) {
        comp().set_configuration({ 0x07, 0xC9 });
        _opamp.set_feedback({ 0x6C, 0x05 });
        clk = Clock::B;
    }

    cap(1).set_value(num)
          .set_in(Capacitor::from_input(in(), 1, clk))
          .set_out(Capacitor::to_opamp(_opamp, 2, clk));
    cap(2).set_value(den)
          .set_in(Capacitor::from_opamp(_opamp))
          .set_out(Capacitor::to_opamp(_opamp));
}

GainSwitch::GainSwitch()
        : AnalogModule{"GainSwitch"} {}

bool GainSwitch::set_parameter(std::string_view, Parameter) {
    return false;
}

void GainSwitch::claim_components() {
    claim_capacitors(3);
    claim_opamps(1);
    claim_comparator();
    claim_inputs(2);
}

#define TEMP_OPAMP_FEEDBACK_SWITCHING { 0x81, 0x05 }

#define TEMP_CAP_IN_SWITCHED_IF_COMP_GT(p) { 0x3D, (uint8_t)(((p.input_connection_selector()) << 4) | 0x1) }
#define TEMP_CAP_IN_SWITCHED_IF_COMP_LT(p) { 0x2D, (uint8_t)(((p.input_connection_selector()) << 4) | 0x1) }

void GainSwitch::finalize() {
    OpAmp &_opamp = opamp(1).set_feedback(TEMP_OPAMP_FEEDBACK_SWITCHING);
    comp().set_configuration({ 0x07, 0x01 });

    cap(1).set_value(255)
          .set_in(TEMP_CAP_IN_SWITCHED_IF_COMP_GT(in(1)))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(2).set_value(255)
          .set_in(TEMP_CAP_IN_SWITCHED_IF_COMP_LT(in(2)))
          .set_out(Capacitor::to_opamp(_opamp));
    cap(3).set_value(255)
          .set_in(Capacitor::from_opamp(_opamp, 1))
          .set_out(Capacitor::to_opamp(_opamp));
}

SampleAndHold::SampleAndHold()
        : AnalogModule{"SampleAndHold"} {}

bool SampleAndHold::set_parameter(std::string_view, Parameter) {
    return false;
}

void SampleAndHold::claim_components() {
    claim_capacitors(2);
    claim_opamps(1);
    claim_inputs(1);
}

void SampleAndHold::finalize() {
    OpAmp &_opamp = opamp(1);

    cap(1).set_value(255)
          .set_in(Capacitor::switched_in(_opamp, in(1)))
          .set_out(Capacitor::to_opamp(_opamp, 2));
    cap(2).set_value(255)
          .set_in(Capacitor::from_opamp(_opamp))
          .set_out(Capacitor::to_opamp(_opamp, 1));
}
