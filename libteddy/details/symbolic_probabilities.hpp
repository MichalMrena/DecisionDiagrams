#ifndef LIBTEDDY_SYMBOLIC_PROBABILITIES_HPP
#define LIBTEDDY_SYMBOLIC_PROBABILITIES_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/types.hpp>

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY
#include <ginac/ginac.h>

namespace teddy::symprobs
{
namespace details
{
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
    expression(int32 const val) :
        ex_(val)
    {
    }

    expression(int64 const val) :
        ex_(val)
    {
    }

    expression(double const val) :
        ex_(val)
    {
    }

    expression(GiNaC::ex ex) : ex_(ex)
    {
    }

    auto evaluate (double const t) const -> double
    {
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
    auto operator+ (expression const& lhs, expression const& rhs) -> expression
    {
        auto result = lhs.as_underlying_unsafe() + rhs.as_underlying_unsafe();
        return expression(result);
    }

    auto operator* (expression const& lhs, expression const& rhs) -> expression
    {
        auto result = lhs.as_underlying_unsafe() * rhs.as_underlying_unsafe();
        return expression(result);
    }

    auto operator+= (expression& lhs, expression const& rhs) -> expression&
    {
        lhs.as_underlying_unsafe() += rhs.as_underlying_unsafe();
        return lhs;
    }

    auto operator*= (expression& lhs, expression const& rhs) -> expression&
    {
        lhs.as_underlying_unsafe() *= rhs.as_underlying_unsafe();
        return lhs;
    }
} // namespace details

/**
 *  \brief Creates instance of Exponential distribution
 */
inline auto exponential (double const rate) -> expression
{
    return {rate * GiNaC::exp(-rate * details::time_symbol())};
}

/**
 *  \brief Creates instance of Weibull distribution
 */
inline auto weibull (double const scale, double const shape) -> expression
{
    return {
        (shape / scale) * GiNaC::pow(details::time_symbol() / scale, shape - 1)
        * GiNaC::exp(-GiNaC::pow(details::time_symbol() / scale, shape))};
}

/**
 *  \brief Creates instance of Constant disrtibution
 */
inline auto constant (double prob) -> expression
{
    return GiNaC::ex(prob);
}

/**
 *  \brief Creates instance of Complemented disrtibution (1 - ohter)
 */
inline auto complement (expression const& other) -> expression
{
    return GiNaC::ex(1) - other.as_underlying_unsafe();
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

/**
 *  \brief Wraps \p distVector so that it can be viewed as n x 2 matrix
 */
template<symprob_vector Ps>
auto as_matrix (Ps const& distVector)
{
    // TODO
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