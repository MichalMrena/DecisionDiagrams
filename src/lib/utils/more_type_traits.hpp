#ifndef MIX_UTILS_MORE_TYPE_TRAITS_
#define MIX_UTILS_MORE_TYPE_TRAITS_

#include <array>
#include <bitset>
#include <vector>

namespace mix::utils
{
    /**
        Provides member type definition that is equal to either
        the first or the second type based on bool value.
     */
    template<bool, class IfTrue, class IfFalse>
    struct either_or;

    template<class IfTrue, class IfFalse>
    struct either_or<true, IfTrue, IfFalse>
    {
        using type = IfTrue;
    };

    template<class IfTrue, class IfFalse>
    struct either_or<false, IfTrue, IfFalse>
    {
        using type = IfFalse;
    };

    template<bool Cond, class IfTrue, class IfFalse>
    using either_or_t = typename either_or<Cond, IfTrue, IfFalse>::type;

    /**
        Provides member constant that is equal to true if the
        type of T is std::array and is false otherwise.
     */
    template<class T>
    struct is_std_array : public std::false_type {};

    template<class T, std::size_t N>
    struct is_std_array<std::array<T, N>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_array_v = is_std_array<T>::value;

    /**
        Provides member constant that is equal to true if the
        type of T is std::vector and is false otherwise.
     */
    template<class T>
    struct is_std_vector : public std::false_type {};

    template<class T, class Allocator>
    struct is_std_vector<std::vector<T, Allocator>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_vector_v = is_std_vector<T>::value;

    /**
        Provides member constant that is equal to true if the
        type of T is std::bitset and is false otherwise.
     */
    template<class T>
    struct is_std_bitset : public std::false_type {};

    template<std::size_t N>
    struct is_std_bitset<std::bitset<N>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_bitset_v = is_std_bitset<T>::value;

    namespace aux_impl
    {
        // https://stackoverflow.com/questions/81870/is-it-possible-to-print-a-variables-type-in-standard-c

        template <class T>
        inline constexpr auto type_name() -> std::string_view
        {
        using namespace std;
        #ifdef __clang__
            string_view p = __PRETTY_FUNCTION__;
            return string_view(p.data() + 34, p.size() - 34 - 1);
        #elif defined(__GNUC__)
            string_view p = __PRETTY_FUNCTION__;
        #  if __cplusplus < 201402
            return string_view(p.data() + 36, p.size() - 36 - 1);
        #  else
            return string_view(p.data() + 49, p.find(';', 49) - 49);
        #  endif
        #elif defined(_MSC_VER)
            string_view p = __FUNCSIG__;
            return string_view(p.data() + 84, p.size() - 84 - 7);
        #endif
        }
    }
}

#endif