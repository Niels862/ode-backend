#ifndef OBC_ANALOG_CHIP_HPP
#define OBC_ANALOG_CHIP_HPP

#include "io-cell.hpp"
#include "shadow-sram.hpp"
#include "analog-block.hpp"
#include <array>

class AnalogChip {
public:
    AnalogChip();

    ShadowSRam compile() const;

    void to_header_bytestream(std::vector<uint8_t> &data) const;

    AnalogBlock &cab(int id) { return m_cabs.at(id - 1); }

    AnalogBlock &null_cab() { return m_null_cab; }

    IOCell &io_cell(int id) { return m_io_cells.at(id - 1); }

private:
    void compile_clocks(ShadowSRam &ssram) const;

    std::array<AnalogBlock, 4> m_cabs;
    AnalogBlock m_null_cab;

    std::array<IOCell, 4> m_io_cells;
};

#endif
