#ifndef OBC_COMPARATOR_HPP
#define OBC_COMPARATOR_HPP

#include "io-port.hpp"
#include "shadow-sram.hpp"

class AnalogBlock;
class AnalogModule;

class Comparator {
public:
    Comparator();

    Comparator &claim(AnalogModule &module);
    Comparator &set_configuration(std::array<uint8_t, 2> cfg);

    void compile(AnalogBlock const &cab, ShadowSRam &ssram) const;

    bool is_used() const { return m_module != nullptr; }

    InputPort &in() { return m_in; }

private:
    AnalogModule *m_module;

    InputPort m_in;
    
    /* Comparator output can be implemented in the future but this requires more complex routing and is currently out of scope */
    //OutputPort m_out; 

    std::array<uint8_t, 2> m_cfg;
};

#endif
