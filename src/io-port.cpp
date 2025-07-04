#include "io-port.hpp"
#include "analog-module.hpp"
#include "analog-block.hpp"
#include "io-cell.hpp"
#include "error.hpp"
#include <sstream>
#include <cassert>

char const *to_string(InPortSource source) {
    switch (source) {
        case InPortSource::None:        return "none";
        case InPortSource::IOCell:      return "io-cell";
        case InPortSource::Local:       return "local";
        case InPortSource::Comparator:  return "comparator";
    }
    return "";
}

char const *to_string(OutPortSource source) {
    switch (source) {
        case OutPortSource::None:       return "none";
        case OutPortSource::IOCell:     return "io-cell";
        case OutPortSource::OpAmp1:     return "op-amp1";
        case OutPortSource::OpAmp2:     return "op-amp2";
    }
    return "";
}

PortLink::PortLink() 
        : in{nullptr}, out{nullptr}, driving{nullptr} {}

PortLink::PortLink(InputPort *in, OutputPort *out)
        : in{in}, out{out}, driving{nullptr} {}

std::ostream &operator <<(std::ostream &os, PortLink const &link) {
    if (!link.out || !link.in) {
        os << "(empty link)";
        return os;
    };
    os << *link.out << " -> " << *link.in;
    if (link.driving) {
        os << " in " << *link.driving;
    } else {
        os << " (virtual)";
    }
    return os;
}

InputPort::InputPort()
        : m_cab{}, m_io_cell{}, m_source{}, 
          m_link{}, m_owned_link{} {}

InputPort::InputPort(AnalogBlock &cab, InPortSource source)
        : m_cab{&cab}, m_io_cell{}, m_source{source},
          m_link{}, m_owned_link{} {}

InputPort::InputPort(IOCell &cell)
        : m_cab{&cell.cab()}, m_io_cell{&cell}, 
          m_source{InPortSource::IOCell},
          m_link{}, m_owned_link{} {}

uint8_t InputPort::input_connection_selector() {
    if (!m_link) {
        return 0x0;
    }

    return m_link->out->input_connection_selector(*this);
}

AnalogBlock &InputPort::cab() {
    if (m_io_cell) {
        return m_io_cell->cab();
    }
    return *m_cab;
}

IOCell *InputPort::io_connection() {
    if (m_link == nullptr) {
        return nullptr;
    }
    if (m_link->out->source() == OutPortSource::IOCell) {
        return &m_link->out->io_cell();
    }
    return nullptr;
}

std::ostream &operator <<(std::ostream &os, InputPort const &in) {
    if (in.m_source == InPortSource::IOCell) {
        os << "IO" << in.m_io_cell->id();
    } else {
        os << "CAB" << in.m_cab->id() << ":" << to_string(in.m_source);
    }
    return os;
}

void InputPort::connect(OutputPort &out) {
    if (m_link) {
        std::stringstream ss;
        ss << "IO port already connected: " << *m_link << std::endl;
        throw DesignError(ss.str());
    }

    m_owned_link = PortLink(this, &out);
    m_link = &m_owned_link;

    std::cout << *m_link << std::endl;
}

OutputPort::OutputPort()
        : m_cab{}, m_io_cell{}, m_source{} {}

OutputPort::OutputPort(AnalogBlock &cab, OutPortSource source)
        : m_cab{&cab}, m_io_cell{}, m_source{source} {}

OutputPort::OutputPort(IOCell &cell)
        : m_cab{&cell.cab()}, m_io_cell{&cell}, 
          m_source{OutPortSource::IOCell} {}

void OutputPort::connect(InputPort &in) {
    in.connect(*this);
    m_links.push_back(in.link());
}

AnalogBlock &OutputPort::cab() {
    if (m_io_cell) {
        return m_io_cell->cab();
    }
    return *m_cab;
}

std::ostream &operator <<(std::ostream &os, OutputPort const &out) {
    if (out.m_source == OutPortSource::IOCell) {
        os << "IO" << out.m_io_cell->id();
    } else {
        os << "CAB" << out.m_cab->id() << ":" << to_string(out.m_source);
    }
    return os;
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
            return iocell_connection_selector(*m_io_cell, input.cab());

        case OutPortSource::OpAmp1:    
            return opamp_connection_selector(from, to);

        case OutPortSource::OpAmp2:    
            return opamp_connection_selector(from, to) - 1;
    }

    return 0x0;
}
