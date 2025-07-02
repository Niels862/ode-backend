#ifndef OBC_CLOCK_HPP
#define OBC_CLOCK_HPP

#include "shadow-sram.hpp"
#include <cstddef>

class Clock {
public:
    Clock();
    Clock(int id, std::size_t value, std::size_t offset);

    uint8_t id_nibble() const;
    void compile(ShadowSRam &ssram, std::size_t sys) const;

    int id() const { return m_id; }

    bool is_used() const { return m_is_used; }
    void set_is_used(bool is_used) { m_is_used = is_used; }

    std::size_t freq_kHz() const { return m_freq_kHz; }
    double freq_mHz() const { return static_cast<double>(m_freq_kHz) / 1'000; }
    std::size_t offset() const { return m_offset; }

    enum Select {
        A, 
        B
    };

private:
    int m_id;
    bool m_is_used;

    std::size_t m_freq_kHz;
    std::size_t m_offset;
};

#endif
