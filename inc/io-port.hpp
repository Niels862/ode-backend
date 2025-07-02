#ifndef OBC_IO_PORT_HPP
#define OBC_IO_PORT_HPP

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

struct PortLink {
    PortLink();
    PortLink(InputPort *in, OutputPort *out);

    InputPort *in;
    OutputPort *out;
};

class InputPort {
public:
    InputPort();
    InputPort(AnalogBlock &cab, InPortSource source);
    InputPort(IOCell &cell);

    /* Returns the 4-bit nibble representing the connection to this 
       input port as it appears in the configuration data. */
    uint8_t input_connection_selector();

    AnalogBlock &cab();
    IOCell &io_cell() { return *m_io_cell; }
    InPortSource source() const { return m_source; }

    IOCell *io_connection();

    PortLink *link() { return m_link; }

private:
    void connect(OutputPort &out);

    AnalogBlock *m_cab;
    IOCell *m_io_cell;
    InPortSource m_source;
    OutputPort *m_connection{nullptr};

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

    std::vector<PortLink *> &links() { return m_links; }

private:
    uint8_t input_connection_selector(InputPort &to);

    AnalogBlock *m_cab;
    IOCell *m_io_cell;
    OutPortSource m_source;
    std::vector<InputPort *> m_connections{};

    std::vector<PortLink *> m_links;

    friend InputPort;
};

#endif
