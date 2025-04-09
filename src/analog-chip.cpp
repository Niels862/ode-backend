#include "analog-chip.hpp"
#include "error.hpp"
#include "util.hpp"
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

void AnalogChip::configure() {
    m_conns.reset();

    for (IOCell &cell : m_io_cells) {
        if (cell.id() % 2 == 1) { // FIXME temp
            cell.use_primary_channel();
        } else {
            cell.use_secondary_channel();
        }
        cell.setup();

        std::cout << "IO" << cell.id() << std::endl;
        for (AnalogBlock const *cab : cell.cab_connections()) {
            std::cout << "  Cab" << cab->id() << std::endl;
        }
    }

    for (AnalogBlock &cab : m_cabs) {
        cab.setup();
    }
}

ShadowSRam AnalogChip::compile() {
    auto ssram = ShadowSRam();

    compile_lut_io_control(ssram);
    compile_io_routing(ssram);
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

void AnalogChip::compile_clocks(ShadowSRam &ssram) {
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

    ssram.set(0x0, 0x0B, 0x40);

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

void AnalogChip::compile_lut_io_control(ShadowSRam &ssram) {
    ssram.set(0x01, 0x02, 0x40); // Unknown
    ssram.set(0x01, 0x1E, { 0x02, 0xFF }); // Unknown
}

void AnalogChip::compile_io_routing(ShadowSRam &ssram) {
    for (std::size_t i = 0; i < m_io_cells.size(); i++) {
        uint8_t mode_data;
        switch (m_io_cells[i].mode()) {
            case IOMode::Disabled:      mode_data = 0x00; break;
            case IOMode::InputBypass:   mode_data = 0x40; break;
            case IOMode::OutputBypass:  mode_data = 0x10; break;
            default:                    
                throw DesignError("Unexpected Mode");
        }

        ssram.set(0x02, 0x13 - 3 * i, mode_data);
    }

    uint8_t routing_lo[4] = { 0 };
    configure_shared_routing(io_cell(1), io_cell(2), routing_lo);

    uint8_t routing_hi[4] = { 0 };
    configure_shared_routing(io_cell(3), io_cell(4), routing_hi);

    ssram.set(0x02, 0x07, from_nibbles(routing_lo[1], routing_lo[0]));
    ssram.set(0x02, 0x05, from_nibbles(routing_lo[3], routing_lo[2]));
    ssram.set(0x02, 0x03, from_nibbles(routing_hi[1], routing_hi[0]));
    ssram.set(0x02, 0x01, from_nibbles(routing_hi[3], routing_hi[2]));
}

void AnalogChip::configure_shared_routing(IOCell &pri, IOCell &sec, 
                                          uint8_t data[4]) {
    auto &conns1 = pri.cab_connections();
    auto &conns2 = sec.cab_connections();
    bool matrix[2][NBlocksPerChip] = { false };

    for (AnalogBlock *cab : conns1) {
        matrix[0][cab->id() - 1] = true;
    }
    for (AnalogBlock *cab : conns2) {
        matrix[1][cab->id() - 1] = true;
    }

    int near_odd, near_even, far_odd, far_even;
    if (pri.id() > 2 && sec.id() > 2) {
        far_odd = 0, far_even = 1, near_odd = 2, near_even = 3;
    } else if (pri.id() <= 2 && sec.id() <= 2) {
        far_odd = 2, far_even = 3, near_odd = 0, near_even = 1;
    } else {
        throw DesignError("err");
    }

    /* If one CAB is far, then both connections are far */
    bool pri_any_far = matrix[0][far_even] || matrix[0][far_odd];
    bool sec_any_far = matrix[1][far_even] || matrix[1][far_odd];

    for (auto &a : matrix) {
        for (auto &e : a) {
            std::cout << e << " ";
        }
        std::cout << std::endl;
    }

    /* For both data fields [0:2] and [2:4] */
    for (int i = 0; i < 2; i++) {
        int cn, cf;
        if (i % 2) {
            cn = near_even, cf = far_even;
        } else {
            cn = near_odd,  cf = far_odd;
        }

        int b1 = 2 * i + 1;
        int b2 = 2 * i;

        bool pri_near = !pri_any_far && matrix[0][cn];
        bool sec_near = !pri_any_far && matrix[1][cn];
        bool pri_far  = pri_any_far && (matrix[0][cn] || matrix[0][cf]);
        bool sec_far  = sec_any_far && (matrix[1][cn] || matrix[1][cf]);

        std::cout << b1 << ": " << pri_far << std::endl;
        if (pri_near) {
            data[b1] = pri.mode() == IOMode::InputBypass ? 0x1 : 0xC;
        } else if (pri_far) {
            data[b1] = 0x5;
        }
        
        if (sec_near) {
            data[b2] = pri.mode() == IOMode::InputBypass ? 0x1 : 0xC;
        } else if (sec_far) {
            data[b2] = 0x5;
        }

        if (data[b1] == data[b2]) {
            switch (data[b1]) {
                case 0x0:
                    break;

                case 0x1:       
                    data[b2] = 0x2; 
                    break;

                case 0x5:       
                    data[b2] = 0x6; 
                    break;

                default:
                    throw DesignError("unsupported");
            }

            std::cout << "pri/sec" << std::endl;
            pri.use_primary_channel();
            sec.use_secondary_channel();
        } else {
            std::cout << "pri/pri" << std::endl;
            pri.use_primary_channel();
            sec.use_primary_channel();
        }
    }
}
