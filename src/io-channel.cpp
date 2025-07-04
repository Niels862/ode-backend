#include "io-channel.hpp"
#include "io-port.hpp"
#include "analog-block.hpp"
#include <sstream>
#include <stdexcept>
#include <cassert>

Channel::Channel()
        : driver{nullptr} {}

Channel Channel::IntraCab(Side side) {
    Channel channel;

    channel.type = Channel::Type::IntraCab;
    channel.side = side;

    return channel;
}

Channel Channel::InterCab(Side side, AnalogBlock &from, AnalogBlock &to) {
    Channel channel;

    channel.type = Channel::Type::InterCab;
    channel.side = side;
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
        case Channel::Type::GlobalDirect:
            break;
        
        case Channel::Type::GlobalIndirect:
            break;

        case Channel::Type::IntraCab:
            select = 0x2;
            break;

        case Channel::Type::InterCab: {
            select = intercab_selector(data.intracab.cab_from_id, 
                                       data.intracab.cab_to_id);
            break;
        }

        case Channel::Type::Local:
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
