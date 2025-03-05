#include "analog-chip.hpp"

AnalogChip::AnalogChip()
        : m_cabs{ {
            AnalogBlock{1},
            AnalogBlock{2},
            AnalogBlock{3},
            AnalogBlock{4}
        } }, 
        m_io_cells{ {
            IOCell(1),
            IOCell(2),
            IOCell(3),
            IOCell(4)
        } } {}
