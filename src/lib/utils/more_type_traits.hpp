#ifndef MIX_UTILS_MORE_TYPE_TRAITS_HPP
#define MIX_UTILS_MORE_TYPE_TRAITS_HPP

#include <array>
#include <bitset>
#include <vector>
#include <iterator>

namespace teddy::utils
{
    /**
     *  @brief Checks if @p T is @c std::array .
     */
    template<class T>
    struct is_std_array : public std::false_type {};

    template<class T, std::size_t N>
    struct is_std_array<std::array<T, N>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_array_v = is_std_array<T>::value;

    /**
     *  @brief Checks if @p T is @c std::vector .
     */
    template<class T>
    struct is_std_vector : public std::false_type {};

    template<class T, class Allocator>
    struct is_std_vector<std::vector<T, Allocator>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_vector_v = is_std_vector<T>::value;

    /**
     *  @brief Checks if @p T is @c std::bitset .
     */
    template<class T>
    struct is_std_bitset : public std::false_type {};

    template<std::size_t N>
    struct is_std_bitset<std::bitset<N>> : public std::true_type {};

    template<class T>
    inline constexpr auto is_std_bitset_v = is_std_bitset<T>::value;

    /**
     *  @brief Checks if @p Iterator is random access.
     */
    template<class Iterator>
    struct is_random_access : public std::conditional_t< std::is_same_v< std::random_access_iterator_tag
                                                                       , typename std::iterator_traits<Iterator>::iterator_category >
                                                       , std::true_type
                                                       , std::false_type > {};

    template<class Iterator>
    inline constexpr auto is_random_access_v = is_random_access<Iterator>::value;
}

#endif