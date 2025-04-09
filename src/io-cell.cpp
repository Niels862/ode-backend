#include "io-cell.hpp"
#include "error.hpp"
#include "analog-block.hpp"
#include "analog-chip.hpp"

IOCell::IOCell()
        : AnalogModule{}, m_id{}, 
          m_mode{IOMode::Disabled}, m_channel{false},
          m_in{*this}, m_out{*this} {}

void IOCell::initialize(int id, AnalogBlock &cab) {
    set_cab(cab);
    m_id = id;
    m_mode = IOMode::Disabled;
}

uint8_t IOCell::connection_nibble(AnalogModule &to) {
    int to_id = to.cab().id();
    uint8_t n = 0x0;

    switch (m_id) {
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

    std::cout << "channel-" << m_channel << std::endl;

    return n - m_channel;
}

void IOCell::configure() {
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