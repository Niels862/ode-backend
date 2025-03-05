#include "memory-base.hpp"
#include <iomanip>

MemoryCell::MemoryCell()
        : m_value{0}, m_is_set{false} {}

MemoryCell::MemoryCell(std::byte value)
        : m_value{value}, m_is_set{true} {}

MemoryCell::MemoryCell(int value)
        : m_value{std::byte(value)}, m_is_set{true} {}

MemoryBase::MemoryBase(std::size_t bank_size, std::size_t n_banks, 
                       std::size_t bank_addr_start)
        : m_bank_size{bank_size}, m_n_banks{n_banks}, 
          m_bank_addr_start{bank_addr_start},
          m_banks{std::make_unique<MemoryCell[]>(m_bank_size * m_n_banks)} {}

bool MemoryBase::includes(std::size_t bank_addr, 
                          std::size_t byte_addr) const {
    return bank_addr >= m_bank_addr_start
            && translate(bank_addr, byte_addr) < m_bank_size * m_n_banks;
}

MemoryCell const &MemoryBase::get(std::size_t bank_addr, 
                                  std::size_t byte_addr) const {
    if (!includes(bank_addr, byte_addr)) {
        /* error... */
    }
    return m_banks[translate(bank_addr, byte_addr)];
}

void MemoryBase::set(std::size_t bank_addr, std::size_t byte_addr, 
                     std::byte value) const {
    if (!includes(bank_addr, byte_addr)) {
        /* error... */
    }
    if (get(bank_addr, byte_addr).is_set()) {
        /* error... */
    }
    m_banks[translate(bank_addr, byte_addr)] = MemoryCell(value);
}

void MemoryBase::set(std::size_t bank_addr, std::size_t byte_addr,
                     std::initializer_list<MemoryCell> cells) {
    std::size_t i = 0;
    for (MemoryCell const &cell : cells) {
        if (cell.is_set()) {
            set(bank_addr, byte_addr + i, cell.value());
        }
        i++;
    }
}


std::size_t MemoryBase::translate(std::size_t bank_addr, 
                                   std::size_t byte_addr) const {
    return (bank_addr - m_bank_addr_start) * m_bank_size + byte_addr;
}

std::ostream &operator <<(std::ostream &stream, MemoryBase const &mem) {
    stream << std::hex << std::setfill('0');

    bool first = true;
    for (std::size_t i = 0; i < mem.m_n_banks; i++) {
        if (first) {
            first = false;
        } else {
            stream << std::endl;
        }
        
        stream << std::setw(2) << mem.m_bank_addr_start + i << ": ";
        for (std::size_t j = 0; j < mem.m_bank_size; j++) {
            MemoryCell const &cell = mem.get(i, j);
            if (cell.is_set()) {
                stream << std::setw(2) 
                       << static_cast<std::size_t>(cell.value()) << " ";
            } else {
                stream << ".. ";
            }
        }
    }

    return stream;
}
