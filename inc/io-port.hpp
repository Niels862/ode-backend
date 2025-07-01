#ifndef OBC_IO_PORT_HPP
#define OBC_IO_PORT_HPP

#include <vector>
#include <cstddef>
#include <cinttypes>

class AnalogBlock;
class AnalogModule;
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

class InputPort {
public:
    InputPort();
    InputPort(AnalogBlock &cab, InPortSource source);
    InputPort(IOCell &cell);

    /* Returns the 4-bit nibble representing the connection to this 
       input port as it appears in the configuration data. */
    uint8_t input_connection_selector();

    AnalogBlock &cab();
    IOCell &cell() { return *m_cell; }
    InPortSource source() const { return m_source; }

    OutputPort *connection() { return m_connection; }
    IOCell *io_connection();

private:
    void connect(OutputPort &port);

    AnalogBlock *m_cab;
    AnalogModule *m_module;
    IOCell *m_cell;
    InPortSource m_source;
    OutputPort *m_connection{nullptr};

    friend OutputPort;
};

class OutputPort {
public:
    OutputPort();
    OutputPort(AnalogBlock &cab, OutPortSource source);
    OutputPort(IOCell &cell);

    void connect(InputPort &port);

    AnalogBlock &cab();
    IOCell &cell() { return *m_cell; }
    OutPortSource source() const { return m_source; }

    std::vector<InputPort *> &connections() {
        return m_connections;
    }

private:
    uint8_t input_connection_selector(InputPort &to);

    AnalogBlock *m_cab;
    AnalogModule *m_module;
    IOCell *m_cell;
    OutPortSource m_source;
    std::vector<InputPort *> m_connections{};

    friend InputPort;
};

#endif
