#include "analog-block.hpp"
#include "analog-chip.hpp"
#include "settings.hpp"
#include "io-cell.hpp"
#include "error.hpp"
#include <vector>
#include <sstream>
#include <cassert>

AnalogBlock::AnalogBlock()
        : m_chip{}, m_id{}, m_set_up{false}, 
          m_local_ins{}, m_next_local_in{},
          m_caps{}, m_next_cap{}, 
          m_opamps{}, m_next_opamp{}, m_comp{*this},
          m_used_clocks{nullptr, nullptr}, 
          m_internal_P{}, m_internal_Q{},
          m_local_opamp_channels{}, 
          m_local_input_channels{}, 
          m_local_output_channels{}, m_modules{} {
    for (InputPort &in : m_local_ins) {
        in = InputPort(*this, InPortSource::Local);
    }

    for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
        local_opamp_channel(side) = Channel::IntraCab(side);
        local_input_channel(side) = Channel::LocalInput(side);
        local_output_channel(side) = Channel::LocalOutput(side);
    }
}

void AnalogBlock::initialize(int id, AnalogChip &chip) {
    m_chip = &chip;

    m_id = id;
    
    m_used_clocks[0] = &m_chip->null_clock();
    m_used_clocks[1] = &m_chip->null_clock();
    
    for (std::size_t i = 0; i < NCapacitorsPerBlock; i++) {
        m_caps[i] = Capacitor(i + 1);
    }

    for (std::size_t i = 0; i < NOpAmpsPerBlock; i++) {
        m_opamps[i] = OpAmp(*this, i + 1);
    }
}

void AnalogBlock::setup(Clock &clk_a, Clock &clk_b) {
    if (m_set_up) {
        std::stringstream ss;
        ss << "CAB" << m_id << ": already set up" << std::endl;
        throw DesignError(ss.str());
    }
    m_used_clocks[0] = &clk_a;
    m_used_clocks[1] = &clk_b;

    m_set_up = true;
}

InputPort &AnalogBlock::claim_in(AnalogModule &) {
    if (m_next_local_in >= m_local_ins.size()) {
        std::stringstream ss;
        ss << "CAB" << m_id << ": cannot claim input port";
        throw DesignError(ss.str());
    }

    InputPort &in = m_local_ins[m_next_local_in];
    m_next_local_in++;
    return in;
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

static Channel::Side source_to_side(OutPortSource source) {
    assert(source == OutPortSource::OpAmp1 || source == OutPortSource::OpAmp2);
    if (source == OutPortSource::OpAmp1) {
        return Channel::Primary;
    }
    return Channel::Secondary;
}

static void allocate_intercab_channel(AnalogBlock &cab, PortLink &link) {
    InputPort &in = *link.in;
    OutputPort &out = *link.out;

    assert(in.source() == InPortSource::Local 
           || in.source() == InPortSource::Comparator);

    Channel::Side side = source_to_side(out.source());
    cab.chip().intercam_channel(out.cab(), in.cab(), side).allocate(link);
}

void allocate_local_opamp_channel(AnalogBlock &cab, PortLink &link) {
    InputPort &in = *link.in;
    OutputPort &out = *link.out;

    assert(in.source() == InPortSource::Local 
           || in.source() == InPortSource::Comparator);

    Channel::Side side = source_to_side(out.source());
    cab.local_opamp_channel(side).allocate(link);
}

void allocate_external_loopback_channels(AnalogBlock &cab, PortLink &link) {
    InputPort &in = *link.in;
    OutputPort &out = *link.out;

    assert(in.source() == InPortSource::Local 
           || in.source() == InPortSource::Comparator);

    Channel::Side side = source_to_side(out.source());
    CabGroup group = Channel::to_cab_group(cab);

    /* OpAmpX -> OutputX -> GlobalY -> InputZ */
    Channel *output = &cab.local_output_channel(side).allocate(link);

    Channel *global{nullptr};
    for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
        Channel &channel = cab.chip().global_bi_indirect(group, side);
        if (channel.available(link)) {
            global = &channel;
            break;
        }
    }

    if (global) {
        global->allocate(link);
    } else {
        throw DesignError("cannot route");
    }

    Channel *input{nullptr};
    for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
        Channel &channel = cab.local_input_channel(side);
        if (channel.available(link)) {
            input = &channel;
            break;
        }
    }

    if (input) {
        input->allocate(link);
    } else {
        throw DesignError("cannot route");
    }

    input->set_local_input_source(*global);
    output->set_local_output_dest(*global);
}

void AnalogBlock::finalize() {
    for (InputPort &in : m_local_ins) {
        if (!in.connected()) {
            continue;
        }

        PortLink &link = *in.link();
        OutputPort &out = *link.out;

        switch (out.source()) {
            case OutPortSource::None:
                break;
                
            case OutPortSource::IOCell:
                break;

            case OutPortSource::OpAmp1:
            case OutPortSource::OpAmp2:
                if (out.cab() == in.cab()) {
                    if (m_id == 3) {
                        allocate_external_loopback_channels(*this, link);
                    } else {
                        allocate_local_opamp_channel(*this, link);
                    }
                } else {
                    allocate_intercab_channel(*this, link);
                }
                break;
        }

        std::cerr << link << std::endl;
    }

    if (args.verbose) {
        log_resources();
    }
    for (auto const &module : m_modules) {
        if (args.verbose) {
            std::cerr << module->name() << ":" << std::endl;
        }
        module->finalize();
    }
}

