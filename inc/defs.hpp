#ifndef OBC_DEFS_HPP
#define OBC_DEFS_HPP

#include <cstddef>

constexpr std::size_t NCapacitorsPerBlock = 8;

constexpr std::size_t NOpAmpsPerBlock = 2;

constexpr std::size_t NType1IOCellsPerChip = 4;

constexpr std::size_t NBlocksPerChip = 4;

constexpr uint8_t from_nibbles(uint8_t n1, uint8_t n2) {
    return (n1 << 4) | n2;
}

#endif
