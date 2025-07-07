#ifndef OBC_IO_PORT_HPP
#define OBC_IO_PORT_HPP

#include "io-channel.hpp"
#include <iostream>
#include <vector>
#include <cstddef>
#include <cinttypes>

class AnalogBlock;
class AnalogModule;

class InputPort;
class OutputPort;

class IOCell;

enum class InPortSource {
    None,
    IOCell,
    Local,
    Comparator,
};

enum class OutPortSource {
    None,
    IOCell,
    OpAmp1,
    OpAmp2,
};

char const *to_string(InPortSource source);
char const *to_string(OutPortSource source);

struct PortLink {
    PortLink();
    PortLink(InputPort *in, OutputPort *out);

    uint8_t switch_connection_selector();
    uint8_t comparator_connection_selector();

    friend std::ostream &operator <<(std::ostream &os, PortLink const &link);

    InputPort *in;
    OutputPort *out;
    std::vector<Channel *> channels;
};

class InputPort {
public:
    InputPort();
    InputPort(AnalogBlock &cab, InPortSource source);
    InputPort(IOCell &cell);

    /* Returns the 4-bit nibble representing the connection to this 
       input port as it appears in the configuration data. */
    uint8_t switch_connection_selector();
    uint8_t comparator_connection_selector();

    AnalogBlock &cab();
    IOCell &io_cell() { return *m_io_cell; }
    InPortSource source() const { return m_source; }

    IOCell *io_connection();

    bool connected() const { return m_link != nullptr; }
    PortLink *link() { return m_link; }

    friend std::ostream &operator <<(std::ostream &os, InputPort const &in);

private:
    void connect(OutputPort &out);

    AnalogBlock *m_cab;
    IOCell *m_io_cell;
    InPortSource m_source;

    PortLink *m_link;
    PortLink m_owned_link;

    friend OutputPort;
};

class OutputPort {
public:
    OutputPort();
    OutputPort(AnalogBlock &cab, OutPortSource source);
    OutputPort(IOCell &cell);

    void connect(InputPort &in);

    AnalogBlock &cab();
    IOCell &io_cell() { return *m_io_cell; }
    OutPortSource source() const { return m_source; }

    bool connected() const { return !m_links.empty(); }
    std::vector<PortLink *> &links() { return m_links; }

    friend std::ostream &operator <<(std::ostream &os, OutputPort const &out);

private:
    AnalogBlock *m_cab;
    IOCell *m_io_cell;
    OutPortSource m_source;

    std::vector<PortLink *> m_links;

    friend InputPort;
};

#endif
