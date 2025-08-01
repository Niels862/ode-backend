#include "clock.hpp"
#include "error.hpp"
#include <sstream>

Clock::Clock()
        : m_id{}, m_is_used{}, m_freq_kHz{}, m_offset{} {}

Clock::Clock(int id, std::size_t value, std::size_t offset) 
        : m_id{id}, m_is_used{false}, m_freq_kHz{value}, m_offset{offset} {}

uint8_t Clock::id_nibble() const {
    switch (m_id) {
        case 0: return 0x0;
        case 1: return 0xC;
        case 2: return 0xD;
        case 3: return 0xE;
        case 4: return 0xF;
        case 5: return 0xA;
        case 6: return 0xB;
    }

    return 0x0;
}

void Clock::compile(ShadowSRam &ssram, std::size_t sys) const {
    uint8_t data_value;

    if (m_freq_kHz == sys) {
        data_value = 0;
    } else {
        if (sys % (2 * m_freq_kHz) != 0) {
            std::stringstream ss;
            ss << "Cannot realize value of Clock " << m_id << ": "
               << m_freq_kHz;
            throw DesignError(ss.str());
        }

        data_value = sys / m_freq_kHz / 2;
    }

    switch (m_id) {
        case 1: ssram.set(0x0, 0x07, data_value); break;
        case 2: ssram.set(0x0, 0x06, data_value); break;
        case 3: ssram.set(0x0, 0x05, data_value); break;
        case 4: ssram.set(0x0, 0x04, data_value); break;
        case 5: ssram.set(0x0, 0x03, data_value);
                ssram.set(0x0, 0x02, 0x0);        break;
        case 6: ssram.set(0x0, 0x01, data_value);
                ssram.set(0x0, 0x00, 0x0);        break;
    }
}
