#ifndef MIX_UTILS_LESS_FUNCTIONAL_HPP
#define MIX_UTILS_LESS_FUNCTIONAL_HPP

namespace teddy::utils
{
    /**
     *  @brief Creates a function that negates an output of binary function @p f .
     */
    auto constexpr fnot = [](auto const f)
    {
        // using F = decltype(f);
        return [f](auto const l, auto const r)
        {
            return ! f (l, r);
        };
    };

    /**
     *  @brief Modular addition.
     */
    template<auto M>
    auto constexpr plus_mod = [](auto const l, auto const r) { return (l + r) % M; };
    template<auto M>
    using plus_mod_t = decltype(plus_mod<M>);

    /**
     *  @brief Modular multiplication.
     */
    template<auto M>
    auto constexpr multiplies_mod = [](auto const l, auto const r) { return (l * r) % M; };
    template<auto M>
    using multiplies_mod_t = decltype(multiplies_mod<M>);

    /**
     *  @brief Logical and.
     */
    auto constexpr logical_and = [](auto const l, auto const r) { return l && r; };
    using logical_and_t = decltype(logical_and);

    /**
     *  @brief Logical or.
     */
    auto constexpr logical_or = [](auto const l, auto const r) { return l || r; };
    using logical_or_t = decltype(logical_or);

    /**
     *  @brief Logical nand.
     */
    auto constexpr logical_nand = fnot(logical_and);
    using logical_nand_t = decltype(logical_nand);

    /**
     *  @brief Logical nor.
     */
    auto constexpr logical_nor = fnot(logical_or);
    using logical_nor_t = decltype(logical_nor);

    /**
     *  @brief Logical xor.
     */
    auto constexpr logical_xor = [](auto const l, auto const r) { return l != r; };
    using logical_xor_t = decltype(logical_xor);

    /**
     *  @brief Equal to.
     */
    auto constexpr equal_to = [](auto const l, auto const r) { return l == r; };
    using equal_to_t = decltype(equal_to);

    /**
     *  @brief Not equal to.
     */
    auto constexpr not_equal_to = fnot(equal_to);
    using not_equal_to_t = decltype(not_equal_to);

    /**
     *  @brief Less.
     */
    auto constexpr less = [](auto const l, auto const r) { return l < r; };
    using less_t = decltype(less);

    /**
     *  @brief Less equal.
     */
    auto constexpr less_equal = [](auto const l, auto const r) { return l <= r; };
    using less_equal_t = decltype(less_equal);

    /**
     *  @brief Greater.
     */
    auto constexpr greater = [](auto const l, auto const r){ return l > r; };
    using greater_t = decltype(greater);

    /**
     *  @brief Greater equal.
     */
    auto constexpr greater_equal = [](auto const l, auto const r){ return l >= r; };
    using greater_equal_t = decltype(greater_equal);

    /**
     *  @brief Min.
     */
    auto constexpr min = [](auto const l, auto const r) { return l < r ? l : r; };
    using min_t = decltype(min);

    /**
     *  @brief Max.
     */
    auto constexpr max = [](auto const l, auto const r) { return l > r ? l : r; };
    using max_t = decltype(max);

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