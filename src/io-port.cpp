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
        : in{nullptr}, out{nullptr}, channels{} {}

PortLink::PortLink(InputPort *in, OutputPort *out)
        : in{in}, out{out}, channels{} {}

uint8_t PortLink::switch_connection_selector() {
    Channel const &final = *channels.back();
    return final.switch_connection_selector();
}

uint8_t PortLink::comparator_connection_selector() {
    Channel const &final = *channels.back();
    return final.comparator_connection_selector();
}

std::ostream &operator <<(std::ostream &os, PortLink const &link) {
    if (!link.out || !link.in) {
        os << "(empty link)";
        return os;
    };
    os << *link.out << " -> " << *link.in;
    if (!link.channels.empty()) {
        os << " in "; 
        bool first = true;
        for (Channel *channel : link.channels) {
            if (first) {
                first = false;
            } else {
                os << " -> ";
            }
            os << *channel;
        }
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

uint8_t InputPort::switch_connection_selector() {
    if (!m_link) {
        return 0x0;
    }
    return m_link->switch_connection_selector();
}

uint8_t InputPort::comparator_connection_selector() {
    if (!m_link) {
        return 0x0;
    }
    return m_link->comparator_connection_selector();
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
