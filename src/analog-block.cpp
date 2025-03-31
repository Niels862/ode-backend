#include "analog-block.hpp"

AnalogBlock::AnalogBlock()
        : m_id{}, m_caps{}, m_opamps{} {}

AnalogBlock::AnalogBlock(int id)
        : m_id{id}, m_caps{}, m_opamps{}, m_modules{} {
    for (std::size_t i = 0; i < NCapacitorsPerBlock; i++) {
        m_caps[i] = Capacitor(i + 1);
    }

    for (std::size_t i = 0; i < NOpAmpsPerBlock; i++) {
        m_opamps[i] = OpAmp(i + 1);
    }
}

void AnalogBlock::compile(ShadowSRam &ssram) const {
    for (auto const &module : m_modules) {
        module->setup();
    }

    for (Capacitor const &cap : m_caps) {
        cap.compile(*this, ssram);
    }

    for (OpAmp const &opamp : m_opamps) {
        opamp.compile(*this, ssram);
    }
}
