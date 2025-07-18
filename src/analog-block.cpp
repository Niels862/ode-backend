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

void AnalogBlock::finalize_comparator() {
    m_comp.finalize();
}

static void allocate_intercab_channel(AnalogBlock &cab, PortLink &link) {
    InputPort &in = *link.in;
    OutputPort &out = *link.out;

    assert(in.source() == InPortSource::Local 
           || in.source() == InPortSource::Comparator);

    Channel::Side side = Channel::source_to_side(out.source());
    cab.chip().intercam_channel(out.cab(), in.cab(), side).allocate(link);
}

static void allocate_local_opamp_channel(AnalogBlock &cab, PortLink &link) {
    InputPort &in = *link.in;
    OutputPort &out = *link.out;

    assert(in.source() == InPortSource::Local 
           || in.source() == InPortSource::Comparator);

    Channel::Side side = Channel::source_to_side(out.source());
    cab.local_opamp_channel(side).allocate(link);
}

static void allocate_external_loopback_channels(AnalogBlock &cab, PortLink &link) {
    InputPort &in = *link.in;
    OutputPort &out = *link.out;

    assert(in.source() == InPortSource::Local 
           || in.source() == InPortSource::Comparator);

    Channel::Side side = Channel::source_to_side(out.source());
    CabColumn group = Channel::to_cab_column(cab);

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

uint8_t local_output_reroute_selector(AnalogBlock &cab) {
    Channel *local_to_bi_pri = nullptr, *local_to_bi_sec = nullptr;

    for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
        Channel &channel = cab.local_output_channel(side);
        
        Channel *reroute = channel.local_output_dest();
        if (!reroute) {
            continue;
        }

        switch (reroute->side) {
            case Channel::Primary:
                local_to_bi_pri = &channel;
                break;

            case Channel::Secondary:
                local_to_bi_sec = &channel;
                break;
        }
    }
    
    if (local_to_bi_pri) {
        if (!local_to_bi_sec) {
            switch (local_to_bi_pri->side) {
                case Channel::Primary:      return 0x01;
                case Channel::Secondary:    return 0x02;
            }
        } else {
            return 0x11;
        }
    } else {
        if (!local_to_bi_sec) {
            return 0x00;
        } else {
            switch (local_to_bi_sec->side) {
                case Channel::Primary:      return 0x08;
                case Channel::Secondary:    return 0x10;
            }
        }
    }

    return 0x00;
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

    m_comp.compile(*this, ssram);

    for (std::size_t i = 0; i < 2; i++) {
        Channel &channel = m_local_input_channels[i];
        uint8_t select = channel.local_input_source_selector();
        ssram.set(bank_b(), 0x05 - i, select);
    }
    
    ssram.set(bank_b(), 0x02, local_output_reroute_selector(*this));

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
