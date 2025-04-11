#include "analog-block.hpp"
#include "settings.hpp"

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

void AnalogBlock::configure() {
    for (auto const &module : m_modules) {
        if (args.verbose) {
            std::cerr << module->name() << ":" << std::endl;
        }
        module->configure();
    }
}

void AnalogBlock::compile(ShadowSRam &ssram) const {
    for (Capacitor const &cap : m_caps) {
        cap.compile(*this, ssram);
    }

    for (OpAmp const &opamp : m_opamps) {
        opamp.compile(*this, ssram);
    }

    ssram.set(bank_b(), 0x0, m_modules.empty() ? 0x00 : 0x0C);
}
