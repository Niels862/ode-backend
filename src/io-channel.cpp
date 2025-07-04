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

Channel Channel::LocalInput(Side side) {
    return Channel(Channel::Type::LocalInput, side);
}

Channel Channel::LocalOutput(Side side) {
    return Channel(Channel::Type::LocalOutput, side);
}

Channel Channel::InterCab(Side side, AnalogBlock &from, AnalogBlock &to) {
    Channel channel(Channel::Type::InterCab, side);

    auto &data = channel.data.intracab;
    data.cab_from_id = from.id();
    data.cab_to_id = to.id();

    return channel;
}

void Channel::allocate(PortLink &link) {
    if (driver == link.out) {
        link.channels.push_back(this);
        return;
    }
    if (driver == nullptr) {
        link.channels.push_back(this);
        driver = link.out;
        return;
    }
    std::stringstream ss;
    ss << "Cannot allocate Channel " << *this << " for Link " << link;
    throw std::runtime_error(ss.str());
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
            select = intercab_selector(data.intracab.cab_from_id, 
                                       data.intracab.cab_to_id);
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
    if (channel.side == Channel::Primary) {
        os << "primary";
    } else {
        os << "secondary";
    }
    os << "]";

    if (channel.driver) {
        os << " (>>> " << *channel.driver << ")";
    }
    return os;
}
