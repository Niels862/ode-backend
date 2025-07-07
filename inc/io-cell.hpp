#ifndef OBC_IO_CELL_HPP
#define OBC_IO_CELL_HPP

#include "analog-module.hpp"
#include "analog-block.hpp"
#include "io-channel.hpp"
#include "io-port.hpp"

enum class IOMode {
    Disabled        = 0x00, 
    InputBypass     = 0x40, 
    OutputBypass    = 0x10,
};

class AnalogChip;
class IOCell;

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

    AnalogChip &chip() { return *m_chip; }

    int id() const { return m_id; }

    InputPort &in(std::size_t i = 0) override;
    OutputPort &out(std::size_t i = 0) override;

    void set_used_channel(CabColumn &group, Channel &channel);
    Channel &used_channel(CabColumn &group);

private:
    AnalogChip *m_chip;

    int m_id;

    IOMode m_mode;

    InputPort m_in;
    OutputPort m_out;

    std::array<Channel *, 2> m_used_channels;
};

#endif
