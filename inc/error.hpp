#ifndef OBC_ERROR_HPP
#define OBC_ERROR_HPP

#include <string>
#include <stdexcept>

class DesignError : public std::exception {
public:
    DesignError(std::string const &reason)
            : m_reason{reason} {}

    const char *what() const noexcept {
        return m_reason.c_str();
    }

private:
    std::string m_reason;
};

#endif
