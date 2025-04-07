#include "analog-module.hpp"
#include "util.hpp"
#include "analog-block.hpp"

AnalogModule::AnalogModule()
        : m_cab{} {}

void AnalogModule::set_cab(AnalogBlock &cab) {
    m_cab = &cab; 
}

InvGain::InvGain(double gain)
        : m_gain{gain}, 
          m_in{*this}, m_out{*this} {}

void InvGain::setup() {
    uint8_t num, den;
    approximate_ratio(m_gain, num, den);

    OpAmp &opamp = cab().opamp(1).claim(true);

    cab().cap(1).claim(num)
                .set_in(Capacitor::from_input(m_in))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(2).claim(num)
                .set_in(Capacitor::from_input(m_in, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
    cab().cap(3).claim(den)
                .set_in(Capacitor::from_opamp(opamp))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(4).claim(den)
                .set_in(Capacitor::from_opamp(opamp, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
}

InvSum::InvSum(double lgain, double ugain)
        : m_lgain{lgain}, m_ugain{ugain}, 
          m_in_x{*this}, m_in_y{*this}, m_out{*this} {}

void InvSum::setup() {
    std::vector<double> gains = { m_lgain, m_ugain };
    std::vector<uint8_t> nums;
    uint8_t den;
    approximate_ratios(gains, nums, den);

    OpAmp &opamp = cab().opamp(1).claim(true);

    cab().cap(1).claim(nums[0])
                .set_in(Capacitor::from_input(m_in_x))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(2).claim(nums[0])
                .set_in(Capacitor::from_input(m_in_x, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
    cab().cap(3).claim(nums[1])
                .set_in(Capacitor::from_input(m_in_y))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(4).claim(nums[1])
                .set_in(Capacitor::from_input(m_in_y, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
    cab().cap(5).claim(den)
                .set_in(Capacitor::from_opamp(opamp))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(6).claim(den)
                .set_in(Capacitor::from_opamp(opamp, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
}
