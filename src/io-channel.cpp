#include "io-channel.hpp"
#include "io-port.hpp"
#include "analog-block.hpp"
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

Channel Channel::GlobalBiIndirect(Side side, CabGroup group) {
    Channel channel(Channel::Type::GlobalBiIndirect, side);

    auto &data = channel.data.global_bi_indirect;
    data.group = group;

    return channel;
}

Channel Channel::InterCab(Side side, AnalogBlock &from, AnalogBlock &to) {
    Channel channel(Channel::Type::InterCab, side);

    auto &data = channel.data.intra_cab;
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

CabGroup Channel::to_cab_group(AnalogBlock &cab) {
    switch (cab.id()) {
        case 1: 
        case 3:
            return CabGroup::OddCabs;

        case 2:
        case 4:
            return CabGroup::EvenCabs;
    }

    abort();
}

bool Channel::available(PortLink &link) {
    if (driver == link.out) {
        return true;
    }

    if (driver == nullptr) {
        return true;
    }

    return false;
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

static uint8_t intercab_selector(int from, int to) {
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

    assert(1);
    return 0x0;
}

uint8_t Channel::switch_connection_selector() const {
    uint8_t select = 0x0;

    switch (type) {
        case Channel::Type::GlobalInputDirect:
        case Channel::Type::GlobalOutputDirect:
        case Channel::Type::GlobalBiIndirect:
        case Channel::Type::LocalOutput:
            break;

        case Channel::Type::IntraCab:
            select = 0x2;
            break;

        case Channel::Type::InterCab: {
            select = intercab_selector(data.intra_cab.cab_from_id, 
                                       data.intra_cab.cab_to_id);
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

std::ostream &operator <<(std::ostream &os, Channel const &channel) {    
    os << "Channel [";

    switch (channel.type) {
        case Channel::Type::GlobalInputDirect:  os << "global-input"; break;
        case Channel::Type::GlobalOutputDirect: os << "global-output"; break;
        case Channel::Type::GlobalBiIndirect:   os << "global-bi"; break;
        case Channel::Type::InterCab:           os << "inter-cab"; break;
        case Channel::Type::IntraCab:           os << "intra-cab"; break;
        case Channel::Type::LocalInput:         os << "local-input"; break;
        case Channel::Type::LocalOutput:        os << "local-output"; break;
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
