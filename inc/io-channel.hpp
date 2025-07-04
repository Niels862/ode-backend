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
        GlobalDirect,
        GlobalIndirect,
        InterCab,
        IntraCab,
        Local,
    };
    
    Channel();

    static Channel InterCab(Side side, AnalogBlock &from, AnalogBlock &to);
    static Channel IntraCab(Side side);

    void allocate(PortLink &link);

    uint8_t switch_connection_selector() const;

    friend std::ostream &operator <<(std::ostream &os, Channel const &channel);

    OutputPort *driver;
    Side side;
    Type type;

    union {
        struct {
            int cab_from_id;
            int cab_to_id;
        } intracab;
    } data;
};

#endif
