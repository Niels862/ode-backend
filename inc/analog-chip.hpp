#ifndef OBC_ANALOG_CHIP_HPP
#define OBC_ANALOG_CHIP_HPP

#include "analog-block.hpp"
#include "io-cell.hpp"
#include <array>

class AnalogChip {
public:
    AnalogChip();

    AnalogBlock &cab(int id) { return m_cabs.at(id - 1); }

    IOCell &io_cell(int id) { return m_io_cells.at(id - 1); }

private:
    std::array<AnalogBlock, 4> m_cabs;

    std::array<IOCell, 4> m_io_cells;
};

#endif
