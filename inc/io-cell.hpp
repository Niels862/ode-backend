#ifndef OBC_IO_CELL_HPP
#define OBC_IO_CELL_HPP

#include "analog-module.hpp"
#include "io-port.hpp"

enum class IOMode {
    Disabled, InputBypass, OutputBypass
};

class AnalogChip;

class IOCell : public AnalogModule {
public:
    IOCell();

    /* Delete copy/move semantics as this breaks links with Ports. */
    IOCell(IOCell const &) = delete;
    IOCell &operator=(IOCell const &) = delete;
    IOCell(IOCell &&) = delete;
    IOCell &operator=(IOCell &&) = delete;

    /* Only allow in-place (re)initialization */
    void initialize(int id, AnalogBlock &cab);

    void setup() override {}

    void set_mode(IOMode mode);

    int id() const { return m_id; }

    InputPort &in();
    OutputPort &out();

private:
    int m_id;

    IOMode m_mode;

    InputPort m_in;
    OutputPort m_out;
};

#endif
