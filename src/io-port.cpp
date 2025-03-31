#include "io-port.hpp"
#include "analog-module.hpp"
#include "analog-block.hpp"
#include "error.hpp"

uint8_t nibble_cab_to_cab(int from, int to) {
    switch (from) {
        case 1:
            switch (to) {
                case 1: return 0x3;
                case 2: return 0xB;
                case 3: return 0xB;
                case 4: return 0xB;
            }
            break;
        
        case 2:
            switch (to) {
                case 1: return 0xF;
                case 2: return 0x3;
                case 3: return 0x9;
                case 4: return 0x9;
            }
            break;

        case 3:
            switch (to) {
                case 1: return 0xB;
                case 2: return 0xD;
                case 3: return 0x7;
                case 4: return 0xD;
            }
            break;

        case 4:
            switch (to) {
                case 1: return 0xD;
                case 2: return 0xF;
                case 3: return 0xF;
                case 4: return 0x3;
            }
            break;
    }

    throw DesignError("Invalid Block ID");
}

InputPort::InputPort(AnalogModule &module)
        : m_module{&module} {}

uint8_t InputPort::connection_nibble() const {
    if (!m_connection) {
        return 0x0;
    }

    int from = m_connection->module().cab().id();
    int to = m_module->cab().id();

    if (from > 0 && to > 0) {
        return nibble_cab_to_cab(from, to);
    }

    return 0x0;
}

void InputPort::connect(OutputPort &port) {
    if (m_connection) {
        throw DesignError(
            "Cannot connect multiple output ports"
            " to single input port");
    }

    m_connection = &port;
}

OutputPort::OutputPort(AnalogModule &module)
        : m_module{&module} {}

void OutputPort::connect(InputPort &port) {
    m_connections.push_back(&port);
    port.connect(*this);
}
