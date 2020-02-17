#include "operators.hpp"

namespace mix::dd
{
    auto AND::operator() 
        (const bool_t lhs, const bool_t rhs) const -> bool_t
    {
        if (0 == lhs || 0 == rhs) return 0;
        if (X == lhs || X == rhs) return X;
        
        return lhs && rhs;
    }

    auto OR::operator() 
        (const bool_t lhs, const bool_t rhs) const -> bool_t
    {
        if (1 == lhs || 1 == rhs) return 1;
        if (X == lhs || X == rhs) return X;
        
        return lhs || rhs;
    }

    auto XOR::operator() 
        (const bool_t lhs, const bool_t rhs) const -> bool_t
    {
        if (X == lhs || X == rhs) return X;

        return lhs ^ rhs;
    }

    auto NAND::operator() 
        (const bool_t lhs, const bool_t rhs) const -> bool_t
    {
        if (0 == lhs || 0 == rhs) return 1;
        if (X == lhs || X == rhs) return X;
        
        return ! (lhs && rhs);
    }

    auto NOR::operator() 
        (const bool_t lhs, const bool_t rhs) const -> bool_t
    {
        if (1 == lhs || 1 == rhs) return 0;
        if (X == lhs || X == rhs) return X;
        
        return ! (lhs || rhs);
    }
}