#ifndef LIBTEDDY_SYMBOLIC_PROBABILITIES_HPP
#define LIBTEDDY_SYMBOLIC_PROBABILITIES_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/types.hpp>
#include <cassert>

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY
#include <ginac/ginac.h>

namespace teddy::symprobs
{
namespace details
{
/**
 *  \brief All expressions need to share a single instance of the symbol
 */
inline GiNaC::symbol& time_symbol ()
{
    static GiNaC::realsymbol t("t");
    return t;
}
} // details

/**
 *  \brief Wrapper around third-party-library-specific expression type
 */
class expression
{
public:
    explicit expression(int32 const val) :
        ex_(val)
    {
    }

    explicit expression(int64 const val) :
        ex_(val)
    {
    }

    explicit expression(double const val) :
        ex_(val)
    {
    }

    explicit expression(GiNaC::ex ex) : ex_(ex)
    {
    }

    auto evaluate (double const t) const -> double
    {
        // TODO visitor?
        GiNaC::ex result = GiNaC::evalf(ex_.subs(details::time_symbol() == t));
        return GiNaC::ex_to<GiNaC::numeric>(result).to_double();
    }

    auto as_underlying_unsafe () -> GiNaC::ex&
    {
        return ex_;
    }

    auto as_underlying_unsafe () const -> GiNaC::ex
    {
        return ex_;
    }

    auto to_latex (std::ostream& ost) const -> void
    {
        ost << GiNaC::latex << ex_ << GiNaC::dflt;
    }

    auto to_matlab (std::ostream& ost) const -> void
    {
        ost << ex_;
    }

private:
    GiNaC::ex ex_;
};

namespace details
{
    inline auto operator+ (
        expression const& lhs,
        expression const& rhs
    ) -> expression
    {
        auto result = lhs.as_underlying_unsafe() + rhs.as_underlying_unsafe();
        return expression(result);
    }

    inline auto operator* (
        expression const& lhs,
        expression const& rhs
    ) -> expression
    {
        auto result = lhs.as_underlying_unsafe() * rhs.as_underlying_unsafe();
        return expression(result);
    }

    inline auto operator+= (
        expression& lhs,
        expression const& rhs
    ) -> expression&
    {
        lhs.as_underlying_unsafe() += rhs.as_underlying_unsafe();
        return lhs;
    }

    inline auto operator*= (
        expression& lhs,
        expression const& rhs
    ) -> expression&
    {
        lhs.as_underlying_unsafe() *= rhs.as_underlying_unsafe();
        return lhs;
    }
} // namespace details

/**
 *  \brief Creates CDF expression of Exponential distribution
 */
inline auto exponential (double const rate) -> expression
{
    auto& t = details::time_symbol();
    return expression(
        GiNaC::ex(1) - GiNaC::exp(-rate * t)
    );
}

/**
 *  \brief Creates CDF expression of Weibull distribution
 */
inline auto weibull (double const scale, double const shape) -> expression
{
    auto& t = details::time_symbol();
    return expression(
        GiNaC::ex(1) - GiNaC::exp(-GiNaC::pow(t / scale, shape))
    );
}

/**
 *  \brief Creates CDF expression of Normal distribution
 */
inline auto normal (double mean, double var) -> expression
{
    // TODO
    return expression(-1);
}

/**
 *  \brief Creates instance of Constant distribution
 */
inline auto constant (double prob) -> expression
{
    return expression(
        GiNaC::ex(prob)
    );
}

/**
 *  \brief Creates instance of Complemented disrtibution (1 - ohter)
 */
inline auto complement (expression const& other) -> expression
{
    return expression(
        GiNaC::ex(1) - other.as_underlying_unsafe()
    );
}

/**
 *  \brief Vector of time-independent probabilities (just for BSS)
 */
template<class Probabilities>
concept symprob_vector = requires(Probabilities probs, int32 index) {
                          {
                              probs[index]
                          } -> std::convertible_to<expression>;
                      };

/**
 *  \brief Matrix of time-independent probabilities
 */
template<class Probabilities>
concept symprob_matrix = requires(Probabilities probs, int32 index, int32 value) {
                          {
                              probs[index][value]
                          } -> std::convertible_to<expression>;
                      };

namespace details
{
/**
 *  \brief Helper for vector wrap
 *
 *  Returns expression of probability that the component is in state 1 `p_{i,1}`
 *  from \p vec or in case we need the probability that it is in state 0
 *  it calculates it as `1 - p_{i,1}`.
 */
template<class Vector>
class vector_to_matrix_proxy
{
public:
    vector_to_matrix_proxy(Vector const& vec, std::size_t const index) :
        index_(index),
        vec_(&vec)
    {
    }

    auto operator[] (std::size_t const value) const
    {
        assert(value == 1 || value == 0);
        expression expr = (*vec_)[index_];
        return value == 1 ? expr : complement(expr);
    }

private:
    std::size_t index_;
    Vector const* vec_;
};

/**
 *  \brief Wraps prob. vector so that it can be used as matrix
 *
 *  Algorithms working with probability require matrix \c ps such that
 *  \c ps[i][s] expression of probability that component \c i is in state \c s
 *  Since for BSS we only need to know probabilities that the component
 *  is in state 1 (p1) we can "fake" the matrix by wrapping the vector and
 *  calculating p0 as 1-p1
 */
template<class Vector>
class vector_to_matrix_wrap
{
public:
    vector_to_matrix_wrap(Vector const& vec) : vec_(&vec)
    {
    }

    auto operator[] (std::size_t const index) const
    {
        return vector_to_matrix_proxy<Vector>(*vec_, index);
    }

private:
    Vector const* vec_;
};
} // namespace details

/**
 *  \brief Wraps \p distVector so that it can be viewed as n x 2 matrix
 */
template<symprob_vector Ps>
auto as_matrix (Ps const& distVector)
{
    return details::vector_to_matrix_wrap<Ps>(distVector);
}

/**
 *  \brief Transforms \p distVector into n x 2 matrix
 */
template<symprob_vector Ps>
auto to_matrix (Ps const& distVector) -> std::vector<std::array<expression, 2>>
{
    std::vector<std::array<expression, 2>> matrix;
    for (expression const& expr : distVector)
    {
        matrix.push_back(std::array {complement(expr), expr});
    }
    return matrix;
}
} // namespace teddy::symprobs

#endif
#endif