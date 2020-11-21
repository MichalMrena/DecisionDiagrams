#ifndef MIX_UTILS_MORE_FUNCTIONAL_HPP
#define MIX_UTILS_MORE_FUNCTIONAL_HPP

#include <functional>
#include <utility>

namespace mix::utils
{
    /**
        @brief Function object for plus mod m.
     */
    template<std::size_t M>
    struct plus_mod
    {
        static_assert(M > 0, "Division by zero.");
        template<class T>
        [[nodiscard]]
        constexpr auto operator() 
            (T const& lhs, T const& rhs) const noexcept -> T
        {
            return (lhs + rhs) % M;
        }
    };

    /**
        @brief Function object for multiplies mod m.
     */
    template<std::size_t M>
    struct multiplies_mod
    {
        static_assert(M > 0, "Division by zero.");
        template<class T>
        [[nodiscard]]
        constexpr auto operator() 
            (T const& lhs, T const& rhs) const noexcept -> T
        {
            return (lhs * rhs) % M;
        }
    };

    namespace impl
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
        @brief Function logical nand.
     */
    inline const auto logical_nand = impl::fnot(std::logical_and<>());
    using logical_nand_t = decltype(logical_nand);

    /**
        @brief Function logical nor.
     */
    inline const auto logical_nor = impl::fnot(std::logical_or<>());
    using logical_nor_t = decltype(logical_nor);

    /**
        @brief Function min.
     */
    inline auto const min = [](auto const& lhs, auto const& rhs) { return std::min(lhs, rhs); };
    using min_t = decltype(min);

    /**
        @brief Function max.
     */
    inline auto const max = [](auto const& lhs, auto const& rhs) { return std::max(lhs, rhs); };
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
        @brief Function that returns true if argument is not nullptr and false otherwise.
     */
    inline auto const not_null = [](void* const arg) noexcept { return nullptr != arg; };
    using not_null_t = decltype(not_null);

    /**
     *  @brief Function that returns true if given container is not empty.
     */
    inline auto const not_empty = [](auto const& c) { return !c.empty(); };
    using not_empty_t = decltype(not_empty);
}

#endif