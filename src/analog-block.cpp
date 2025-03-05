#include "analog-block.hpp"

AnalogBlock::AnalogBlock(int id)
        : m_id{id} {}

AnalogBlock &AnalogBlock::None() {
    static AnalogBlock none{0};
    return none;
}
