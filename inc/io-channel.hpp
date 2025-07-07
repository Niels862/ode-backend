#ifndef OBC_IO_CHANNEL_HPP
#define OBC_IO_CHANNEL_HPP

#include <cstdint>
#include <variant>
#include <array>
#include <iostream>
#include <stdexcept>

class AnalogBlock;
class IOCell;

enum class OutPortSource;
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
    static bool uses_direct_channel(IOCell &cell, AnalogBlock &cab);
    static Channel::Side source_to_side(OutPortSource source);

    template<typename F>
    static Channel &find_available(F select, PortLink *link) {
        for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
            Channel &channel = select(side);
            if (channel.allocated_for(*link)) {
                return channel;
            }
        }

        for (Channel::Side side : { Channel::Primary, Channel::Secondary }) {
            Channel& channel = select(side);
            if (channel.available(*link)) {
                return channel;
            }
        }

        throw std::runtime_error("Could not route design");
    }

    bool allocated_for(PortLink &link);
    bool available(PortLink &link);
    Channel &allocate(PortLink &link);

    void set_local_input_source(Channel &source);
    void set_local_output_dest(Channel &dest);
    Channel *local_input_source();
    Channel *local_output_dest();

    uint8_t io_routing_selector() const;
    uint8_t switch_connection_selector() const;
    uint8_t comparator_connection_selector() const;
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
        } inter_cab;

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
