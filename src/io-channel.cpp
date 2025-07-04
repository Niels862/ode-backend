#include "io-channel.hpp"
#include "io-port.hpp"
#include <sstream>
#include <stdexcept>

Channel::Channel()
        : driver{nullptr}, side{} {}

Channel::Channel(Channel::Side side)
        : driver{nullptr}, side{side} {}

void Channel::allocate(PortLink &link) {
    if (driver == link.out) {
        link.channel = this;
        return;
    }
    if (driver == nullptr) {
        link.channel = this;
        driver = link.out;
        return;
    }
    std::stringstream ss;
    ss << "Cannot allocate Channel " << *this << " for Link " << link;
    throw std::runtime_error(ss.str());
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
