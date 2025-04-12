#ifndef OBC_ANALOG_BLOCK_HPP
#define OBC_ANALOG_BLOCK_HPP

#include "analog-module.hpp"
#include "capacitor.hpp"
#include "opamp.hpp"
#include "defs.hpp"
#include "shadow-sram.hpp"
#include <array>
#include <cstddef>
#include <iostream>
#include <vector>
#include <memory>

class AnalogChip;

class AnalogBlock {
public:
    AnalogBlock();
    AnalogBlock(int id);

    template <typename T>
    T &add(T *module) {
        m_modules.push_back(std::unique_ptr<T>(module));
        module->set_cab(*this);
        return *module;
    }

    void configure();

    void compile(ShadowSRam &ssram) const;

    std::size_t bank_a() const { return 2 * m_id + 1; }
    std::size_t bank_b() const { return bank_a() + 1; }

    int id() const { return m_id; }

    Capacitor &cap(int id) { return m_caps.at(id - 1); }
    OpAmp &opamp(int id) { return m_opamps.at(id - 1); }

    operator bool() const { return m_id > 0; }

private:
    int m_id;

    std::array<Capacitor, NCapacitorsPerBlock> m_caps;
    std::array<OpAmp, NOpAmpsPerBlock> m_opamps;

    std::vector<std::unique_ptr<AnalogModule>> m_modules;
};

#endif
