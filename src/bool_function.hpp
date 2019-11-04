#ifndef MIX_DD_BOOL_FUNCTION
#define MIX_DD_BOOL_FUNCTION


#include "typedefs.hpp"

namespace mix::dd
{
    class bool_function
    {
    public:
        virtual ~bool_function() = default;

        virtual auto operator[] (const input_t input) const -> log_val_t = 0;
        
        virtual auto variable_count () const -> size_t = 0;
    };
    
}

#endif