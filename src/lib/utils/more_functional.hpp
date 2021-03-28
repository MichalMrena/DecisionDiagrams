#ifndef MIX_UTILS_MORE_FUNCTIONAL_HPP
#define MIX_UTILS_MORE_FUNCTIONAL_HPP

    // TODO replace with custom ones
    #include <functional>
    #include <utility>

namespace mix::utils
{
    /**
     *  @brief Creates a function that negates output of @p f .
     */
    auto constexpr fnot = [](auto const f)
    {
        using F = decltype(f);
        return [](auto&&... args)
        {
            return ! F () (std::forward<decltype(args)>(args)...);
        };
    };

    /**
     *  @brief Modular addition.
     */
    template<std::size_t M>
    auto constexpr plus_mod = [](auto const l, auto const r) { return (l + r) % M; };
    template<std::size_t M>
    using plus_mod_t = decltype(plus_mod<M>);

    /**
     *  @brief Modular multiplication.
     */
    template<std::size_t M>
    auto constexpr multiplies_mod = [](auto const l, auto const r) { return (l * r) % M; };
    template<std::size_t M>
    using multiplies_mod_t = decltype(multiplies_mod<M>);

    /**
     *  @brief Logical nand.
     */
    auto constexpr logical_nand = fnot(std::logical_and<>());
    using logical_nand_t = decltype(logical_nand);

    /**
     *  @brief Logical nor.
     */
    auto constexpr logical_nor = fnot(std::logical_or<>());
    using logical_nor_t = decltype(logical_nor);

    /**
     *  @brief Min.
     */
    auto constexpr min = [](auto const& l, auto const& r) { return std::min(l, r); }; // TODO to values, refs je overkill
    using min_t = decltype(min);

    /**
     *  @brief Max.
     */
    auto constexpr max = [](auto const& l, auto const& r) { return std::max(l, r); };
    using max_t = decltype(max);

    /**
     *  @brief Function that does nothing.
     */
    auto constexpr no_op = [](auto&&...) noexcept {};
    using no_op_t = decltype(no_op);

    /**
     *  @brief Returns true iif argument is not nullptr.
     */
    auto constexpr not_null = [](void* const arg) noexcept { return nullptr != arg; };
    using not_null_t = decltype(not_null);

    /**
     *  @brief Returns true if given container is not empty.
     */
    auto constexpr not_empty = [](auto const& c) { return !c.empty(); };
    using not_empty_t = decltype(not_empty);

    /**
     *  @brief Identity function.
     */
    auto constexpr identity = [](auto&& a) -> decltype(auto) { return std::forward<decltype(a)>(a); };
    using identity_t = decltype(identity);

    /**
     *  @brief Identity function for small value types.
     */
    auto constexpr identityv = [](auto const a) { return a; };
    using identityv_t = decltype(identityv);

    /**
     *  @brief Creates constant function that returns @p x .
     */
    auto constexpr const_ = [](auto&& x){ return [&x](auto&&...) -> decltype(auto) { return std::forward<decltype(x)>(x); }; };
    using const_t = decltype(const_);

    /**
     *  @brief Creates a constant function that returns @p x by value.
     */
    auto constexpr constv_ = [](auto const x){ return [x](auto&&...){ return x; }; };
    using constv_t = decltype(constv_);

    /**
     *  @brief Function that always returns true.
     */
    auto constexpr always_true = constv_(true);
    using always_true_t = decltype(always_true);
}

#endif