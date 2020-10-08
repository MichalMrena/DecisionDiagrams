#ifndef MIX_UTILS_MORE_FUNCTIONAL_HPP
#define MIX_UTILS_MORE_FUNCTIONAL_HPP

#include <utility>

namespace mix::utils
{
    /**
        Negates the result of given operation.
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
}

#endif