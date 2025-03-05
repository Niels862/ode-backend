#ifndef OBC_IO_CELL_HPP
#define OBC_IO_CELL_HPP

#include "analog-module.hpp"
#include "io-port.hpp"

enum class IOMode {
    Disabled, InputBypass, OutputBypass
};

class IOCell : public AnalogModule {
public:
    IOCell(int id);

    void set_mode(IOMode mode);

    int id() const { return m_id; }

    InputPort &in();
    OutputPort &out();

private:
    int m_id{};

    IOMode m_mode{IOMode::Disabled};

    InputPort m_in{*this};
    OutputPort m_out{*this};
};

#endif