int find_connection_channel(OpAmp &opamp1, OpAmp &opamp2, 
                                   Connection::Channel channel) {
    bool is1 = false, is2 = false;
    
    if (opamp1.is_used()) {
        for (PortLink *link : opamp1.out().links()) {
            if (link->in->source() == InPortSource::IOCell) {
                Connection &conn = link->in->io_cell().connection(opamp1.out().cab());
                if (conn.channel == channel && conn.mode == Connection::Far) {
                    is1 = true;
                }
            }
        }
    }

    if (opamp2.is_used()) {
        for (PortLink *link : opamp2.out().links()) {
            if (link->in->source() == InPortSource::IOCell) {
                Connection &conn = link->in->io_cell().connection(opamp2.out().cab());
                if (conn.channel == channel && conn.mode == Connection::Far) {
                    is2 = true;
                }
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
uint8_t compile_local_output_routing(OpAmp &opamp1, OpAmp &opamp2) {
    int pri = find_connection_channel(opamp1, opamp2, Connection::Primary);
    int sec = find_connection_channel(opamp1, opamp2, Connection::Secondary);

    if (sec) {
        sec = (sec == 2 ? 1 : 2);
    }

    return pri | (sec << 4);
}

/*
The CAB has two internal routing channels, P and Q.
In the local routing configuration, external channels can be mapped 
to the channels:

Byte 05 controls the signal to Q, byte 04 controls the signal to P,
possibly multiplexer values. In the CAB module routing, these channels
can be referenced using the defined values. 

Constraints: 
- Capacitor switches can select either P or Q.
- The single comparator is *only able* to use the Q channel, as the 
  configuration to P is unknown / may not exist.

These channels are used for `far` connections to IO-Cells. Whether a 
connection is `near` or `far` is determind elsewhere and can be tested with 
conn.channel.

Approach:
- Check if comparator is in use and if its connnection is to a far IO-Cell. 
  If so, reserve Q for the signal from this IO-Cell. It may still be used by 
  switches as well. 
- Iterate over each switch connection to the CAB. If a connection is to a far 
  IO-Cell, first check if it is to an existing mapping to P or Q. If not, 
  add a mapping. If both P and Q are mapped and both do not match, throw a 
  RoutingError.  
*/
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

    m_comp.compile(*this, ssram);

    /*
    map_internal_channels();
    ssram.set(bank_b(), 0x05, compile_internal_channel_routing(m_internal_P));
    ssram.set(bank_b(), 0x04, compile_internal_channel_routing(m_internal_Q));
    
    uint8_t b = compile_local_output_routing(m_opamps[0], m_opamps[1]);
    ssram.set(bank_b(), 0x02, b);
    */

    for (std::size_t i = 0; i < 2; i++) {
        Channel &channel = m_local_input_channels[i];
        uint8_t select = channel.local_input_source_selector();
        ssram.set(bank_b(), 0x05 - i, select);
    }

    uint8_t select = 0x0;
    for (std::size_t i = 0; i < 2; i++) {
        Channel &channel = m_local_output_channels[i];
        select |= channel.local_output_dest_selector();
    }
    ssram.set(bank_b(), 0x02, select);

    ssram.set(bank_b(), 0x0, from_nibbles(m_used_clocks[1]->id_nibble(), 
                                          m_used_clocks[0]->id_nibble()));
}

void AnalogBlock::set_used_clock(int i, Clock &clock) {
    m_used_clocks[i] = &clock;
}

Channel &AnalogBlock::local_opamp_channel(Channel::Side side) {
    return m_local_opamp_channels.at(static_cast<int>(side));
}

Channel &AnalogBlock::local_input_channel(Channel::Side side) {
    return m_local_input_channels.at(static_cast<int>(side));
}

Channel &AnalogBlock::local_output_channel(Channel::Side side) {
    return m_local_output_channels.at(static_cast<int>(side));
}

void AnalogBlock::log_resources() const {
    std::cerr << m_next_cap << " / " 
              << NCapacitorsPerBlock << " Capacitors, " 
              << m_next_opamp << " / " << NOpAmpsPerBlock << " OpAmps, " 
              << m_comp.is_used() << " / 1 Comparators" << std::endl;
}

void AnalogBlock::map_internal_channels() {
    IOCell *comp_io = m_comp.in().io_connection();
    if (comp_io) {
        Connection &conn = comp_io->connection(*this);
        if (conn.mode == Connection::Far) {
            conn.internal = Connection::Internal::Q;
            m_internal_Q = comp_io;
        }
    }

    for (InputPort &in : m_local_ins) {
        IOCell *in_io = in.io_connection();
        if (!in_io) {
            continue;
        }
        
        Connection &conn = in_io->connection(*this);
        if (conn.mode != Connection::Far) {
            continue;
        }

        if (m_internal_P == in_io) {
            conn.internal = Connection::Internal::P;
        } else if (m_internal_Q == in_io) {
            conn.internal = Connection::Internal::Q;
        } else if (m_internal_P == nullptr) {
            conn.internal = Connection::Internal::P;
            m_internal_P = in_io;
        } else if (m_internal_Q == nullptr) {
            conn.internal = Connection::Internal::Q;
            m_internal_Q = in_io;
        } else {
            throw DesignError(
                    "Cannot realize routing: out of internal channels");
        }
    }
}

uint8_t AnalogBlock::compile_internal_channel_routing(IOCell *channel) {
    if (channel == nullptr) {
        return 0x00;
    }

    Connection &conn = channel->connection(*this);
    if (conn.channel == Connection::Primary) {
        return 0x01;
    }
    return 0x02;
}
