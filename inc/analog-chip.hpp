#ifndef OBC_ANALOG_CHIP_HPP
#define OBC_ANALOG_CHIP_HPP

#include "io-cell.hpp"
#include "io-channel.hpp"
#include "clock.hpp"
#include "shadow-sram.hpp"
#include "analog-block.hpp"
#include <array>

class AnalogChip {
public:
    AnalogChip();

    /* Delete copy/move semantics as this breaks links with Ports. */
    AnalogChip(AnalogChip const &) = delete;
    AnalogChip &operator=(AnalogChip const &) = delete;
    
    AnalogChip(AnalogChip &&) = delete;
    AnalogChip &operator=(AnalogChip &&) = delete;

    ShadowSRam compile();

    void to_header_bytestream(std::vector<uint8_t> &data) const;

    AnalogBlock &cab(int id)        { return m_cabs.at(id - 1); }
    AnalogBlock &null_cab()         { return m_null_cab; }

    IOCell &io_cell(int id)         { return m_io_cells.at(id - 1); }

    Clock &clock(int id)            { return m_clocks.at(id - 1); }
    Clock &null_clock()             { return m_null_clock; }

    Channel &global_input_direct(IOGroup from, AnalogBlock &to, 
                                 Channel::Side side);
    Channel &global_output_direct(IOGroup to, AnalogBlock &from,
                                  Channel::Side side);
    Channel &global_bi_indirect(CabColumn group, Channel::Side side);
    Channel &intercam_channel(AnalogBlock &from, AnalogBlock &to, 
                              Channel::Side side);

private:
    void compile_clocks(ShadowSRam &ssram);
    void compile_lut_io_control(ShadowSRam &ssram);
    void compile_io_routing(ShadowSRam &ssram);

    std::array<AnalogBlock, NBlocksPerChip> m_cabs;
    AnalogBlock m_null_cab;

    std::array<IOCell, NType1IOCellsPerChip> m_io_cells;

    std::array<Clock, 6> m_clocks;
    Clock m_null_clock;

    // [iogroup_from][cab_to][side]
    std::array<std::array<std::array<Channel, 2>, 4>, 2> m_global_input_direct_channels;

    // [iogroup_to][cab_from][side]
    std::array<std::array<std::array<Channel, 2>, 4>, 2> m_global_output_direct_channels;

    // [cab_group][side]
    std::array<std::array<Channel, 2>, 4> m_global_bi_indirect_channels;

    // [from][to][side]
    std::array<std::array<std::array<Channel, 2>, 4>, 4> m_intercam_channels;
};

#endif
