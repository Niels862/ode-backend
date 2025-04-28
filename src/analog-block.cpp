#include "analog-block.hpp"
#include "settings.hpp"
#include "io-cell.hpp"
#include "error.hpp"
#include <vector>
#include <sstream>

AnalogBlock::AnalogBlock()
        : m_id{}, m_caps{}, m_next_cap{}, 
          m_opamps{}, m_next_opamp{},
          m_used_clocks{}, m_modules{} {}

AnalogBlock::AnalogBlock(int id, Clock &pri_clock, Clock &sec_clock)
        : m_id{id}, m_caps{}, m_next_cap{}, 
          m_opamps{}, m_next_opamp{}, 
          m_used_clocks{&pri_clock, &sec_clock}, m_modules{} {
    for (std::size_t i = 0; i < NCapacitorsPerBlock; i++) {
        m_caps[i] = Capacitor(i + 1);
    }

    for (std::size_t i = 0; i < NOpAmpsPerBlock; i++) {
        m_opamps[i] = OpAmp(i + 1);
    }
}

Capacitor &AnalogBlock::claim_cap(int value) {
    if (m_next_cap >= m_caps.size()) {
        std::stringstream ss;
        ss << "CAB" << m_id << ": cannot claim capactitor";
        throw DesignError(ss.str());
    }

    Capacitor &cap = m_caps[m_next_cap];
    m_next_cap++;
    return cap.claim(value);
}

OpAmp &AnalogBlock::claim_opamp(bool closed_loop) {
    if (m_next_opamp >= m_opamps.size()) {
        std::stringstream ss;
        ss << "CAB" << m_id << ": cannot claim opamp";
        throw DesignError(ss.str());
    }

    OpAmp &opamp = m_opamps[m_next_opamp];
    m_next_opamp++;
    return opamp.claim(closed_loop);
}

Comparator &AnalogBlock::claim_comp() {
    return m_comp.claim();
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
    /* Enable Clocks */
    for (Clock *clock : m_used_clocks) {
        clock->set_is_used(true);
    }

    /* Compile each Capacitor: values and switches */
    for (Capacitor const &cap : m_caps) {
        cap.compile(*this, ssram);
    }

    /* Compile each OpAmp: switches */
    for (OpAmp const &opamp : m_opamps) {
        opamp.compile(*this, ssram);
    }

    /* Compile Comparator: unknown setup values */
    m_comp.compile(*this, ssram);


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
    ssram.set(bank_b(), 0x0, 
              from_nibbles(m_used_clocks[1]->id_nibble(), 
                           m_used_clocks[0]->id_nibble()));
}

void AnalogBlock::set_used_clock(int i, Clock &clock) {
    m_used_clocks[i] = &clock;
}
