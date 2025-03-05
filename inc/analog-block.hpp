#ifndef OBC_ANALOG_BLOCK_HPP
#define OBC_ANALOG_BLOCK_HPP

#include <cstddef>

class AnalogBlock {
public:
    AnalogBlock(int id);
    static AnalogBlock &None();

    std::size_t bank_a() const { return 2 * m_id + 1; }
    std::size_t bank_b() const { return bank_a() + 1; }

    int id() const { return m_id; }

private:
    int m_id;
};

#endif
