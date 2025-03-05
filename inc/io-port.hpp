#ifndef OBC_IO_PORT_HPP
#define OBC_IO_PORT_HPP

#include <vector>
#include <cstddef>

class AnalogModule;
class OutputPort;

class InputPort {
public:
    InputPort(AnalogModule &module);

    /* Returns the 4-bit nibble representing the connection to this 
       input port as it appears in the configuration data. */
    std::byte connection_nibble() const;

    AnalogModule &module() { return m_module; }

    OutputPort const *connection() const { return m_connection; }

private:
    void connect(OutputPort &port);

    AnalogModule &m_module;
    OutputPort *m_connection{nullptr};

    friend OutputPort;
};

class OutputPort {
public:
    OutputPort(AnalogModule &module);

    void connect(InputPort &port);

    AnalogModule &module() { return m_module; }

    std::vector<InputPort *> const &connections() {
     return m_connections;
    }

private:
    AnalogModule &m_module;
    std::vector<InputPort *> m_connections{};
};

#endif
