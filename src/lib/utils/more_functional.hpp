#ifndef MIX_UTILS_MORE_FUNCTIONAL_HPP
#define MIX_UTILS_MORE_FUNCTIONAL_HPP

#include <functional>
#include <utility>

namespace mix::utils
{
    namespace mf_impl
    {
        inline auto const fnot = [](auto const f)
        {
            using F = decltype(f);
            return [](auto&&... args)
            {
                return ! F {} (std::forward<decltype(args)>(args)...);
            };
        };
    }

    /**
        @brief Modular addition.
     */
    template<std::size_t M>
    inline auto const plus_mod = [](auto const l, auto const r) { return (l + r) % M; };
    template<std::size_t M>
    using plus_mod_t = decltype(plus_mod<M>);

    /**
        @brief Modular multiplication.
     */
    template<std::size_t M>
    inline auto const multiplies_mod = [](auto const l, auto const r) { return (l * r) % M; };
    template<std::size_t M>
    using multiplies_mod_t = decltype(multiplies_mod<M>);

    /**
        @brief Logical nand.
     */
    inline auto const logical_nand = mf_impl::fnot(std::logical_and<>());
    using logical_nand_t = decltype(logical_nand);

    /**
        @brief Logical nor.
     */
    inline auto const logical_nor = mf_impl::fnot(std::logical_or<>());
    using logical_nor_t = decltype(logical_nor);

    /**
        @brief Min.
     */
    inline auto const min = [](auto const& l, auto const& r) { return std::min(l, r); };
    using min_t = decltype(min);

    /**
        @brief Max.
     */
    inline auto const max = [](auto const& l, auto const& r) { return std::max(l, r); };
    using max_t = decltype(max);

    /**
        @brief Function that does nothing.
     */
    inline auto const no_op = [](auto&&...) noexcept {};
    using no_op_t = decltype(no_op);

    /**
        @brief Function that always returns true.
     */
    inline auto const always_true = [](auto&&...) noexcept { return true; };
    using always_true_t = decltype(always_true);

    /**
        @brief Returns true iif argument is not nullptr.
     */
    inline auto const not_null = [](void* const arg) noexcept { return nullptr != arg; };
    using not_null_t = decltype(not_null);

    /**
     *  @brief Returns true if given container is not empty.
     */
    inline auto const not_empty = [](auto const& c) { return !c.empty(); };
    using not_empty_t = decltype(not_empty);

    /**
     * @brief Identity function.
     */
    inline auto const identity = [](auto&& a) -> decltype(auto) { return std::forward<decltype(a)>(a); };
    using identity_t = decltype(identity);
}

#endif