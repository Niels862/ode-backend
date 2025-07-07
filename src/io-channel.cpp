#include "io-channel.hpp"
#include "io-port.hpp"
#include "analog-block.hpp"
#include "io-cell.hpp"
#include <sstream>
#include <stdexcept>
#include <cassert>

Channel::Channel()
        : type{}, side{}, driver{nullptr} {}

Channel::Channel(Channel::Type type, Channel::Side side)
        : type{type}, side{side}, driver{nullptr} {}

Channel Channel::IntraCab(Side side) {
    return Channel(Channel::Type::IntraCab, side);
}

Channel &Channel::None() {
    static Channel channel;
    return channel;
}

Channel Channel::GlobalInputDirect(Side side, IOGroup from, AnalogBlock &to) {
    Channel channel(Channel::Type::GlobalInputDirect, side);

    auto &data = channel.data.global_input_direct;
    data.from = from;
    data.cab_to = to.id();

    return channel;
}

Channel Channel::GlobalOutputDirect(Side side, IOGroup to, AnalogBlock &from) {
    Channel channel(Channel::Type::GlobalOutputDirect, side);

    auto &data = channel.data.global_output_direct;
    data.to = to;
    data.cab_from = from.id();

    return channel;
}

Channel Channel::GlobalBiIndirect(Side side, CabColumn group) {
    Channel channel(Channel::Type::GlobalBiIndirect, side);

    auto &data = channel.data.global_bi_indirect;
    data.group = group;

    return channel;
}

Channel Channel::InterCab(Side side, AnalogBlock &from, AnalogBlock &to) {
    Channel channel(Channel::Type::InterCab, side);

    auto &data = channel.data.inter_cab;
    data.cab_from_id = from.id();
    data.cab_to_id = to.id();

    return channel;
}

Channel Channel::LocalInput(Side side) {
    Channel channel(Channel::Type::LocalInput, side);

    auto &data = channel.data.local_input;
    data.source = nullptr;

    return channel;
}

Channel Channel::LocalOutput(Side side) {
    Channel channel(Channel::Type::LocalOutput, side);

    auto &data = channel.data.local_output;
    data.dest = nullptr;

    return channel;
}

CabRow Channel::to_cab_row(AnalogBlock &cab) {
    switch (cab.id()) {
        case 1:
        case 2:
            return CabRow::LowCabs;

        case 3:
        case 4:
            return CabRow::HighCabs;
    }

    abort();
}

CabColumn Channel::to_cab_column(AnalogBlock &cab) {
    switch (cab.id()) {
        case 1: 
        case 3:
            return CabColumn::OddCabs;

        case 2:
        case 4:
            return CabColumn::EvenCabs;
    }

    abort();
}

IOGroup Channel::to_io_group(IOCell &io_cell) {
    switch (io_cell.id()) {
        case 1:
        case 2:
            return IOGroup::LowIO;

        case 3:
        case 4:
            return IOGroup::HighIO;
    }

    abort();
}

bool Channel::uses_direct_channel(IOCell &cell, AnalogBlock &cab) {
    assert(cab.id() >= 1 && cab.id() <= 4);

    IOGroup group = Channel::to_io_group(cell);

    switch (group) {
        case IOGroup::LowIO:
            return cab.id() == 1 || cab.id() == 2;

        case IOGroup::HighIO:
            return cab.id() == 3 || cab.id() == 4;
    }

    abort();
}

Channel::Side Channel::source_to_side(OutPortSource source) {
    assert(source == OutPortSource::OpAmp1 || source == OutPortSource::OpAmp2);

    if (source == OutPortSource::OpAmp1) {
        return Channel::Primary;
    } else {
        return Channel::Secondary;
    }
}

bool Channel::available(PortLink &link) {
    if (allocated_for(link)) {
        return true;
    }

    if (driver == nullptr) {
        return true;
    }

    return false;
}

bool Channel::allocated_for(PortLink &link) {
    return driver == link.out;
}

