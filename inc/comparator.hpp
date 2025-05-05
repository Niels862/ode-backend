#ifndef OBC_COMPARATOR_HPP
#define OBC_COMPARATOR_HPP

#include "shadow-sram.hpp"

class AnalogBlock;
class AnalogModule;

class Comparator {
public:
    Comparator();

    Comparator &claim(AnalogModule &module);

    void compile(AnalogBlock const &cab, ShadowSRam &ssram) const;

    bool is_used() const { return m_module != nullptr; }

private:
    AnalogModule *m_module;
};

#endif
