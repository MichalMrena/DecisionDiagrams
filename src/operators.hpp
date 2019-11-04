#ifndef DD_OPERATORS
#define DD_OPERATORS

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
        auto operator() (const log_val_t lhs
                       , const log_val_t rhs) const -> log_val_t;
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

    /**
        If - then - else ternary function.
     */
    struct ITE
    {

    };
}

#endif