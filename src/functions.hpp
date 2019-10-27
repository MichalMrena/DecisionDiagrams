#ifndef DD_FUNCTIONS
#define DD_FUNCTIONS

#include "typedefs.hpp"

namespace mix::dd
{
    struct AND
    {
        auto operator() (const log_val_t lhs
                       , const log_val_t rhs) const -> log_val_t;
    };

    struct OR
    {

    };

    struct XOR
    {

    };

    struct NAND
    {

    };

    struct NOR
    {

    };
}

#endif