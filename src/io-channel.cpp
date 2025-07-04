#include "io-channel.hpp"
#include "io-port.hpp"

Channel::Channel()
        : driver{nullptr} {}

std::ostream &operator <<(std::ostream &os, Channel const &channel) {
    if (channel.driver) {
        os << "(>>" << *channel.driver << ")";
    }
    return os;
}
