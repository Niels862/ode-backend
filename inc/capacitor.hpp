#ifndef OBC_CAPACITOR_HPP
#define OBC_CAPACITOR_HPP

#include "shadow-sram.hpp"
#include <initializer_list>
#include <cstddef>
#include <cstdint>

class AnalogBlock;

class OpAmp;

class InputPort;

struct SwitchConfiguration {
    uint8_t b1;
    uint8_t b2;
};

class Capacitor {
public:
    Capacitor();
    Capacitor(int id);

    static SwitchConfiguration from_input(InputPort &port,
                                          int sg_phase = 0);

    static SwitchConfiguration from_opamp(OpAmp const &opamp, 
                                          int sg_phase = 0);
    static SwitchConfiguration to_opamp(OpAmp const &opamp, 
                                        int sg_phase = 0);

    Capacitor &claim(int value);
    Capacitor &set_in(SwitchConfiguration switch_cfg);
    Capacitor &set_out(SwitchConfiguration switch_cfg);

    void compile(AnalogBlock const &cab, ShadowSRam &ssram) const;

    int id() const { return m_id; }
    bool is_used() const { return m_is_used; }

private:
    MemoryAddress switch_address(AnalogBlock const &cab) const;
    MemoryAddress value_address(AnalogBlock const &block) const;

    int m_id;
    bool m_is_used;

    uint8_t m_value;
    uint8_t m_switch_cfg[4];
};

#endif
