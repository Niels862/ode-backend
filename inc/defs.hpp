#ifndef OBC_DEFS_HPP
#define OBC_DEFS_HPP

#include <cstddef>

constexpr int NCapacitorsPerBlock = 8;

constexpr int NOpAmpsPerBlock = 2;

constexpr int NType1IOCellsPerChip = 4;

constexpr int NBlocksPerChip = 4;

constexpr uint8_t from_nibbles(uint8_t n1, uint8_t n2) {
    return (n1 << 4) | n2;
}

#endif
