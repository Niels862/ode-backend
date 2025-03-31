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
    OpAmp &opamp = cab().opamp(1).claim(true);

    cab().cap(1).claim(0)
                .set_in(Capacitor::from_input(m_in))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(2).claim(0)
                .set_in(Capacitor::from_input(m_in, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
    cab().cap(3).claim(0)
                .set_in(Capacitor::from_opamp(opamp))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(4).claim(0)
                .set_in(Capacitor::from_opamp(opamp, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
}

InvSum::InvSum(double lgain, double ugain)
        : m_lgain{lgain}, m_ugain{ugain}, 
          m_in_x{*this}, m_in_y{*this}, m_out{*this} {}

void InvSum::setup() {
    GainEncodingTriple triple 
            = compute_gain_encoding(m_lgain, m_ugain);

    OpAmp &opamp = cab().opamp(1).claim(true);

    cab().cap(1).claim(triple.C_1)
                .set_in(Capacitor::from_input(m_in_x))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(2).claim(triple.C_1)
                .set_in(Capacitor::from_input(m_in_x, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
    cab().cap(3).claim(triple.C_2)
                .set_in(Capacitor::from_input(m_in_y))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(4).claim(triple.C_2)
                .set_in(Capacitor::from_input(m_in_y, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
    cab().cap(5).claim(triple.C_out)
                .set_in(Capacitor::from_opamp(opamp))
                .set_out(Capacitor::to_opamp(opamp));
    cab().cap(6).claim(triple.C_out)
                .set_in(Capacitor::from_opamp(opamp, 1))
                .set_out(Capacitor::to_opamp(opamp, 1));
}
