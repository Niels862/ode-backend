#include "analog-block.hpp"
#include "settings.hpp"
#include "io-cell.hpp"
#include <vector>

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

    bool use_far_pri = false, use_far_sec = false;
    for (auto &module : m_modules) {
        for (auto &in : module->ins()) {
            OutputPort *port = in.connection();
            if (!port) {
                continue;
            }

            if (auto *cell = dynamic_cast<IOCell *>(&port->module())) {
                Connection &conn = cell->connection(*this);

                if (conn.mode == Connection::Far) {
                    if (conn.channel == Connection::Primary) {
                        use_far_pri = true;
                    }
                    if (conn.channel == Connection::Secondary) {
                        use_far_sec = true;
                    }
                }
            }
        }
    }

    uint8_t b05 = 0x00, b04 = 0x00;
    if (use_far_pri && use_far_sec) {
        b05 = 0x01; // both present
        b04 = 0x02;
    } else if (use_far_pri) {
        b05 = 0x01;
    } else if (use_far_sec) {
        b05 = 0x02;
    }

    ssram.set(bank_b(), 0x05, b05);
    ssram.set(bank_b(), 0x04, b04);
    ssram.set(bank_b(), 0x0, m_modules.empty() ? 0x00 : 0x0C);
}
