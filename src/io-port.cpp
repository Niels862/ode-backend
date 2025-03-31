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

    auto &a = m_connection->module();
    auto &b = a.cab();
    auto c = b.id();

    std::cout << ">> In Module at " << &a << ":" << std::endl;
    std::cout << ">> Cab" << b.id() << " at " << &b << std::endl;

    int from = c;
    int to = m_module->cab().id();

    if (from < 1 || to < 1) {
        throw DesignError("Not yet supported");
    }

    std::array<std::array<uint8_t, 4>, 4> nibbles = { {
        { 0x3, 0xB, 0xB, 0xB },
        { 0xF, 0x3, 0x9, 0x9 },
        { 0xB, 0xD, 0x7, 0xD },
        { 0xD, 0xF, 0xF, 0x3 }
    } };

    return nibbles.at(from).at(to);
}

void InputPort::connect(OutputPort &port) {
    if (m_connection) {
        throw DesignError(
            "Cannot connect multiple output ports"
            " to single input port");
    }

    std::cout << "InputPort at " << this << " connected to " 
              << &port << std::endl;

    m_connection = &port;
}

OutputPort::OutputPort(AnalogModule &module)
        : m_module{&module} {}

void OutputPort::connect(InputPort &port) {
    m_connections.push_back(&port);
    std::cout << "connecting to " << &port << " from " << this << std::endl;
    port.connect(*this);
}
