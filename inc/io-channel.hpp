#ifndef OBC_IO_CHANNEL_HPP
#define OBC_IO_CHANNEL_HPP

#include <array>
#include <iostream>

class OutputPort;
class PortLink;

struct Channel {
    enum Side {
        Primary, 
        Secondary
    };
    
    Channel();
    Channel(Side side);

    void allocate(PortLink &link);

    friend std::ostream &operator <<(std::ostream &os, Channel const &channel);

    OutputPort *driver;
    Side side;
};

#endif
