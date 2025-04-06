#ifndef OBC_CONNECTION_MATRIX_HPP
#define OBC_CONNECTION_MATRIX_HPP

#include "defs.hpp"
#include <bitset>
#include <cstddef>

class ConnectionMatrix {
public:
    ConnectionMatrix() = default;

    void reset() {
        m_matrix = 0;
    }

    void set(std::size_t from, std::size_t to) {
        m_matrix.set(translate(from, to));
    }

    bool test(std::size_t from, std::size_t to) const {
        return m_matrix.test(translate(from, to));
    }

    using bitset_type = 
            std::bitset<NType1IOCellsPerChip * NBlocksPerChip>;

    bitset_type const &bitset() { return m_matrix; } 

private:
    std::size_t translate(std::size_t from, std::size_t to) const {
        return NType1IOCellsPerChip * from + to;
    }

    bitset_type m_matrix;
};

#endif
