#ifndef OBC_ANALOG_CHIP_HPP
#define OBC_ANALOG_CHIP_HPP

#include "io-cell.hpp"
#include "clock.hpp"
#include "shadow-sram.hpp"
#include "analog-block.hpp"
#include "connection-matrix.hpp"
#include <array>

class AnalogChip {
public:
    AnalogChip();

    ShadowSRam compile();

    void to_header_bytestream(std::vector<uint8_t> &data) const;

    AnalogBlock &cab(int id)        { return m_cabs.at(id - 1); }
    AnalogBlock &null_cab()         { return m_null_cab; }

    IOCell &io_cell(int id)         { return m_io_cells.at(id - 1); }

    Clock &clock(int id)            { return m_clocks.at(id - 1); }
    Clock &null_clock()             { return m_null_clock; }

private:
    void compile_clocks(ShadowSRam &ssram);
    void compile_lut_io_control(ShadowSRam &ssram);
    void compile_io_routing(ShadowSRam &ssram);

    void configure_shared_routing(IOCell &pri, IOCell &sec, uint8_t data[]);

    std::array<AnalogBlock, NBlocksPerChip> m_cabs;
    AnalogBlock m_null_cab;

    std::array<IOCell, NType1IOCellsPerChip> m_io_cells;

    std::array<Clock, 6> m_clocks;
    Clock m_null_clock;
};

#endif
