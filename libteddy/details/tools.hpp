#ifndef LIBTEDDY_DETAILS_UTILS_HPP
#define LIBTEDDY_DETAILS_UTILS_HPP

#include <libteddy/details/types.hpp>

    #include <charconv>
    #include <optional>
    #include <string_view>
#include <cstddef>
#include <vector>

namespace teddy::utils
{
/**
 *  \brief Exponentiation by squaring
 */
template<class Base>
auto constexpr int_pow(Base base, int32 exponent) -> Base
{
    Base result = 1;

    for (;;)
    {
        if (exponent & 1)
        {
            result *= base;
        }

        exponent >>= 1;

        if (0 == exponent)
        {
            break;
        }

        base *= base;
    }

    return result;
}

/**
 *  \brief Tries to parse \p input to \p Num
 *  \param input input string
 *  \return optinal result
 */
template<class Num>
auto parse (std::string_view const input) -> std::optional<Num>
{
    auto ret = Num {};
    auto result
        = std::from_chars(input.data(), input.data() + input.size(), ret);
    return std::errc {} == result.ec
                && result.ptr == input.data() + input.size()
             ? std::optional<Num>(ret)
             : std::nullopt;
}

/**
 *  \brief Hash for pointers
 */
inline auto do_hash (void* const p) -> std::size_t
{
    return reinterpret_cast<std::size_t>(p) >> 4;
}

/**
 *  \brief Hash for int
 */
inline auto do_hash (int32 const x) -> std::size_t
{
    return static_cast<std::size_t>(x);
}

/**
 *  \brief Hashes \p elem and combines the result with \p hash
 */
template<class T>
auto add_hash (std::size_t& hash, T const& elem) -> void
{
    // see boost::hash_combine
    hash ^= do_hash(elem) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
}

/**
 *  \brief Computes hash of the \p args
 */
template<class... Ts>
auto pack_hash (Ts const&... args) -> std::size_t
{
    std::size_t result = 0;
    (add_hash(result, args), ...);
    return result;
}

/**
 *  \brief The min function
 */
template<class T>
auto constexpr min (T lhs, T rhs) -> T
{
    return lhs < rhs ? lhs : rhs;
}

/**
 *  \brief The max function
 */
template<class T>
auto constexpr max (T lhs, T rhs) -> T
{
    return lhs > rhs ? lhs : rhs;
}

/**
 *  \brief The min function for parameter packs
 */
template<class X>
auto constexpr pack_min (X x) -> X
{
    return x;
}

/**
 *  \brief The min function for parameter packs
 */
template<class X, class... Xs>
auto constexpr pack_min (X x, Xs... xs) -> X
{
    return min(x, pack_min(xs...));
}

/**
 *  \brief Maximum of a range
 *  Implementation of std::max_element
 */
template<class It>
auto constexpr max_elem (It first, It const last) -> It
{
    It maxIt = first;
    while (first != last)
    {
        if (*first > *maxIt)
        {
            maxIt = first;
        }
        ++first;
    }
    return maxIt;
}

// TODO move to pla input
/**
 *  \brief Finds the first element satisfying \p test
 *  Implementation of std::find_if
 */
template<class It, class Predicate>
auto constexpr find_if (It first, It const last, Predicate test) -> It
{
    while (first != last)
    {
        if (test(*first))
        {
            return first;
        }
        ++first;
    }
    return last;
}

// TODO move to pla input
/**
 *  \brief Finds the first element not satisfying \p test
 *  Implementation of std::find_if_not
 */
template<class It, class Predicate>
auto constexpr find_if_not (It first, It const last, Predicate test) -> It
{
    while (first != last)
    {
        if (not test(*first))
        {
            return first;
        }
        ++first;
    }
    return last;
}

/**
 *  \brief Exchages value of \p var to \p newVal and returns the old value
 *  Simplified implementation of std::exchange
 */
template<class T, class U = T>
auto constexpr exchange (T& var, U newVal) noexcept -> T
{
    auto oldVal = var;
    var         = newVal;
    return oldVal;
}

/**
 *  \brief Swaps values in \p first and \p second
 *  Simplified implementation of std::swap
 */
template<typename T>
auto constexpr swap (T& first, T& second) noexcept -> void
{
    auto tmp = first;
    first    = second;
    second   = tmp;
}

/**
 *  \brief Simple heapsort for vectors
 */
template<class T, class Compare>
auto sort (std::vector<T>& xs, Compare cmp) -> void
{
    if (xs.empty())
    {
        return;
    }

    auto const sift_down = [&xs, cmp] (uint32 parent, uint32 const size)
    {
        uint32 left  = 2 * parent + 1;
        uint32 right = left + 1;
        while (left < size)
        {
            uint32 swap = parent;
            if (cmp(xs[swap], xs[left]))
            {
                swap = left;
            }

            if (right < size && cmp(xs[swap], xs[right]))
            {
                swap = right;
            }

            if (swap == parent)
            {
                break;
            }

            utils::swap(xs[parent], xs[swap]);
            parent = swap;
            left   = 2 * parent + 1;
            right  = left + 1;
        }
    };

    uint32 const size = static_cast<uint32>(xs.size());

    // make-heap
    for (uint32 i = size / 2 + 1; i > 0;)
    {
        --i;
        sift_down(i, size);
    }

    // pop-heap
    for (uint32 last = size - 1; last > 0; --last)
    {
        utils::swap(xs[last], xs[0]);
        sift_down(0, last);
    }
}

// TODO this wont be necessary when we sort out node data and caches...
template<class T>
struct is_void
{
    static constexpr bool value = false;
};

template<>
struct is_void<void>
{
    static constexpr bool value = true;
};

template<class T, class U>
struct is_same
{
    static constexpr bool value = false;
};

template<class T>
struct is_same<T, T>
{
    static constexpr bool value = true;
};

template<class T, class U>
concept same_as = is_same<T, U>::value;

template<class T>
concept is_std_vector = same_as<
    T,
    std::vector<typename T::value_type, typename T::allocator_type>
>;

/**
 *  \brief Provides member typedef based on the value of \p B
 *  Implementation of \c std::conditional
 */
template<bool B, class T, class F>
struct type_if;

/**
 *  \brief Specialization for B = true
 */
template<class T, class F>
struct type_if<true, T, F>
{
    using type = T;
};

/**
 *  \brief Specialization for B = false
 */
template<class T, class F>
struct type_if<false, T, F>
{
    using type = F;
};

/**
 *  \brief Helper for SFINE functions
 */
template<class X, class T>
using second_t = type_if<false, X, T>::type;

/**
 *  \brief Implementation of \c std::remove_reference
 */
template<class T>
struct remove_reference
{
    using type = T;
};

/**
 *  \brief Specialization for lvalue ref
 */
template<class T>
struct remove_reference<T&>
{
    using type = T;
};

/**
 *  \brief Specialization for rvalue ref
 */
template<class T>
struct remove_reference<T&&>
{
    using type = T;
};

    // TODO asi nebude treba
    template<class T>
    struct optional_member
    {
        T member_;
    };

    template<>
    struct optional_member<void>
    {
    };
} // namespace teddy::utils

#endif