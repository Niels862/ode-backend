#include "analog-chip.hpp"
#include "error.hpp"
#include <sstream>

AnalogChip::AnalogChip()
        : m_cabs{}, m_null_cab{0}, m_io_cells{} {
    for (std::size_t i = 0; i < NBlocksPerChip; i++) {
        m_cabs[i] = AnalogBlock(i + 1);
    }

    for (std::size_t i = 0; i < NType1IOCellsPerChip; i++) {
        m_io_cells[i].initialize(i + 1, m_null_cab);
    }
}

void AnalogChip::setup() {
    m_conns.reset();

    for (AnalogBlock &cab : m_cabs) {
        cab.setup();
    }

    std::cout << conns().bitset() << std::endl;
}

ShadowSRam AnalogChip::compile() const {
    auto ssram = ShadowSRam();

    compile_clocks(ssram);
    for (AnalogBlock const &cab : m_cabs) {
        cab.compile(ssram);
    }

    return ssram;
}

void AnalogChip::to_header_bytestream(std::vector<uint8_t> &data) const {
    uint8_t header[] = {
        0xD5, /* Synch     */
        0xB7, /* JTAG0     */
        0x20, /* JTAG1     */
        0x01, /* JTAG2     */
        0x00, /* JTAG3     */
        0x01, /* Address1  */
        0xC1, /* Control   */
    };
 
    for (uint8_t entry : header) {
        data.push_back(entry);
    }
}

void AnalogChip::compile_clocks(ShadowSRam &ssram) const {
    const int ACLK = 16'000;
    const int Sys1 = ACLK;
    //const int Sys2 = ACLK;

    const std::array<int, 6> Clocks = {
        4'000,
        16'000,
        2'000,
        250,
        16'000,
        16'000,
    };

    const std::array<bool, 6> ClocksUsed = {
        true,
        false,
        false,
        false,
        false,
        false,
    };

    std::array<uint8_t, 6> data;
    for (std::size_t i = 0; i < Clocks.size(); i++) {
        int clock = Clocks[i];

        if (Sys1 == clock) {
            data[i] = 0;
        } else {
            if (Sys1 % (2 * clock) != 0) {
                std::stringstream ss;
                ss << "Cannot realize value of Clock " << i << ": "
                   << clock;
                throw DesignError(ss.str());
            }
    
            data[i] = Sys1 / clock / 2;
        }
    }

    uint8_t data_used = 1 | (0 << 1);
    for (std::size_t i = 0; i < ClocksUsed.size(); i++) {
        data_used |= ClocksUsed[i] << (2 + i);
    }

    ssram.set(0x0, 0x08, data_used);

    ssram.set(0x0, 0x07, data[0]);
    ssram.set(0x0, 0x06, data[1]);
    ssram.set(0x0, 0x05, data[2]);
    ssram.set(0x0, 0x04, data[3]);
    ssram.set(0x0, 0x03, data[4]);
    ssram.set(0x0, 0x02, 0x0); // Clock 4 offset = 0
    ssram.set(0x0, 0x01, data[5]);
    ssram.set(0x0, 0x00, 0x0); // Clock 5 offset = 0

    ssram.set(0x0, 0x0E, { 0x51, 0xFF, 0x0F, 0xF1}); // Unknown
}
