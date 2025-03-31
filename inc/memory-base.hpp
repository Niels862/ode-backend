#ifndef OBC_MEMORY_BASE_HPP
#define OBC_MEMORY_BASE_HPP

#include <iostream>
#include <memory>
#include <vector>
#include <cinttypes>
#include <cstddef>
#include <initializer_list>

struct MemoryAddress {
    MemoryAddress();
    MemoryAddress(uint8_t bank_addr, uint8_t byte_addr);

    uint8_t bank_addr;
    uint8_t byte_addr;
};

class MemoryCell {
public:
    MemoryCell();

    MemoryCell(uint8_t value);
    MemoryCell(int value); /* For convenient initializer arrays */

    uint8_t value() const { return m_value; }

    bool is_set() const { return m_is_set; }

private:
    uint8_t m_value;
    bool m_is_set;
};

class MemoryBase {
public:
    MemoryBase(std::size_t bank_size, std::size_t n_banks, 
               std::size_t bank_addr_start);

    bool includes(std::size_t bank_addr, std::size_t byte_addr) const;

    MemoryCell const &get(std::size_t bank_addr, std::size_t byte_addr) const;

    void set(std::size_t bank_addr, std::size_t byte_addr, 
             uint8_t value) const;

    void set(std::size_t bank_addr, std::size_t byte_addr,
             std::initializer_list<MemoryCell> cells);

    std::vector<uint8_t> to_configuration_info() const;

    std::size_t size() const { return m_bank_size * m_n_banks; }

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
