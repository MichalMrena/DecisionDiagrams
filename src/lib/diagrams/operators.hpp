#ifndef MIX_DD_OPERATORS_
#define MIX_DD_OPERATORS_

#include "typedefs.hpp"

namespace mix::dd
{
    struct AND
    {
        auto operator() (bool_t const lhs, bool_t const rhs) const -> bool_t;
    };

    struct OR
    {
        auto operator() (bool_t const lhs, bool_t const rhs) const -> bool_t;
    };

    struct XOR
    {
        auto operator() (bool_t const lhs, bool_t const rhs) const -> bool_t;
    };

    struct NAND
    {
        auto operator() (bool_t const lhs, bool_t const rhs) const -> bool_t;
    };

    struct NOR
    {
        auto operator() (bool_t const lhs, bool_t const rhs) const -> bool_t;
    };

    struct PI_CONJ
    {
        auto operator() (bool_t const lhs, bool_t const rhs) const -> bool_t;
    };

    namespace aux_impl::bool_t_constants
    {
        auto constexpr N = log_val_traits<2>::nondetermined;
        auto constexpr U = log_val_traits<2>::undefined;
    }

    inline auto AND::operator() 
        (bool_t const lhs, bool_t const rhs) const -> bool_t
    {
        using namespace aux_impl::bool_t_constants;

        if (0 == lhs || 0 == rhs) return 0;
        if (N == lhs || N == rhs) return N;
        
        return lhs && rhs;
    }

    inline auto OR::operator() 
        (bool_t const lhs, bool_t const rhs) const -> bool_t
    {
        using namespace aux_impl::bool_t_constants;

        if (1 == lhs || 1 == rhs) return 1;
        if (N == lhs || N == rhs) return N;
        
        return lhs || rhs;
    }

    inline auto XOR::operator() 
        (bool_t const lhs, bool_t const rhs) const -> bool_t
    {
        using namespace aux_impl::bool_t_constants;

        if (N == lhs || N == rhs) return N;

        return lhs ^ rhs;
    }

    inline auto NAND::operator() 
        (bool_t const lhs, bool_t const rhs) const -> bool_t
    {
        using namespace aux_impl::bool_t_constants;

        if (0 == lhs || 0 == rhs) return 1;
        if (N == lhs || N == rhs) return N;
        
        return ! (lhs && rhs);
    }

    inline auto NOR::operator() 
        (bool_t const lhs, bool_t const rhs) const -> bool_t
    {
        using namespace aux_impl::bool_t_constants;

        if (1 == lhs || 1 == rhs) return 0;
        if (N == lhs || N == rhs) return N;
        
        return ! (lhs || rhs);
    }

    inline auto PI_CONJ::operator()
        (bool_t const lhs, bool_t const rhs) const -> bool_t
    {
        using namespace aux_impl::bool_t_constants;

        if (0 == lhs || 0 == rhs) return 0;       
        if (N == lhs || N == rhs) return N;
        if (U == lhs && U == rhs) return U;

        return 1;       
    }
}

#endif