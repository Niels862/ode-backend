#include "io-port.hpp"
#include "analog-module.hpp"
#include "analog-block.hpp"
#include "io-cell.hpp"
#include "error.hpp"
#include <cassert>

InputPort::InputPort()
        : m_module{} {}

InputPort::InputPort(AnalogModule &module)
        : m_module{&module} {}

uint8_t InputPort::input_connection_selector() {
    if (!m_connection) {
        return 0x0;
    }

    return m_connection->input_connection_selector(*this);
}

AnalogBlock &InputPort::cab() {
    return m_module->cab();
}

IOCell *InputPort::io_connection() {
    if (m_connection == nullptr) {
        return nullptr;
    }
    AnalogModule *other = &m_connection->module();
    if (auto cell = dynamic_cast<IOCell *>(other)) {
        return cell;
    } else {
        return nullptr;
    }
}

void InputPort::connect(OutputPort &port) {
    if (m_connection) {
        throw DesignError(
            "Cannot connect multiple output ports"
            " to single input port");
    }

    m_connection = &port;
}

OutputPort::OutputPort()
        : m_module{}, m_source{} {}

OutputPort::OutputPort(AnalogModule &module, PortSource source)
        : m_module{&module}, m_source{source} {}

void OutputPort::connect(InputPort &port) {
    m_connections.push_back(&port);
    port.connect(*this);
}

AnalogBlock &OutputPort::cab() {
    return m_module->cab();
}

static uint8_t iocell_connection_selector(AnalogModule &from,
                                          AnalogModule &to) {
    IOCell *cell = dynamic_cast<IOCell *>(&from);
    assert(cell != nullptr);
    
    Connection &conn = cell->connection(to.cab());

    return conn.cab_nibble(*cell);
}

static uint8_t opamp_connection_selector(int from, int to) {
    assert(from > 0 && from <= NBlocksPerChip);
    assert(to > 0 && to <= NBlocksPerChip);
    
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

    return 0x0;
}

uint8_t OutputPort::input_connection_selector(InputPort &input) {
    // Assert that the method is not called on an IOCell's input port (cab == 0) 
    assert(input.module().cab().id() > 0);

    int from = cab().id();
    int to   = input.module().cab().id();

    switch (m_source) {
        case PortSource::None:      
            return 0x0;

        case PortSource::IOCell:    
            return iocell_connection_selector(module(), input.module());

        case PortSource::OpAmp1:    
            return opamp_connection_selector(from, to);

        case PortSource::OpAmp2:    
            return opamp_connection_selector(from, to) - 1;
    }

    return 0x0;
}
