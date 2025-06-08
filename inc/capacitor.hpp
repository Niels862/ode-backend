#ifndef OBC_CAPACITOR_HPP
#define OBC_CAPACITOR_HPP

#include "clock.hpp"
#include "switch.hpp"
#include "shadow-sram.hpp"
#include <initializer_list>
#include <cstddef>
#include <cstdint>

class AnalogBlock;
class AnalogModule;

class OpAmp;

class InputPort;

class Capacitor {
public:
    Capacitor();
    Capacitor(int id);

    static SwitchConfiguration from_input(InputPort &port,
                                          int sg_phase = 0,
                                          Clock::Select select = Clock::A);

    static SwitchConfiguration from_opamp(OpAmp const &opamp, 
                                          int sg_phase = 0,
                                          Clock::Select select = Clock::A);
    static SwitchConfiguration to_opamp(OpAmp const &opamp, 
                                        int sg_phase = 0,
                                        Clock::Select select = Clock::A);

    Capacitor &claim(AnalogModule &module);
    Capacitor &set_value(uint8_t value);
    Capacitor &set_in(SwitchConfiguration switch_cfg);
    Capacitor &set_out(SwitchConfiguration switch_cfg);

    void compile(AnalogBlock const &cab, ShadowSRam &ssram) const;

    int id() const { return m_id; }
    bool is_used() const { return m_module != nullptr; }

private:
    MemoryAddress switch_address(AnalogBlock const &cab) const;
    MemoryAddress value_address(AnalogBlock const &block) const;

    int m_id;
    AnalogModule *m_module;

    uint8_t m_value;
    std::array<uint8_t, 4> m_switch_cfg;
};

#endif
