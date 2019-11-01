#include "operators.hpp"

namespace mix::dd
{
    auto AND::operator() (const log_val_t lhs
                        , const log_val_t rhs) const -> log_val_t
    {
        if (0 == lhs || 0 == rhs) return 0;
        if (X == lhs || X == rhs) return X;
        else return lhs && rhs;
    }
}