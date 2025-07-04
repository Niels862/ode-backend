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
class IOCell;

class AnalogBlock {
public:
    AnalogBlock();
    
    /* Delete copy/move semantics as this breaks links with Ports. */
    AnalogBlock(AnalogBlock const &) = delete;
    AnalogBlock &operator=(AnalogBlock const &) = delete;
    
    AnalogBlock(AnalogBlock &&) = delete;
    AnalogBlock &operator=(AnalogBlock &&) = delete;

    void initialize(int id, AnalogChip &chip);

    void setup(Clock &clk_a, Clock &clk_b);

    template <typename Module>
    Module &add(Module *module) {
        add_raw(module)->claim_components();
        return *module;
    }

    template <typename Module>
    Module *add_raw(Module *module) {
        m_modules.push_back(std::unique_ptr<Module>(module));
        module->set_cab(*this);
        return module;
    }

    InputPort &claim_in(AnalogModule &module);
    Capacitor &claim_cap(AnalogModule &module);
    OpAmp &claim_opamp(AnalogModule &module);
    Comparator &claim_comp(AnalogModule &module);

    void finalize();

    void compile(ShadowSRam &ssram);

    std::size_t bank_a() const { return 2 * m_id + 1; }
    std::size_t bank_b() const { return bank_a() + 1; }

    int id() const { return m_id; }

    Capacitor &cap(int id) { return m_caps.at(id - 1); }
    OpAmp &opamp(int id) { return m_opamps.at(id - 1); }
    Comparator &comp() { return m_comp; }

    void set_used_clock(int i, Clock &clock);
    Clock &get_clock(Clock::Select select) {
        if (select == Clock::A) {
            return *m_used_clocks[0];
        } else {
            return *m_used_clocks[1];
        }
    }

    AnalogChip &chip() { return *m_chip; }

    Channel &local_opamp_channel(Channel::Side side);
    Channel &local_input_channel(Channel::Side side);
    Channel &local_output_channel(Channel::Side side);

    void log_resources() const;

    bool operator ==(AnalogBlock &other) { return m_id == other.m_id; }
    operator bool() const { return m_id > 0; }

private:
    void map_internal_channels();
    uint8_t compile_internal_channel_routing(IOCell *channel);

    AnalogChip *m_chip;

    int m_id;
    bool m_set_up;

    std::array<InputPort, 8> m_local_ins;
    std::size_t m_next_local_in;

    std::array<Capacitor, NCapacitorsPerBlock> m_caps;
    std::size_t m_next_cap;

    std::array<OpAmp, NOpAmpsPerBlock> m_opamps;
    std::size_t m_next_opamp;

    Comparator m_comp;

    std::array<Clock *, 2> m_used_clocks;
    IOCell *m_internal_P;
    IOCell *m_internal_Q;

    std::array<Channel, 2> m_local_opamp_channels;
    std::array<Channel, 2> m_local_input_channels;
    std::array<Channel, 2> m_local_output_channels;

    std::vector<std::unique_ptr<AnalogModule>> m_modules;
};

#endif
