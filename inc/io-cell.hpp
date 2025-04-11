#ifndef OBC_IO_CELL_HPP
#define OBC_IO_CELL_HPP

#include "analog-module.hpp"
#include "analog-block.hpp"
#include "io-port.hpp"

enum class IOMode {
    Disabled, InputBypass, OutputBypass
};

class AnalogChip;

struct Connection {
    enum Kind {
        None,
        Connected,
    
        InputPrimary,
        InputSecondary,
    
        OutputPrimary,
        OutputSecondary,
    
        FarPrimary,
        FarSecondary,
    };

    Kind kind;
    AnalogBlock *cab;
};


class IOCell : public AnalogModule {
public:
    IOCell();

    /* Only allow in-place (re)initialization */
    void initialize(int id, AnalogBlock &cab);

    virtual uint8_t connection_nibble(AnalogModule &to); 

    void configure() override;

    IOMode mode() const { return m_mode; }
    void set_mode(IOMode mode);

    void use_primary_channel() { m_channel = false; }
    void use_secondary_channel() { m_channel = true; }

    int id() const { return m_id; }

    InputPort &in();
    OutputPort &out();

    std::array<Connection, NBlocksPerChip> &connections() { 
        return m_conns; 
    }
    Connection::Kind connection(AnalogBlock const &cab) const {
        return m_conns[cab.id() - 1].kind;
    }

private:
    int m_id;

    IOMode m_mode;
    bool m_channel;

    InputPort m_in;
    OutputPort m_out;

    std::array<Connection, NBlocksPerChip> m_conns;
};

#endif
