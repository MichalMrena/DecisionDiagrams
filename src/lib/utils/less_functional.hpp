#ifndef MIX_UTILS_LESS_FUNCTIONAL_HPP
#define MIX_UTILS_LESS_FUNCTIONAL_HPP

namespace teddy::utils
{
    /**
     *  @brief Modular addition.
     */
    template<auto M>
    struct plus_mod_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return (l + r) % M; } };

    /**
     *  @brief Modular multiplication.
     */
    template<auto M>
    struct multiplies_mod_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return (l * r) % M; } };

    /**
     *  @brief Logical and.
     */
    struct logical_and_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l && r; } };

    /**
     *  @brief Logical or.
     */
    struct logical_or_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l || r; } };

    /**
     *  @brief Logical nand.
     */
    struct logical_nand_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return !(l && r); } };

    /**
     *  @brief Logical nor.
     */
    struct logical_nor_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return !(l || r); } };

    /**
     *  @brief Logical xor.
     */
    struct logical_xor_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l != r; } };

    /**
     *  @brief Equal to.
     */
    struct equal_to_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l == r; } };

    /**
     *  @brief Not equal to.
     */
    struct not_equal_to_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l != r; } };

    /**
     *  @brief Less.
     */
    struct less_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l < r; } };

    /**
     *  @brief Less equal.
     */
    struct less_equal_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l <= r; } };

    /**
     *  @brief Greater.
     */
    struct greater_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l > r; } };

    /**
     *  @brief Greater equal.
     */
    struct greater_equal_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l >= r; } };

    /**
     *  @brief Min.
     */
    struct min_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l < r ? l : r; } };

    /**
     *  @brief Max.
     */
    struct max_t { template<class T> auto constexpr operator() (T const l, T const r) const noexcept { return l > r ? l : r; } };

    /**
     *  @brief Function that does nothing.
     */
    auto constexpr no_op = [](auto&&...) noexcept {};
    using no_op_t = decltype(no_op);

    /**
     *  @brief Returns true if given container is not empty.
     */
    auto constexpr not_empty = [](auto const& c) { return !c.empty(); };
    using not_empty_t = decltype(not_empty);

    /**
     *  @brief Identity function for small value types.
     */
    auto constexpr identity = [](auto const a) { return a; };
    using identity_t = decltype(identity);

    /**
     *  @brief Creates a constant function that returns @p x by value.
     */
    auto constexpr constant = [](auto const x){ return [x](auto&&...){ return x; }; };
    using constant_t = decltype(constant);
}

#endif