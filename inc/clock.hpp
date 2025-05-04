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

    std::size_t value() const { return m_value; }
    std::size_t offset() const { return m_offset; }

    enum Select {
        A, 
        B
    };

private:
    int m_id;
    bool m_is_used;

    std::size_t m_value;
    std::size_t m_offset;
};

#endif
