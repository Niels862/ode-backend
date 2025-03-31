#ifndef OBC_OPAMP_HPP
#define OBC_OPAMP_HPP

#include "shadow-sram.hpp"

class AnalogBlock;

class OpAmp {
public:
    OpAmp();
    OpAmp(int id);

    OpAmp &claim(bool closed_loop);

    void compile(AnalogBlock const &cab, ShadowSRam &ssram) const;

    int id() const { return m_id; }
    bool is_used() const { return m_is_used; }

private:
    int m_id;
    bool m_is_used;

    bool m_closed_loop;
};

#endif
