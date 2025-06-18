#include "memory-base.hpp"
#include "error.hpp"
#include <iomanip>
#include <sstream>

MemoryAddress::MemoryAddress()
        : bank_addr{}, byte_addr{} {}

MemoryAddress::MemoryAddress(uint8_t bank_addr, uint8_t byte_addr)
        : bank_addr{bank_addr}, byte_addr{byte_addr} {}

MemoryCell::MemoryCell()
        : m_value{0}, m_is_set{false} {}

MemoryCell::MemoryCell(uint8_t value)
        : m_value{value}, m_is_set{true} {}

MemoryCell::MemoryCell(int value)
        : m_value{uint8_t(value)}, m_is_set{true} {}

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
        std::stringstream ss;
        ss << "illegal access to bank " << std::hex << bank_addr << ", byte " 
           << byte_addr << std::endl;
        throw DesignError(ss.str());
    }
    return m_banks[translate(bank_addr, byte_addr)];
}

void MemoryBase::set(std::size_t bank_addr, std::size_t byte_addr, 
                     uint8_t value) const {
    if (!includes(bank_addr, byte_addr)) {
        std::stringstream ss;
        ss << "illegal access to bank " << std::hex << bank_addr << ", byte " 
           << byte_addr << std::endl;
        throw DesignError(ss.str());
    }
    if (get(bank_addr, byte_addr).is_set()) {
        std::stringstream ss;
        ss << "Memory at " << std::hex << bank_addr << ":" 
           << byte_addr << std::dec << " already written";
        throw DesignError(ss.str());
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

void MemoryBase::to_data_bytestream(std::vector<uint8_t> &data) const {
    constexpr int SectionHeaderSize = 4;

    std::vector<std::vector<uint8_t>> sections;
    std::vector<uint8_t> section, data_section;

    for (std::size_t i = 0; i < size(); /* manual increment */) {
        if (m_banks[i].value() == 0) {
            i++;
            continue;
        }
    
        section.clear();
    
        std::size_t null_count = 0;
        std::size_t start = i;
        std::size_t j = i;
    
        for (; j < size() && null_count <= SectionHeaderSize && j - start <= 256; j++) {
            if (m_banks[j].value() == 0) {
                null_count++;
            } else {
                null_count = 0;
            }
    
            section.push_back(m_banks[j].value());
        }
    
        // Trim trailing zeros
        while (!section.empty() && section.back() == 0) {
            section.pop_back();
        }
    
        std::size_t byte_addr = start % m_bank_size + m_bank_addr_start;
        std::size_t bank_addr = start / m_bank_size;
        std::size_t size = section.size();
    
        data_section.clear();
        data_section.push_back(byte_addr);
        data_section.push_back(bank_addr);
        data_section.push_back(size == 256 ? 0 : size);
    
        for (uint8_t entry : section) {
            data_section.push_back(entry);
        }
    
        data_section.push_back(0x2A);
        sections.push_back(data_section);
    
        // Update i to point to next candidate
        i = j;
    }
    
    for (std::size_t i = 0; i < sections.size(); i++) {
        bool last = i == sections.size() - 1;
        section = sections[i];

        if (last) {
            section[0] |= 0b1000'0000;
        } else {
            section[0] |= 0b1100'0000;
        }   

        for (uint8_t entry : section) {
            data.push_back(entry);
        }
    }
}

std::size_t MemoryBase::translate(std::size_t bank_addr, 
                                  std::size_t byte_addr) const {
    return (bank_addr - m_bank_addr_start) * m_bank_size + byte_addr;
}

std::ostream &operator <<(std::ostream &stream, MemoryBase const &mem) {
    stream << std::hex << std::setfill('0') << std::uppercase;

    stream << "    ";
    for (std::size_t i = 0; i < mem.m_bank_size; i++) {
        stream << std::setw(2) << i << " ";
    }
    stream << std::endl;

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

    stream << std::dec;

    return stream;
}
