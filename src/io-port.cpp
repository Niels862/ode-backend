#include "io-port.hpp"
#include "analog-module.hpp"
#include "analog-block.hpp"
#include "error.hpp"

InputPort::InputPort(AnalogModule &module)
        : m_module{&module} {}

uint8_t InputPort::connection_nibble() const {
    if (!m_connection) {
        return 0x0;
    }

    return m_connection->module().connection_nibble(*m_module);
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