Channel &Channel::allocate(PortLink &link) {
    if (available(link)) {
        link.channels.push_back(this);
        driver = link.out;
    } else {
        std::stringstream ss;
        ss << "Cannot allocate Channel " << *this << " for Link " << link;
        throw std::runtime_error(ss.str());
    }

    return *this;
}

void Channel::set_local_input_source(Channel &source) {
    assert(type == Channel::Type::LocalInput);
    assert(source.type == Channel::Type::GlobalBiIndirect);
    assert(source.driver != nullptr);

    data.local_input.source = &source;
}

void Channel::set_local_output_dest(Channel &dest) {
    assert(driver != nullptr);
    assert(type == Channel::Type::LocalOutput);
    assert(dest.type == Channel::Type::GlobalBiIndirect);
    assert(dest.driver == driver);

    data.local_output.dest = &dest;
}

Channel *Channel::local_input_source() {
    assert(type == Channel::Type::LocalInput);

    return data.local_input.source;
}

Channel *Channel::local_output_dest() {
    assert(type == Channel::Type::LocalOutput);

    return data.local_output.dest;
}

uint8_t Channel::io_routing_selector() const {
    if (type == Channel::Type::None) {
        return 0x0;
    }

    switch (type) {
        case Channel::Type::None:
            break;

        case Channel::Type::GlobalInputDirect:
            if (side == Channel::Primary) {
                return 0x1;
            } else {
                return 0x2;
            }
            
        case Channel::Type::GlobalOutputDirect:
            if (side == Channel::Primary) {
                return 0xC;
            } else {
                return 0x8;
            }

        case Channel::Type::GlobalBiIndirect:
            if (side == Channel::Primary) {
                return 0x5;
            } else {
                return 0x6;
            }

        default:
            break;
    }

    abort();
}

static uint8_t switch_intercab_selector(int from, int to) {
    assert(from > 0 && from <= NBlocksPerChip);
    assert(to > 0 && to <= NBlocksPerChip);
    assert(from != to);

    switch (from) {
        case 1:
            switch (to) {
                case 2: return 0xA;
                case 3: return 0xA;
                case 4: return 0xA;
            }
            break;
        
        case 2:
            switch (to) {
                case 1: return 0xE;
                case 3: return 0x8;
                case 4: return 0x8;
            }
            break;

        case 3:
            switch (to) {
                case 1: return 0xA;
                case 2: return 0xC;
                case 4: return 0xC;
            }
            break;

        case 4:
            switch (to) {
                case 1: return 0xC;
                case 2: return 0xE;
                case 3: return 0xE;
            }
            break;
    }

    abort();
}

static uint8_t switch_io_selector(IOGroup from, int to) {
    switch (from) {
        case IOGroup::LowIO:
            switch (to) {
                case 1:
                case 2: return 0x8;
                /*case 3:
                case 4: return 0x6;*/
            }
            break;

        case IOGroup::HighIO:
            switch (to) {
                /*case 1: 
                case 2: return 0x6;*/
                case 3: return 0xC;
                case 4: return 0xE;
            }
            break;
    }

    abort();
}

uint8_t Channel::switch_connection_selector() const {
    uint8_t select = 0x0;

    switch (type) {
        case Channel::Type::None:
        case Channel::Type::GlobalOutputDirect:
        case Channel::Type::GlobalBiIndirect:
        case Channel::Type::LocalOutput:
            break;

        case Channel::Type::GlobalInputDirect:
            select = switch_io_selector(data.global_input_direct.from,
                                        data.global_input_direct.cab_to);
            break;

        case Channel::Type::IntraCab:
            select = 0x2;
            break;

        case Channel::Type::InterCab: {
            select = switch_intercab_selector(data.inter_cab.cab_from_id, 
                                              data.inter_cab.cab_to_id);
            break;
        }

        case Channel::Type::LocalInput:
            select = 0x6;
            break;
    }

    assert(select != 0x0);

    if (side == Channel::Primary) {
        return select | 0x1;
    } else {
        return select;
    }
}

