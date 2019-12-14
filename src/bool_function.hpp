#ifndef _MIX_DD_BOOL_FUNCTION_
#define _MIX_DD_BOOL_FUNCTION_


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