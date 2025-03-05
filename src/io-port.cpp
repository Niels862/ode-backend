#include "io-port.hpp"
#include "analog-module.hpp"
#include "error.hpp"

InputPort::InputPort(AnalogModule &module)
        : m_module{module} {}

std::byte InputPort::connection_nibble() const {
    if (!m_connection) {
        return std::byte{0x0};
    }

    int from = m_connection->module().cab().id();
    int to = m_module.cab().id();

    if (from < 1 || to < 0) {
        throw DesignError("Not yet supported");
    }

    std::array<std::array<unsigned char, 4>, 4> nibbles = { {
        { 0x3, 0xB, 0xB, 0xB },
        { 0xF, 0x3, 0x9, 0x9 },
        { 0xB, 0xD, 0x7, 0xD },
        { 0xD, 0xF, 0xF, 0x3 }
    } };

    return std::byte{nibbles.at(from).at(to)};
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
        : m_module{module} {}

void OutputPort::connect(InputPort &port) {
    m_connections.push_back(&port);
    port.connect(*this);
}
