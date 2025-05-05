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

void AnalogBlock::setup(Clock &clk_a, Clock &clk_b) {
    m_used_clocks[0] = &clk_a;
    m_used_clocks[1] = &clk_b;
}

Capacitor &AnalogBlock::claim_cap(AnalogModule &module) {
    if (m_next_cap >= m_caps.size()) {
        std::stringstream ss;
        ss << "CAB" << m_id << ": cannot claim capactitor";
        throw DesignError(ss.str());
    }

    Capacitor &cap = m_caps[m_next_cap];
    m_next_cap++;
    return cap.claim(module);
}

OpAmp &AnalogBlock::claim_opamp(AnalogModule &module) {
    if (m_next_opamp >= m_opamps.size()) {
        std::stringstream ss;
        ss << "CAB" << m_id << ": cannot claim opamp";
        throw DesignError(ss.str());
    }

    OpAmp &opamp = m_opamps[m_next_opamp];
    m_next_opamp++;
    return opamp.claim(module);
}

Comparator &AnalogBlock::claim_comp(AnalogModule &module) {
    return m_comp.claim(module);
}

void AnalogBlock::finalize() {
    log_resources();
    for (auto const &module : m_modules) {
        if (args.verbose) {
            std::cerr << module->name() << ":" << std::endl;
        }
        module->finalize();
    }
}

static int find_connection_channel(OpAmp &opamp1, OpAmp &opamp2, 
                                   Connection::Channel channel) {
    bool is1 = false, is2 = false;
    
    if (opamp1.is_used()) {
        for (auto &port : opamp1.out().connections()) {
            if (auto *cell = dynamic_cast<IOCell *>(&port->module())) {
                Connection &conn = cell->connection(opamp1.out().module().cab());
                is1 = conn.channel == channel;
            }
        }
    }

    if (opamp2.is_used()) {
        for (auto &port : opamp2.out().connections()) {
            if (auto *cell = dynamic_cast<IOCell *>(&port->module())) {
                Connection &conn = cell->connection(opamp2.out().module().cab());
                is2 = conn.channel == channel;
            }
        }
    }
    
    if (is1 && is2) {
        abort();
    }

    if (is1) return 1;
    if (is2) return 2;
    return 0;
}

/* FIXME: unreliable, needs more research */
static uint8_t compile_local_output_routing(OpAmp &opamp1, OpAmp &opamp2) {
    int pri = find_connection_channel(opamp1, opamp2, Connection::Primary);
    int sec = find_connection_channel(opamp1, opamp2, Connection::Secondary);

    if (sec) {
        sec = sec == 2 ? 1 : 2;
    }

    return pri | (sec << 4);
}

void AnalogBlock::compile(ShadowSRam &ssram) {
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

    uint8_t b = compile_local_output_routing(m_opamps[0], m_opamps[1]);
    ssram.set(bank_b(), 0x02, b);

    ssram.set(bank_b(), 0x0, from_nibbles(m_used_clocks[1]->id_nibble(), 
                                          m_used_clocks[0]->id_nibble()));
}

void AnalogBlock::set_used_clock(int i, Clock &clock) {
    m_used_clocks[i] = &clock;
}

void AnalogBlock::log_resources() const {
    std::cerr << m_next_cap << " / " 
              << NCapacitorsPerBlock << " Capacitors, " 
              << m_next_opamp << " / " << NOpAmpsPerBlock << " OpAmps, " 
              << m_comp.is_used() << " / 1 Comparators" << std::endl;
}
