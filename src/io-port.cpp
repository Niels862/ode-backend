#include "io-port.hpp"
#include "analog-module.hpp"
#include "analog-block.hpp"
#include "io-cell.hpp"
#include "error.hpp"
#include <cassert>

InputPort::InputPort()
        : m_cab{}, m_cell{}, m_source{} {}

InputPort::InputPort(AnalogBlock &cab, InPortSource source)
        : m_cab{&cab}, m_cell{}, m_source{source} {}

InputPort::InputPort(IOCell &cell)
        : m_cab{&cell.cab()}, m_cell{&cell}, 
          m_source{InPortSource::IOCell} {}

uint8_t InputPort::input_connection_selector() {
    if (!m_connection) {
        return 0x0;
    }

    return m_connection->input_connection_selector(*this);
}

AnalogBlock &InputPort::cab() {
    if (m_cell) {
        return m_cell->cab();
    }
    return *m_cab;
}

IOCell *InputPort::io_connection() {
    if (m_connection == nullptr) {
        return nullptr;
    }
    if (m_connection->source() == OutPortSource::IOCell) {
        return &m_connection->cell();
    }
    return nullptr;
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
        : m_cab{}, m_cell{}, m_source{}, m_connections{} {}

OutputPort::OutputPort(AnalogBlock &cab, OutPortSource source)
        : m_cab{&cab}, m_cell{}, m_source{source} {}

OutputPort::OutputPort(IOCell &cell)
        : m_cab{&cell.cab()}, m_cell{&cell}, 
          m_source{OutPortSource::IOCell} {}

void OutputPort::connect(InputPort &port) {
    m_connections.push_back(&port);
    port.connect(*this);
}

AnalogBlock &OutputPort::cab() {
    if (m_cell) {
        return m_cell->cab();
    }
    return *m_cab;
}

static uint8_t iocell_connection_selector(IOCell &cell_from,
                                          AnalogBlock &cab_to) {    
    Connection &conn = cell_from.connection(cab_to);
    return conn.cab_nibble(cell_from);
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
    assert(input.source() != InPortSource::IOCell);

    int from = cab().id();
    int to   = input.cab().id();

    switch (m_source) {
        case OutPortSource::None:    
            return 0x0;

        case OutPortSource::IOCell:    
            return iocell_connection_selector(cell(), input.cab());

        case OutPortSource::OpAmp1:    
            return opamp_connection_selector(from, to);

        case OutPortSource::OpAmp2:    
            return opamp_connection_selector(from, to) - 1;
    }

    return 0x0;
}
