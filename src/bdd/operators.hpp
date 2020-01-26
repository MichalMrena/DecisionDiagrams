#ifndef _MIX_DD_OPERATORS_
#define _MIX_DD_OPERATORS_

#include "../dd/typedefs.hpp"

namespace mix::dd
{
    struct AND
    {
        auto operator() ( const bool_t lhs
                        , const bool_t rhs ) const -> bool_t;
    };

    struct OR
    {
        auto operator() ( const bool_t lhs
                        , const bool_t rhs ) const -> bool_t;
    };

    struct XOR
    {
        auto operator() ( const bool_t lhs
                        , const bool_t rhs ) const -> bool_t;
    };

    struct NAND
    {
        auto operator() ( const bool_t lhs
                        , const bool_t rhs ) const -> bool_t;
    };

    struct NOR
    {
        auto operator() ( const bool_t lhs
                        , const bool_t rhs ) const -> bool_t;
    };
}

#endif