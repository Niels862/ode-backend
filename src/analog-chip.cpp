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
    return m_global_input_direct_channels
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

#if 0

    uint8_t routing_lo[4] = { 0 };
    configure_shared_routing(io_cell(1), io_cell(2), routing_lo);

    uint8_t routing_hi[4] = { 0 };
    configure_shared_routing(io_cell(3), io_cell(4), routing_hi);

    ssram.set(0x02, 0x07, from_nibbles(routing_lo[1], routing_lo[0]));
    ssram.set(0x02, 0x05, from_nibbles(routing_lo[3], routing_lo[2]));
    ssram.set(0x02, 0x03, from_nibbles(routing_hi[1], routing_hi[0]));
    ssram.set(0x02, 0x01, from_nibbles(routing_hi[3], routing_hi[2]));
}
#endif

static void setup_connection_matrix(IOCell &cell, Connection *conns[2][2]) {
    for (auto &conn : cell.connections()) {
        if (conn.block == Connection::None) {
            continue;
        }

        int cid = conn.cab->id();
        switch (cid) {
            case 1: conns[0][0] = &conn; break;
            case 2: conns[1][0] = &conn; break;
            case 3: conns[0][1] = &conn; break;
            case 4: conns[1][1] = &conn; break;

            default:
                abort(); // todo 
        }
    }
}

static void promote_near_connections(Connection *conns[2][2], bool c1c2_near) {
    int far  = c1c2_near ? 1 : 0;
    
    if (!conns[0][far] && !conns[1][far]) {
        return;
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (conns[i][j]) {
                conns[i][j]->mode = Connection::Far;
            }
        }
    }
}

static Connection *get_local_representative(Connection *lconns[2]) {
    Connection *repr = nullptr;

    for (std::size_t i = 0; i < 2; i++) {
        if (!lconns[i]) {
            continue;
        }

        if (!repr) {
            repr = lconns[i];
        } else if (!repr->equivalent(*lconns[i])) {
            /* This is unexpected, because if both exist, then they must 
               both be Far according to promote_near_connections. */
            abort(); 
        }
    }

    return repr;
}

static void resolve_local_connection_conflicts(Connection *lconns1[2], 
                                               Connection *lconns2[2]) {
    auto repr1 = get_local_representative(lconns1);
    auto repr2 = get_local_representative(lconns2);

    if (!repr1 || !repr2 || !repr1->equivalent(*repr2)) {
        return;
    }

    if (lconns2[0]) {
        lconns2[0]->channel = Connection::Secondary;
    }
    if (lconns2[1]) {
        lconns2[1]->channel = Connection::Secondary;
    }
}

static void initialize_routing_data(Connection *conns[2], uint8_t &entry) {
    Connection *repr = get_local_representative(conns);

    if (repr) {
        entry = repr->io_nibble();
    } else {
        entry = 0x0;
    }
}

/*
io1 and io2 are two IOCells that share routing to and from CABs in two bytes,
which are represented as four nibbles in data[4]. This array has the following 
structure:

[0] = io2 to c1/c3
[1] = io1 to c1/c3
[2] = io2 to c2/c4
[3] = io1 to c1/c3

In this function, this array is configured, as well as the connections
to the CABs in io1 & io2. The follwing constraints are present.

- any connection to c3/c4 is equivalent for Input/Output. In this case, this
  is set using CELL.connection(CAB).mode = Connection::Far (default is Near).
  * if a connection to c1/c2 respectively also exists, then this should be
    configured exactly like above
- if ioX is connected to c3/c4 as well as to c1/c2, then the connection to 
  c1/c2 is also encoded as Far and should be handled as above. 
- if two connections are conflicting, then one of the commections should be 
  switched to the secondary channel. This is handled by 
  CELL.connection(CAB).channel = Connection::Secondary (default is Primary).

A conflict has the following constraints:
- the data cells where the two connections will be placed are either 
  (data[0], data[1]), or (data[2], data[3]). 
- the two connections have the same kind: CELL.connection(CAB).kind

the nibbles are determined by CELL.connection(CAB).io_nibble().
*/
void AnalogChip::configure_shared_routing(IOCell &io1, IOCell &io2, 
                                          uint8_t data[4]) {
    Connection *conns1[2][2] = {};
    Connection *conns2[2][2] = {};

    setup_connection_matrix(io1, conns1); 
    setup_connection_matrix(io2, conns2);

    bool c1c2_near;
    if (io1.id() == 1 && io2.id() == 2) {
        c1c2_near = true;
    } else if (io1.id() == 3 && io2.id() == 4) {
        c1c2_near = false;
    } else {
        abort();
    }

    promote_near_connections(conns1, c1c2_near);
    promote_near_connections(conns2, c1c2_near);

    resolve_local_connection_conflicts(conns1[0], conns2[0]);
    resolve_local_connection_conflicts(conns1[1], conns2[1]);

    initialize_routing_data(conns1[0], data[1]);
    initialize_routing_data(conns2[0], data[0]);
    initialize_routing_data(conns1[1], data[3]);
    initialize_routing_data(conns2[1], data[2]);
}
