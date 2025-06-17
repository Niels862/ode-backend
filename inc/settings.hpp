#ifndef OBC_SETTINGS_HPP
#define OBC_SETTINGS_HPP

#include <string>

struct Args {
    bool verbose;
    bool raw;
    std::string infile;
    std::string outfile;
};

extern Args args; 

#endif
