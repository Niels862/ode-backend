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
        : AnalogModule{"IOCell"}, m_id{}, 
          m_mode{IOMode::Disabled},
          m_in{*this}, m_out{*this, PortSource::IOCell}, m_conns{} {}

void IOCell::initialize(int id, AnalogBlock &cab) {
    set_cab(cab);
    m_id = id;
    m_mode = IOMode::Disabled;

    for (Connection &conn : m_conns) {
        conn.reset();
    }
}

void IOCell::finalize() {
    for (Connection &conn : m_conns) {
        conn.reset();
    }

    if (m_mode == IOMode::InputBypass) {
        for (InputPort *port : out(1).connections()) {
            AnalogBlock &cab = port->module().cab();
            if (cab) {
                connection(cab).initialize(cab, Connection::ToInput);
            } else {
                throw DesignError("cannot connect 2 IO-Cells");
            }
        }
    } else if (m_mode == IOMode::OutputBypass) {
        OutputPort *port = in(1).connection();
        if (port) {
            AnalogBlock &cab = port->module().cab();
            Connection &conn = connection(cab);

            switch (port->source()) {
                case PortSource::None:
                    break;
                    
                case PortSource::IOCell:
                    throw DesignError("cannot connect 2 IO-Cells");

                case PortSource::OpAmp1:
                    conn.initialize(cab, Connection::FromOutput);
                    break;

                case PortSource::OpAmp2:
                    conn.initialize(cab, Connection::FromOutput);
                    conn.channel = Connection::Secondary;
                    break;
            }
        }
    }
}

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
