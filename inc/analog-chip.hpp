#ifndef OBC_ANALOG_CHIP_HPP
#define OBC_ANALOG_CHIP_HPP

#include "io-cell.hpp"
#include "shadow-sram.hpp"
#include "analog-block.hpp"
#include "connection-matrix.hpp"
#include <array>

class AnalogChip {
public:
    AnalogChip();

    void configure();

    ShadowSRam compile();

    void to_header_bytestream(std::vector<uint8_t> &data) const;

    AnalogBlock &cab(int id) { return m_cabs.at(id - 1); }
    AnalogBlock &null_cab() { return m_null_cab; }

    IOCell &io_cell(int id) { return m_io_cells.at(id - 1); }

    ConnectionMatrix &conns() { return m_conns; }

private:
    void compile_clocks(ShadowSRam &ssram);
    void compile_lut_io_control(ShadowSRam &ssram);
    void compile_io_routing(ShadowSRam &ssram);

    void configure_shared_routing(IOCell &pri, IOCell &sec, uint8_t data[]);

    std::array<AnalogBlock, 4> m_cabs;
    AnalogBlock m_null_cab;

    std::array<IOCell, 4> m_io_cells;

    ConnectionMatrix m_conns;
};

#endif
