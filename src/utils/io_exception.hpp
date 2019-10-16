#ifndef MIX_DD_IO_EXCEPTION
#define MIX_DD_IO_EXCEPTION

#include <exception>
#include <string>

namespace mix::utils
{
    class io_exception : public std::exception
    {
    private:
        std::string errorMessage {"io_exception"};
        
    public:
        io_exception() = default;
        io_exception(const std::string& pErrorMessage);

        auto what () const throw () -> const char *;
    };
}


#endif