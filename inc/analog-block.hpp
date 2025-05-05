#ifndef OBC_ANALOG_BLOCK_HPP
#define OBC_ANALOG_BLOCK_HPP

#include "analog-module.hpp"
#include "capacitor.hpp"
#include "opamp.hpp"
#include "comparator.hpp"
#include "clock.hpp"
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
    AnalogBlock(int id, Clock &pri_clock, Clock &sec_clock);

    void setup(Clock &clk_a, Clock &clk_b);

    template <typename T>
    T &add(T *module) {
        m_modules.push_back(std::unique_ptr<T>(module));
        module->set_cab(*this);
        module->claim_components();
        return *module;
    }

    Capacitor &claim_cap(AnalogModule &module);
    OpAmp &claim_opamp(AnalogModule &module);
    Comparator &claim_comp(AnalogModule &module);

    void finalize();

    void compile(ShadowSRam &ssram) const;

    std::size_t bank_a() const { return 2 * m_id + 1; }
    std::size_t bank_b() const { return bank_a() + 1; }

    int id() const { return m_id; }

    Capacitor &cap(int id) { return m_caps.at(id - 1); }
    OpAmp &opamp(int id) { return m_opamps.at(id - 1); }
    Comparator &comp() { return m_comp; }

    void set_used_clock(int i, Clock &clock);

    void log_resources() const;

    operator bool() const { return m_id > 0; }

private:
    int m_id;

    std::array<Capacitor, NCapacitorsPerBlock> m_caps;
    std::size_t m_next_cap;

    std::array<OpAmp, NOpAmpsPerBlock> m_opamps;
    std::size_t m_next_opamp;

    Comparator m_comp;

    std::array<Clock *, 2> m_used_clocks;

    std::vector<std::unique_ptr<AnalogModule>> m_modules;
};

#endif