static uint8_t comparator_intercab_selector(int from, int to) {
    assert(from > 0 && from <= NBlocksPerChip);
    assert(to > 0 && to <= NBlocksPerChip);
    assert(from != to);

    switch (from) {
        case 1:
            switch (to) {
                case 2: return 0x10;
                case 3: return 0x10;
                case 4: return 0x10;
            }
            break;

        case 2:
            switch (to) {
                case 1: return 0x01;
                case 3: return 0x40;
                case 4: return 0x40;
            }
            break;

        case 3:
            switch (to) {
                case 1: return 0x10;
                case 2: return 0x04;
                case 4: return 0x04;
            }
            break;

        case 4:
            switch (to) {
                case 1: return 0x04;
                case 2: return 0x01;
                case 3: return 0x01;
            }
    }

    abort();
}

static uint8_t comparator_io_selector(IOGroup from, int to) {
    switch (from) {
        case IOGroup::LowIO:
            switch (to) {
                case 1:
                case 2: return 0x40;
                /*case 3: 
                case 4:*/
            }
            break;

        case IOGroup::HighIO:
            switch (to) {
                /*case 1:
                case 2:*/
                case 3: return 0x04;
                case 4: return 0x01;
            }
            break;
    }

    abort();
}

uint8_t Channel::comparator_connection_selector() const {
    uint8_t select = 0xFF;

    switch (type) {
        case Channel::Type::None:
        case Channel::Type::GlobalBiIndirect:
        case Channel::Type::GlobalOutputDirect:
        case Channel::Type::LocalOutput:
            break;

        case Channel::Type::GlobalInputDirect:
            select = comparator_io_selector(data.global_input_direct.from,
                                            data.global_input_direct.cab_to);
            break;

        case Channel::Type::IntraCab:
            break;

        case Channel::Type::InterCab:
            select = comparator_intercab_selector(data.inter_cab.cab_from_id,
                                                  data.inter_cab.cab_to_id);
            break;

        case Channel::Type::LocalInput:
            select = 0x00;
            break;
    }

    assert(select != 0xFF);

    if (side == Channel::Primary) {
        return select;
    } else {
        return select << 1;
    }
}

uint8_t Channel::local_input_source_selector() const {
    assert(type == Channel::Type::LocalInput);

    Channel *source = data.local_input.source;
    if (source == nullptr) {
        return 0x0;
    }

    switch (source->side) {
        case Channel::Primary:      return 0x1;
        case Channel::Secondary:    return 0x2;
    }

    return 0x0;
}

uint8_t Channel::local_output_dest_selector() const {
    assert(type == Channel::Type::LocalOutput);

    Channel *dest = data.local_output.dest;
    if (dest == nullptr) {
        return 0x0;
    }

    if (side == Channel::Primary) {
        switch (dest->side) {
            case Channel::Primary:      return 0x01;
            case Channel::Secondary:    return 0x02;
        }
    } else {
        switch (dest->side) {
            case Channel::Primary:      return 0x02;
            case Channel::Secondary:    return 0x01;
        }
    }

    return 0x0;
}

std::ostream &operator <<(std::ostream &os, Channel const &channel) {    
    os << "Channel [";

    switch (channel.type) {
        case Channel::Type::None:
            os << "none";
            break;

        case Channel::Type::GlobalInputDirect:  
            os << "global-input"; 
            break;

        case Channel::Type::GlobalOutputDirect: 
            os << "global-output"; 
            break;

        case Channel::Type::GlobalBiIndirect:   
            os << "global-bi"; 
            break;
            
        case Channel::Type::InterCab:           
            os << "inter-cab"; 
            break;

        case Channel::Type::IntraCab:           
            os << "intra-cab"; 
            break;

        case Channel::Type::LocalInput:         
            os << "local-input"; 
            break;

        case Channel::Type::LocalOutput:
            os << "local-output"; 
            break;
    }

    if (channel.side == Channel::Primary) {
        os << " primary";
    } else {
        os << " secondary";
    }
    os << "]";

    if (channel.driver) {
        os << " (>>> " << *channel.driver << ")";
    }
    return os;
}
