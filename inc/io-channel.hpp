#ifndef OBC_IO_CHANNEL_HPP
#define OBC_IO_CHANNEL_HPP

#include <cstdint>
#include <variant>
#include <array>
#include <iostream>

class AnalogBlock;
class OutputPort;
class PortLink;

struct Channel {
    enum Side {
        Primary, 
        Secondary
    };

    enum class Type {
        GlobalInputDirect,
        GlobalOutputDirect,
        GlobalBiIndirect,
        InterCab,
        IntraCab,
        LocalInput,
        LocalOutput,
    };
    
    Channel();
    Channel(Channel::Type type, Channel::Side side);

    static Channel InterCab(Side side, AnalogBlock &from, AnalogBlock &to);
    static Channel IntraCab(Side side);
    static Channel LocalInput(Side side);
    static Channel LocalOutput(Side side);

    bool available(PortLink &link);
    Channel &allocate(PortLink &link);

    uint8_t switch_connection_selector() const;

    friend std::ostream &operator <<(std::ostream &os, Channel const &channel);

    Type type;
    Side side;

    OutputPort *driver;

    union {
        struct {
            int cab_from_id;
            int cab_to_id;
        } intracab;
    } data;
};

#endif
