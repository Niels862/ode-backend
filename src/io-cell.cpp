#include "io-cell.hpp"
#include "error.hpp"
#include "analog-block.hpp"
#include "analog-chip.hpp"

IOCell::IOCell()
        : AnalogModule{}, m_id{}, 
          m_mode{IOMode::Disabled},
          m_in{*this}, m_out{*this} {}

void IOCell::initialize(int id, AnalogBlock &cab) {
    set_cab(cab);
    m_id = id;
    m_mode = IOMode::Disabled;
}

void IOCell::setup() {
    m_cab_connections.clear();

    if (m_mode == IOMode::InputBypass) {
        for (InputPort *port : out().connections()) {
            AnalogBlock &cab = port->module().cab();
            if (cab.id() > 0) {
                m_cab_connections.push_back(&cab);
            }
        }
    } else if (m_mode == IOMode::OutputBypass) {
        OutputPort *port = in().connection();
        if (port) {
            AnalogBlock &cab = port->module().cab();
            if (cab.id() > 0) {
                m_cab_connections.push_back(&cab);
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

InputPort &IOCell::in() {
    if (m_mode != IOMode::OutputBypass) {
        throw DesignError(
                "Can only access ::in() of output IO-Cell");
    }

    return m_in;
}

OutputPort &IOCell::out() {
    if (m_mode != IOMode::InputBypass) {
        throw DesignError(
                "Can only access ::out() of input IO-Cell");
    }

    return m_out;
}