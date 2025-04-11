#include "analog-module.hpp"
#include "util.hpp"
#include "analog-block.hpp"
#include "error.hpp"

AnalogModule::AnalogModule(std::string const &name)
        : m_cab{}, m_name{name} {}

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

void AnalogModule::set_cab(AnalogBlock &cab) {
    m_cab = &cab; 
}

InvGain::InvGain(double gain)
        : AnalogModule{"GainInv"}, m_gain{gain}, 
          m_in{*this}, m_out{*this} {}

void InvGain::configure() {
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
        : AnalogModule{"SumInv"}, m_lgain{lgain}, m_ugain{ugain}, 
          m_in_x{*this}, m_in_y{*this}, m_out{*this} {}

void InvSum::configure() {
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
