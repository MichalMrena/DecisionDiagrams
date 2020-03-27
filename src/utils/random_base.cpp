#include "random_base.hpp"

namespace mix::utils
{
    random_base::random_base(unsigned long seed) :
        generator_ {seed}
    {
    }
}