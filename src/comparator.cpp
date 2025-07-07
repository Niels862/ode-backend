#include "comparator.hpp"
#include "error.hpp"
#include "analog-block.hpp"
#include "io-cell.hpp"
#include <stdio.h>

Comparator::Comparator(AnalogBlock &cab)
        : m_module{}, m_in{cab, InPortSource::Comparator}, 
          m_cfg{} {}

Comparator &Comparator::claim(AnalogModule &module) {
    if (m_module) {
        throw DesignError("Comparator already claimed");
    }

    m_module = &module;

    return *this;
}

Comparator &Comparator::set_configuration(std::array<uint8_t, 2> cfg) {
    m_cfg = cfg;

    return *this;
}

static bool is_internally_routed(InputPort &port) {
    PortLink *link = port.link();
    if (!link) {
        return false;
    }

    Channel *channel = link->channels.back();
    return channel->type == Channel::Type::LocalInput;
}

static uint8_t compile_route(Comparator &comp) {
    if (!comp.is_used()) {
        return 0x00;
    }

    if (is_internally_routed(comp.in())) {
        return 0x48;
    }

    return 0x08;
}

void Comparator::finalize() {
    if (!m_module) {
        return;
    }

    AnalogBlock &cab = m_module->cab();

    PortLink *link = m_in.link();
    if (!link) {
        return;
    }

    OutputPort &out = *link->out;
    if (out.source() != OutPortSource::IOCell) {
        return;
    }

    if (Channel::uses_direct_channel(out.io_cell(), cab)) {
        return;
    }

    cab.local_input_channel(Channel::Primary).reserve(out);
}

void Comparator::compile(AnalogBlock const &cab, ShadowSRam &ssram) {
    ssram.set(cab.bank_b(), 0x09, { m_cfg[0], m_cfg[1] });
    ssram.set(cab.bank_a(), 0x0E, compile_route(*this));
    ssram.set(cab.bank_b(), 0x06, m_in.comparator_connection_selector());
}
