#include "counter.hpp"

namespace mix::utils
{
    auto counter::inc 
        (const size_t amount) -> void
    {
        this->count += amount;
    }

    auto counter::get
        () const -> size_t
    {
        return this->count;
    }

    auto counter::reset
        () -> void
    {
        this->count = 0;
    }
}