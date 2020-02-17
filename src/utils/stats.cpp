#include "stats.hpp"

namespace mix::utils
{
    auto averager::add_value 
        (const double val) -> void
    {
        this->accSUm += val;
        ++this->valCount;
    }

    auto averager::average
        () const -> double
    {
        return this->accSUm / this->valCount;
    }
}