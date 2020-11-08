#ifndef MIX_UTILS_MORE_FUNCTIONAL_HPP
#define MIX_UTILS_MORE_FUNCTIONAL_HPP

#include <utility>

namespace mix::utils
{
    /**
        @brief Negates the result of given operation.
        Similar to `std::not_fn` but does not
        require an instance of given @p Op.
        @tparam Op stateless Callable type.
     */
    template<class Op>
    struct logical_negate
    {
        template<class... Args>
        [[nodiscard]] 
        constexpr auto operator() (Args&&... args) const
        {
            return ! Op () (std::forward<Args>(args)...);
        }
    };

    /**
        @brief Function object for function min.
        Unlike `std::min`, the result is return by value.
     */
    struct min
    {
        template<class T>
        [[nodiscard]]
        constexpr auto operator() 
            (T const& lhs, T const& rhs) const noexcept -> T
        {
            return std::min(lhs, rhs);
        }
    };

    /**
        @brief Function object for function max.
        Unlike `std::max`, the result is return by value.
     */
    struct max
    {
        template<class T>
        [[nodiscard]]
        constexpr auto operator() 
            (T const& lhs, T const& rhs) const noexcept -> T
        {
            return std::max(lhs, rhs);
        }
    };

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

    /**
        @brief Function that does nothing.
     */
    inline auto no_op = [](auto&&...) noexcept {};
    using no_op_t = decltype(no_op);

    /**
        @brief Function that always returns true.
     */
    inline auto always_true = [](auto&&...) noexcept { return true; };
    using always_true_t = decltype(always_true);

    /**
        @brief Function that returns true if argument is not nullptr and false otherwise.
     */
    inline auto not_null = [](void* const arg) noexcept { return nullptr != arg; };
    using not_null_t = decltype(not_null);
}

#endif