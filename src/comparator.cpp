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
    IOCell *cell = port.io_connection();
    if (!cell) {
        throw DesignError("No connected to Comparator");
    }

    Connection *conn = &cell->connection(port.cab());
    if (conn->mode == Connection::Mode::Far) {
        return true;
    }
    return false;
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

#define TEMP_I2SEC_TO_COMPC1 0x80

void Comparator::compile(AnalogBlock const &cab, ShadowSRam &ssram) {
    ssram.set(cab.bank_b(), 0x09, { m_cfg[0], m_cfg[1] });
    ssram.set(cab.bank_a(), 0x0E, compile_route(*this));

    if (m_module) {
        ssram.set(cab.bank_b(), 0x06, TEMP_I2SEC_TO_COMPC1);
    } else {
        ssram.set(cab.bank_b(), 0x06, 0x00);
    }
}
