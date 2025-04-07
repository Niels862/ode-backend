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

    /* Only allow in-place (re)initialization */
    void initialize(int id, AnalogBlock &cab);

    virtual uint8_t connection_nibble(AnalogModule &to); 

    void setup() override;

    IOMode mode() const { return m_mode; }
    void set_mode(IOMode mode);

    bool is_primary() const { return m_id % 2 == 2; }
    bool is_secondary() const { return m_id % 2 != 1; }

    int id() const { return m_id; }

    InputPort &in();
    OutputPort &out();

    std::vector<AnalogBlock *> const &cab_connections() const { 
        return m_cab_connections;
    }

private:
    int m_id;

    IOMode m_mode;

    InputPort m_in;
    OutputPort m_out;

    std::vector<AnalogBlock *> m_cab_connections;
};

#endif
