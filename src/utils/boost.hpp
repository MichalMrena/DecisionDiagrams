#ifndef MIX_UTILS_BOOST
#define MIX_UTILS_BOOST

#include <functional>

namespace mix::utils::boost
{
    template <class T>
    auto hash_combine(std::size_t& seed, T const& v) -> void
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }
}

#endif