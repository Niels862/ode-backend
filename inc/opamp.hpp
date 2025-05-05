#ifndef OBC_OPAMP_HPP
#define OBC_OPAMP_HPP

#include "io-port.hpp"
#include "shadow-sram.hpp"

class AnalogBlock;
class AnalogModule;

class OpAmp {
public:
    OpAmp();
    OpAmp(int id);

    OpAmp &claim(AnalogModule &module);
    OpAmp &set_closed_loop(bool closed_loop);

    void compile(AnalogBlock const &cab, ShadowSRam &ssram) const;

    int id() const { return m_id; }
    bool is_used() const { return m_module != nullptr; }

    OutputPort &out() { return m_out; }

private:
    int m_id;
    AnalogModule *m_module;

    bool m_closed_loop;

    OutputPort m_out;
};

#endif
