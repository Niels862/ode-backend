#ifndef OBC_SETTINGS_HPP
#define OBC_SETTINGS_HPP

#include <string>

struct Args {
    bool verbose;
    bool raw;
    bool add_size;
    bool add_check;
    std::string infile;
    std::string outfile;
};

extern Args args; 

#endif
