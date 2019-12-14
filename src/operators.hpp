#ifndef _DD_OPERATORS_
#define _DD_OPERATORS_

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
        auto operator() (const log_val_t lhs
                       , const log_val_t rhs) const -> log_val_t;
    };

    struct NAND
    {
        auto operator() (const log_val_t lhs
                       , const log_val_t rhs) const -> log_val_t;
    };

    struct NOR
    {
        auto operator() (const log_val_t lhs
                       , const log_val_t rhs) const -> log_val_t;
    };

    /**
        If - then - else ternary function.
     */
    struct ITE
    {

    };
}

#endif