#ifndef OBC_IO_PORT_HPP
#define OBC_IO_PORT_HPP

#include <vector>
#include <cstddef>
#include <cinttypes>

class AnalogBlock;
class AnalogModule;
class OutputPort;

class IOCell;

enum class PortSource {
    None,
    IOCell,
    OpAmp1,
    OpAmp2,
};

class InputPort {
public:
    InputPort();
    InputPort(AnalogModule &module);

    /* Returns the 4-bit nibble representing the connection to this 
       input port as it appears in the configuration data. */
    uint8_t input_connection_selector();

    AnalogBlock &cab();
    AnalogModule &module() { return *m_module; }

    OutputPort *connection() { return m_connection; }
    IOCell *io_connection();

private:
    void connect(OutputPort &port);

    AnalogModule *m_module;
    OutputPort *m_connection{nullptr};

    friend OutputPort;
};

class OutputPort {
public:
    OutputPort();
    OutputPort(AnalogModule &module, PortSource source);

    void connect(InputPort &port);

    AnalogBlock &cab();
    AnalogModule &module() { return *m_module; }
    PortSource source() const { return m_source; }

    std::vector<InputPort *> &connections() {
        return m_connections;
    }

private:
    uint8_t input_connection_selector(InputPort &to);

    AnalogModule *m_module;
    PortSource m_source;
    std::vector<InputPort *> m_connections{};

    friend InputPort;
};

#endif
