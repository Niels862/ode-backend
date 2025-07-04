#ifndef OBC_IO_CHANNEL_HPP
#define OBC_IO_CHANNEL_HPP

#include <cstdint>
#include <variant>
#include <array>
#include <iostream>

class AnalogBlock;
class OutputPort;
class PortLink;

enum class CabGroup {
    OddCabs,
    EvenCabs,
};

struct Channel {
    enum Side {
        Primary, 
        Secondary,
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

    static Channel GlobalBiIndirect(Side side, CabGroup group);
    static Channel InterCab(Side side, AnalogBlock &from, AnalogBlock &to);
    static Channel IntraCab(Side side);
    static Channel LocalInput(Side side);
    static Channel LocalOutput(Side side);

    static CabGroup to_cab_group(AnalogBlock &cab);

    bool available(PortLink &link);
    Channel &allocate(PortLink &link);

    void set_local_input_source(Channel &source);
    void set_local_output_dest(Channel &dest);

    uint8_t switch_connection_selector() const;
    uint8_t local_input_source_selector() const;
    uint8_t local_output_dest_selector() const;

    friend std::ostream &operator <<(std::ostream &os, Channel const &channel);

    Type type;
    Side side;

    OutputPort *driver;

    union {
        struct {
            int cab_from_id;
            int cab_to_id;
        } intra_cab;

        struct {
            CabGroup group;
        } global_bi_indirect;

        struct {
            Channel *source;
        } local_input;

        struct {
            Channel *dest;
        } local_output;
    } data;
};

#endif
