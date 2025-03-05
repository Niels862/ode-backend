#ifndef OBC_MEMORY_BASE_HPP
#define OBC_MEMORY_BASE_HPP

#include <iostream>
#include <memory>
#include <cstddef>
#include <initializer_list>

class MemoryCell {
public:
    MemoryCell();

    MemoryCell(std::byte value);
    MemoryCell(int value); /* For convenient initializer arrays */

    std::byte value() const { return m_value; }

    bool is_set() const { return m_is_set; }

private:
    std::byte m_value;
    bool m_is_set;
};

class MemoryBase {
public:
    MemoryBase(std::size_t bank_size, std::size_t n_banks, 
               std::size_t bank_addr_start);

    bool includes(std::size_t bank_addr, std::size_t byte_addr) const;

    MemoryCell const &get(std::size_t bank_addr, std::size_t byte_addr) const;

    void set(std::size_t bank_addr, std::size_t byte_addr, 
             std::byte value) const;

    void set(std::size_t bank_addr, std::size_t byte_addr,
             std::initializer_list<MemoryCell> cells);

    friend std::ostream &operator <<(std::ostream &stream, 
                                     MemoryBase const &mem);

protected:
    std::size_t translate(std::size_t bank_addr, std::size_t byte_addr) const;

    std::size_t m_bank_size;
    std::size_t m_n_banks;
    std::size_t m_bank_addr_start;

    std::unique_ptr<MemoryCell[]> m_banks;
};

#endif
