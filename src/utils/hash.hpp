#ifndef MIX_UTILS_HASH
#define MIX_UTILS_HASH

#include <functional>

namespace mix::utils
{
    template<class T>
    struct pointer_hash
    {
        auto operator() (const T* vptr) const -> size_t;
    };

    template<class T>
    auto pointer_hash<T>::operator() (const T* vptr) const -> size_t
    {
        // TODO na toto sa treba ešte pozrieť
        static const size_t shift {static_cast<size_t>(std::log2(1 + sizeof(T)))};
        return static_cast<size_t>(vptr) >> shift;
    }

    namespace boost
    {
        template <class T, class Hasher>
        auto hash_combine(std::size_t& seed, T const& v) -> void
        {
            Hasher hasher;
            seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        }
    }
}

#endif