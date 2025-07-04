#ifndef OBC_IO_CHANNEL_HPP
#define OBC_IO_CHANNEL_HPP

#include <array>
#include <iostream>

class OutputPort;

struct Channel {
    enum Side {
        Primary, 
        Secondary
    };
    
    Channel();

    friend std::ostream &operator <<(std::ostream &os, Channel const &channel);

    OutputPort *driver;
};

#endif
