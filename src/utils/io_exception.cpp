#include "io_exception.hpp"

namespace mix::utils
{
    io_exception::io_exception(const std::string& pErrorMessage) :
        errorMessage {pErrorMessage}
    {
    }

    auto io_exception::what () const throw () -> const char *
    {
        return this->errorMessage.c_str();
    }
}
