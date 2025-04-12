#include "analog-chip.hpp"
#include "error.hpp"
#include "util.hpp"
#include "settings.hpp"
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

ShadowSRam AnalogChip::compile() {
    auto ssram = ShadowSRam();

    for (IOCell &cell : m_io_cells) {
        cell.configure();
    }

    compile_lut_io_control(ssram);
    compile_io_routing(ssram);
    compile_clocks(ssram);

    for (AnalogBlock &cab : m_cabs) {
        if (args.verbose) {
            std::cerr << "Configuring CAB-" << cab.id() << "..." << std::endl;
        }
        cab.configure();
    }

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

void setup_connection_matrix(IOCell &cell, Connection *conns[2][2]) {
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

void promote_near_connections(Connection *conns[2][2]) {
    if (conns[0][1] || conns[1][1]) {
        if (conns[0][0]) {
            conns[0][0]->mode = Connection::Far;
        }
        if (conns[1][0]) {
            conns[1][0]->mode = Connection::Far;
        }
    }
}

Connection *get_local_representative(Connection *lconns[2]) {
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

void resolve_local_connection_conflicts(Connection *lconns1[2], 
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

void initialize_routing_data(Connection *conns[2], uint8_t &entry) {
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

    promote_near_connections(conns1); // FIXME: works for io1&op2, but things need to be reversed for io3&io4
    promote_near_connections(conns2);

    resolve_local_connection_conflicts(conns1[0], conns2[0]);
    resolve_local_connection_conflicts(conns1[1], conns2[1]);

    initialize_routing_data(conns1[0], data[1]);
    initialize_routing_data(conns2[0], data[0]);
    initialize_routing_data(conns1[1], data[3]);
    initialize_routing_data(conns2[1], data[2]);
}
