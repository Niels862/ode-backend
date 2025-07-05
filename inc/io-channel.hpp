#ifndef OBC_IO_CHANNEL_HPP
#define OBC_IO_CHANNEL_HPP

#include <cstdint>
#include <variant>
#include <array>
#include <iostream>

class AnalogBlock;
class IOCell;
class OutputPort;
class PortLink;

enum class CabRow {
    LowCabs,
    HighCabs,
};

enum class CabColumn {
    OddCabs,
    EvenCabs,
};

enum class IOGroup {
    LowIO,
    HighIO,
};

struct Channel {
    enum Side {
        Primary, 
        Secondary,
    };

    enum class Type {
        None,
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

    static Channel &None();
    static Channel GlobalInputDirect(Side side, IOGroup from, AnalogBlock &to);
    static Channel GlobalOutputDirect(Side side, IOGroup to, AnalogBlock &from);
    static Channel GlobalBiIndirect(Side side, CabColumn group);
    static Channel InterCab(Side side, AnalogBlock &from, AnalogBlock &to);
    static Channel IntraCab(Side side);
    static Channel LocalInput(Side side);
    static Channel LocalOutput(Side side);

    static CabRow to_cab_row(AnalogBlock &cab);
    static CabColumn to_cab_column(AnalogBlock &cab);
    static IOGroup to_io_group(IOCell &io_cell);

    bool available(PortLink &link);
    Channel &allocate(PortLink &link);

    void set_local_input_source(Channel &source);
    void set_local_output_dest(Channel &dest);

    uint8_t io_routing_selector() const;
    uint8_t switch_connection_selector() const;
    uint8_t local_input_source_selector() const;
    uint8_t local_output_dest_selector() const;

    friend std::ostream &operator <<(std::ostream &os, Channel const &channel);

    Type type;
    Side side;

    OutputPort *driver;

    union {
        struct {
            IOGroup from;
            int cab_to;
        } global_input_direct;

        struct {
            IOGroup to;
            int cab_from;
        } global_output_direct;

        struct {
            int cab_from_id;
            int cab_to_id;
        } intra_cab;

        struct {
            CabColumn group;
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
