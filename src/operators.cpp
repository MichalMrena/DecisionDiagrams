#include "operators.hpp"

namespace mix::dd
{
    auto AND::operator() 
        (const log_val_t lhs, const log_val_t rhs) const -> log_val_t
    {
        if (0 == lhs || 0 == rhs) return 0;
        if (X == lhs || X == rhs) return X;
        
        return lhs && rhs;
    }

    auto OR::operator() 
        (const log_val_t lhs, const log_val_t rhs) const -> log_val_t
    {
        if (1 == lhs || 1 == rhs) return 1;
        if (X == lhs || X == rhs) return X;
        
        return lhs || rhs;
    }

    auto XOR::operator() 
        (const log_val_t lhs, const log_val_t rhs) const -> log_val_t
    {
        if (X == lhs || X == rhs) return X;

        return lhs ^ rhs;
    }

    auto NAND::operator() 
        (const log_val_t lhs, const log_val_t rhs) const -> log_val_t
    {
        if (0 == lhs || 0 == rhs) return 1;
        if (X == lhs || X == rhs) return X;
        
        return !(lhs && rhs);
    }

    auto NOR::operator() 
        (const log_val_t lhs, const log_val_t rhs) const -> log_val_t
    {
        if (1 == lhs || 1 == rhs) return 0;
        if (X == lhs || X == rhs) return X;
        
        return !(lhs || rhs);
    }
}