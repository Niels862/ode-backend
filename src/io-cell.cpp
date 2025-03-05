#include "io-cell.hpp"
#include "error.hpp"

IOCell::IOCell(int id)
        : AnalogModule{AnalogBlock::None()}, m_id{id} {}

void IOCell::set_mode(IOMode mode) {
    if (m_mode != IOMode::Disabled) { 
        /* TODO will probably need changing */
        throw DesignError(
                "Cannot change mode of IO-Cell");
    }

    m_mode = mode;
}

InputPort &IOCell::in() {
    if (m_mode != IOMode::OutputBypass) {
        throw DesignError(
                "Can only access ::in() of output IO-Cell");
    }

    return m_in;
}

OutputPort &IOCell::out() {
    if (m_mode != IOMode::InputBypass) {
        throw DesignError(
                "Can only access ::out() of input IO-Cell");
    }

    return m_out;
}