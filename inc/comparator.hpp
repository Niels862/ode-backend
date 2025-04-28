#ifndef OBC_COMPARATOR_HPP
#define OBC_COMPARATOR_HPP

#include "shadow-sram.hpp"

class AnalogBlock;

class Comparator {
public:
    Comparator();

    Comparator &claim();

    void compile(AnalogBlock const &cab, ShadowSRam &ssram) const;

    bool is_used() const { return m_is_used; }

private:
    bool m_is_used;
};

#endif
