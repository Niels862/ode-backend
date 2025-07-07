#include "analog-chip.hpp"
#include "error.hpp"
#include "util.hpp"
#include "settings.hpp"
#include <sstream>
#include <cassert>

AnalogChip::AnalogChip()
        : m_cabs{}, m_null_cab{}, m_io_cells{}, 
          m_clocks{}, m_null_clock{}, m_intercam_channels{} {
    m_null_cab.initialize(0, *this);
    for (std::size_t i = 0; i < NBlocksPerChip; i++) {
        m_cabs[i].initialize(i + 1, *this);
    }

    for (std::size_t i = 0; i < NType1IOCellsPerChip; i++) {
        m_io_cells[i].initialize(i + 1, m_null_cab);
    }

    m_clocks = {
        Clock(1,  4'000, 0),
        Clock(2, 16'000, 0),
        Clock(3,  2'000, 0),
        Clock(4,    250, 0),
        Clock(5, 16'000, 0),
        Clock(6, 16'000, 0)
    };

    for (AnalogBlock &from : m_cabs) {
        for (AnalogBlock &to : m_cabs) {
            if (from.id() == to.id()) {
                continue;
            }
            for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
                intercam_channel(from, to, side) = Channel::InterCab(side, from, to);
            }
        }
    }

    for (CabColumn group : { CabColumn::OddCabs, CabColumn::EvenCabs }) {
        for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
            global_bi_indirect(group, side) = Channel::GlobalBiIndirect(side, group);
        }
    }

    for (IOGroup group : { IOGroup::LowIO, IOGroup::HighIO }) {
        for (AnalogBlock &cab : m_cabs) {
            for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
                global_input_direct(group, cab, side) = Channel::GlobalInputDirect(side, group, cab);
                global_output_direct(group, cab, side) = Channel::GlobalOutputDirect(side, group, cab);
            }
        }
    }
}

ShadowSRam AnalogChip::compile() {
    auto ssram = ShadowSRam();

    for (Clock &clock : m_clocks) {
        clock.set_is_used(false);
    }

    for (IOCell &cell : m_io_cells) {
        cell.finalize();
    }

    compile_lut_io_control(ssram);
    compile_io_routing(ssram);

    for (AnalogBlock &cab : m_cabs) {
        if (args.verbose) {
            std::cerr << "Finalizing CAB-" << cab.id() << "..." << std::endl;
        }
        cab.finalize();
    }

    for (AnalogBlock &cab : m_cabs) {
        cab.compile(ssram);
    }

    compile_clocks(ssram);

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

Channel &AnalogChip::global_input_direct(IOGroup from, AnalogBlock &to, 
                                         Channel::Side side) {
    return m_global_input_direct_channels
        .at(static_cast<int>(from))
        .at(to.id() - 1)
        .at(static_cast<int>(side));
} 

Channel &AnalogChip::global_output_direct(IOGroup to, AnalogBlock &from,
                                          Channel::Side side) {
    return m_global_output_direct_channels
        .at(static_cast<int>(to))
        .at(from.id() - 1)
        .at(static_cast<int>(side));
}

Channel &AnalogChip::global_bi_indirect(CabColumn group, Channel::Side side) {
    return m_global_bi_indirect_channels
        .at(static_cast<int>(group))
        .at(static_cast<int>(side));
}

Channel &AnalogChip::intercam_channel(AnalogBlock &from, AnalogBlock &to, 
                                      Channel::Side side) {
    assert(from.id() != to.id());
    assert(from.id() > 0);
    assert(to.id() > 0);

    return m_intercam_channels
        .at(from.id() - 1)
        .at(to.id() - 1)
        .at(static_cast<int>(side));
}

void AnalogChip::compile_clocks(ShadowSRam &ssram) {
    const int ACLK = 16'000;
    const int Sys1 = ACLK;

    ssram.set(0x0, 0x0B, 0x40);

    uint8_t data_used = 1 | (0 << 1);
    for (std::size_t i = 0; i < m_clocks.size(); i++) {
        data_used |= clock(i + 1).is_used() << (2 + i);
    }
    ssram.set(0x0, 0x08, data_used);

    for (Clock &clock : m_clocks) {
        clock.compile(ssram, Sys1);
    }

    ssram.set(0x0, 0x0E, { 0x51, 0xFF, 0x0F, 0xF1}); // Unknown
}

void AnalogChip::compile_lut_io_control(ShadowSRam &ssram) {
    ssram.set(0x01, 0x02, 0x40); // Unknown
    ssram.set(0x01, 0x1E, { 0x02, 0xFF }); // Unknown
}

void AnalogChip::compile_io_routing(ShadowSRam &ssram) {
    //   dd cc bb aa
    // ~ CD CD AB AB

    uint8_t io_routing[4] = { 0 };

    for (IOCell &cell : m_io_cells) {
        ssram.set(0x02, 0x16 - 3 * cell.id(), static_cast<int>(cell.mode()));

        IOGroup io_group = Channel::to_io_group(cell);
        int group_entry = cell.id() % 2;

        for (CabColumn cab_group : { CabColumn::OddCabs, CabColumn::EvenCabs }) {
            std::size_t idx = 2 * static_cast<int>(io_group) 
                            + static_cast<int>(cab_group);
            uint8_t select = cell.used_channel(cab_group).io_routing_selector();

            if (group_entry == 0) {
                io_routing[idx] |= select;
            } else {
                io_routing[idx] |= select << 4;
            }
        }
    }

    for (std::size_t i = 0; i < 4; i++) {
        std::size_t addr = 0x07 - i * 2;
        ssram.set(0x02, addr, io_routing[i]);
    }
}
