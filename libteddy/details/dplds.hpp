#ifndef LIBTEDDY_DETAILS_DPLDS_HPP
#define LIBTEDDY_DETAILS_DPLDS_HPP

#include <libteddy/details/types.hpp>

namespace teddy::dpld
{
/**
 *  \brief Returns lambda that can be used in basic \c dpld
 */
inline static auto constexpr basic = [] (int32 const fFrom, int32 const fTo)
{
    return [=] (int32 const lhs, int32 const rhs)
    { return lhs == fFrom && rhs == fTo; };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 1
 */
inline static auto constexpr type_1_decrease = [] (int32 const state)
{
    return [state] (int32 const lhs, int32 const rhs)
    { return lhs == state && rhs < state; };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 1
 */
inline static auto constexpr type_1_increase = [] (int32 const state)
{
    return [state] (int32 const lhs, int32 const rhs)
    { return lhs == state && rhs > state; };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 2
 */
inline static auto constexpr type_2_decrease = [] ()
{ return [] (int32 const lhs, int32 const rhs) { return lhs > rhs; }; };

/**
 *  \brief Returns lambda that can be used in \c dpld of type 2
 */
inline static auto constexpr type_2_increase = [] ()
{ return [] (int32 const lhs, int32 const rhs) { return lhs < rhs; }; };

/**
 *  \brief Returns lambda that can be used in \c dpld of type 3
 */
inline static auto constexpr type_3_decrease = [] (int32 const state)
{
    return [state] (int32 const lhs, int32 const rhs)
    { return lhs >= state && rhs < state; };
};

/**
 *  \brief Returns lambda that can be used in \c dpld of type 3
 */
inline static auto constexpr type_3_increase = [] (int32 const state)
{
    return [state] (int32 const lhs, int32 const rhs)
    { return lhs < state && rhs >= state; };
};
} // namespace teddy::dpld

#endif