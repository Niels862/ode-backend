#ifndef OBC_IO_CELL_HPP
#define OBC_IO_CELL_HPP

#include "analog-module.hpp"
#include "analog-block.hpp"
#include "io-port.hpp"

enum class IOMode {
    Disabled, InputBypass, OutputBypass
};

class AnalogChip;
class IOCell;

struct Connection {
    enum Block {
        None,
        ToInput,
        FromOutput,
    };

    enum Mode {
        Near,
        Far
    };

    enum Channel {
        Primary,
        Secondary
    };

    enum class Internal {
        Unset,
        P,
        Q,
    };

    Block block;
    Mode mode;
    Channel channel;
    AnalogBlock *cab;
    Internal internal;

    Connection() { reset(); }
    void reset();

    void initialize(AnalogBlock &cab, Block block);

    uint8_t io_nibble() const;
    uint8_t cab_nibble(IOCell &cell) const;

    bool equivalent(Connection const &other) const;
};

class IOCell : public AnalogModule {
public:
    IOCell();

    /* Only allow in-place (re)initialization */
    void initialize(int id, AnalogBlock &cab);
   
    bool set_parameter(std::string_view, Parameter) override { return false; }

    void claim_components() override {}
    void finalize() override;

    IOMode mode() const { return m_mode; }
    void set_mode(IOMode mode);

    int id() const { return m_id; }

    InputPort &in(std::size_t i = 0) override;
    OutputPort &out(std::size_t i = 0) override;

    std::array<Connection, NBlocksPerChip> &connections() { 
        return m_conns; 
    }
    Connection &connection(AnalogBlock const &cab) {
        return m_conns[cab.id() - 1];
    }

private:
    AnalogChip *m_chip;

    int m_id;

    IOMode m_mode;

    InputPort m_in;
    OutputPort m_out;

    std::array<Connection, NBlocksPerChip> m_conns;
};

#endif
