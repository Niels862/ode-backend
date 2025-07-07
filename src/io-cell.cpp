#include "io-cell.hpp"
#include "error.hpp"
#include "analog-block.hpp"
#include "analog-chip.hpp"
#include <cassert>

void Connection::reset() {
    this->block = Connection::None;
    this->mode = Connection::Near;
    this->channel = Connection::Primary;
    this->cab = nullptr;
    this->internal = Connection::Internal::Unset;
}

void Connection::initialize(AnalogBlock &cab, Block block) {
    this->cab = &cab;
    this->block = block;
}

uint8_t Connection::io_nibble() const {
    if (mode == Connection::Far) {
        if (channel == Connection::Primary) {
            return 0x5;
        }
        return 0x6;
    }

    if (block == Connection::ToInput) {
        if (channel == Connection::Primary) {
            return 0x1;
        }
        return 0x2;
    }

    if (block == Connection::FromOutput) {
        if (channel == Connection::Primary) {
            return 0xC;
        }
        return 0x8;
    }

    assert(1);
    return 0x0;
}

uint8_t Connection::cab_nibble(IOCell &cell_from) const { 
    return 0x0;
    
    int from_id = cell_from.id();
    int to_id = cab->id();

    uint8_t n = 0x0;

    switch (from_id) {
        case 1:
        case 2:
            switch (to_id) {
                case 1:
                case 2: n = 0x9; break;
                case 3:
                case 4: n = 0x7; break;
            }
            break;

        case 3:
        case 4:
            switch (to_id) {
                case 1: 
                case 2: n = 0x7; break;
                case 3: n = 0xD; break;
                case 4: n = 0xF; break;
            }
            break;
    }

    if (n == 0x0) {
        throw DesignError("Unreached todo");
    }

    if (channel == Connection::Primary) {
        n = n;
    } else {
        n = n - 1;
    }

    return n;
}

bool Connection::equivalent(Connection const &other) const {
    if (this->channel != other.channel) {
        return false;
    }
    
    if (this->mode == Connection::Far && other.mode == Connection::Far) {
        return true;
    }

    if (this->block == other.block) {
        return true;
    }

    return false;
}

IOCell::IOCell() /* IOCell manages its own in() and out() port */
        : AnalogModule{"IOCell"}, m_chip{}, m_id{}, 
          m_mode{IOMode::Disabled},
          m_in{*this}, m_out{*this}, 
          m_conns{}, m_used_channels{{nullptr, nullptr}} {}

void IOCell::initialize(int id, AnalogBlock &cab) {
    set_cab(cab);
    m_chip = &cab.chip();
    m_id = id;
    m_mode = IOMode::Disabled;

    for (Connection &conn : m_conns) {
        conn.reset();
    }
}

static void finalize_input(IOCell &cell) {
    assert(cell.mode() == IOMode::InputBypass);

    IOGroup group = Channel::to_io_group(cell);

    bool use_indirect[2] = { false, false };
    for (PortLink *link : cell.out().links()) {
        AnalogBlock &cab = link->in->cab();
        CabColumn cab_group = Channel::to_cab_column(cab);
        bool direct = Channel::uses_direct_channel(cell, cab);

        std::size_t i = static_cast<int>(cab_group);
        use_indirect[i] = use_indirect[i] || !direct;
    }

    for (PortLink *link : cell.out().links()) {
        AnalogBlock &cab = link->in->cab();
        CabColumn cab_group = Channel::to_cab_column(cab);
        bool direct = !use_indirect[static_cast<int>(cab_group)];

        Channel *input = nullptr;
        if (direct) {
            input = &Channel::find_available(
                [&](Channel::Side side) -> Channel &{
                    return cell.chip().global_input_direct(group, cab, side);
                },
                link
            );
        } else {
            input = &Channel::find_available(
                [&](Channel::Side side) -> Channel &{
                    return cell.chip().global_bi_indirect(cab_group, side);
                },
                link
            );
        }

        if (!input) {
            throw DesignError("could not route");
        }

        Channel *local = nullptr;
        if (input->type == Channel::Type::GlobalBiIndirect) {
            local = &Channel::find_available(
                [&](Channel::Side side) -> Channel &{
                    return cab.local_input_channel(side);
                },
                link
            );

            if (!local) {
                throw DesignError("could not route");
            }
        }

        input->allocate(*link);
        if (local) {
            local->allocate(*link);
            local->set_local_input_source(*input);
        }

        cell.set_used_channel(cab_group, *input);
    }
}

static void finalize_output(IOCell &cell) {
    assert(cell.mode() == IOMode::OutputBypass);

    PortLink *link = cell.in().link();
    if (!link) {
        return;
    }

    IOGroup group = Channel::to_io_group(cell);
    AnalogBlock &cab = link->out->cab();
    CabColumn cab_group = Channel::to_cab_column(cab);
    bool direct = Channel::uses_direct_channel(cell, cab);
    Channel::Side side = Channel::source_to_side(link->out->source());

    Channel *output = nullptr;
    if (direct) {
        output = &cell.chip().global_output_direct(group, cab, side);
    } else {
        output = &Channel::find_available(
            [&](Channel::Side side) -> Channel &{
                return cell.chip().global_bi_indirect(cab_group, side);
            },
            link
        );
    }

    if (!output) {
        throw DesignError("could not route");
    }

    Channel *local = nullptr;
    if (output->type == Channel::Type::GlobalBiIndirect) {
        local = &cab.local_output_channel(side).allocate(*link);
    }

    output->allocate(*link);
    if (local) {
        local->set_local_output_dest(*output);
    }

    cell.set_used_channel(cab_group, *output);
}

void IOCell::finalize() {
    switch (m_mode) {
        case IOMode::Disabled:
            break;

        case IOMode::InputBypass:
            finalize_input(*this);
            break;

        case IOMode::OutputBypass:
            finalize_output(*this);
            break;
    }
}

#if 0
    for (Connection &conn : m_conns) {
        conn.reset();
    }

    if (m_mode == IOMode::InputBypass) {
        for (PortLink *link : m_out.links()) {
            AnalogBlock &cab = link->in->cab();

            if (link->in->source() == InPortSource::IOCell) {
                throw DesignError("cannot connect 2 IO-Cells");
            }
            
            connection(cab).initialize(cab, Connection::ToInput);
        }
    } else if (m_mode == IOMode::OutputBypass) {
        OutputPort *port = m_in.link()->out;
        if (port) {
            AnalogBlock &cab = port->cab();
            Connection &conn = connection(cab);

            switch (port->source()) {
                case OutPortSource::None:
                    break;
                    
                case OutPortSource::IOCell:
                    throw DesignError("cannot connect 2 IO-Cells");

                case OutPortSource::OpAmp1:
                    conn.initialize(cab, Connection::FromOutput);
                    break;

                case OutPortSource::OpAmp2:
                    conn.initialize(cab, Connection::FromOutput);
                    conn.channel = Connection::Secondary;
                    break;
            }
        }
    }
#endif

void IOCell::set_mode(IOMode mode) {
    if (m_mode != IOMode::Disabled) { 
        /* TODO will probably need changing */
        throw DesignError("Cannot change mode of IO-Cell");
    }

    m_mode = mode;
}

InputPort &IOCell::in(std::size_t i) {
    if (i == 0) {
        return in(1);
    }

    if (i != 1) {
        throw DesignError("IO-Cell can only access in(1)");
    }
    if (m_mode != IOMode::OutputBypass) {
        throw DesignError("TODO output");
    }

    return m_in;
}

OutputPort &IOCell::out(std::size_t i) {
    if (i == 0) {
        return out(1);
    }
    
    if (i != 1) {
        throw DesignError("IO-Cell can only access out(1)");
    }
    if (m_mode != IOMode::InputBypass) {
        throw DesignError("TODO input");
    }

    return m_out;
}

void IOCell::set_used_channel(CabColumn &group, Channel &channel) {
    Channel *&entry = m_used_channels.at(static_cast<int>(group));
    assert(entry == nullptr || entry == &channel);

    entry = &channel;
}

Channel &IOCell::used_channel(CabColumn &group) {
    Channel *channel = m_used_channels.at(static_cast<int>(group));
    if (channel) {
        return *channel;
    } else {
        return Channel::None();
    }
}
