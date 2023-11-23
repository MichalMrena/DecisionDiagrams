#ifndef LIBTEDDY_SYMBOLIC_PROBABILITIES_HPP
#define LIBTEDDY_SYMBOLIC_PROBABILITIES_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/types.hpp>

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY
#include <ginac/ginac.h>

namespace teddy::symprobs::details
{
inline GiNaC::symbol& time_symbol ()
{
    static GiNaC::realsymbol t("t");
    return t;
}
} // namespace teddy::symprobs::details

namespace teddy::symprobs
{
/**
 *  \brief Wrapper around third-party-library-specific expression type
 */
class expression
{
public:
    expression(GiNaC::ex ex) : ex_(ex)
    {
    }

    auto evaluate (double const t) const -> double
    {
        GiNaC::ex result = GiNaC::evalf(ex_.subs(details::time_symbol() == t));
        return GiNaC::ex_to<GiNaC::numeric>(result).to_double();
    }

    auto as_underlying_unsafe () const -> GiNaC::ex
    {
        return ex_;
    }

    auto to_latex (std::ostream& ost) const -> void
    {
        ost << GiNaC::latex << ex_ << GiNaC::dflt;
    }

    // TODO
    auto to_matlab (std::ostream& ost) const -> void
    {
        ost << ex_;
    }

private:
    GiNaC::ex ex_;
};

/**
 *  \brief Exponential distribution
 */
inline auto exponential (double const rate) -> expression
{
    return {rate * GiNaC::exp(-rate * details::time_symbol())};
}

/**
 *  \brief Weibull distribution
 */
inline auto weibull (double const shape, double const scale) -> expression
{
    return {
        (shape / scale) * GiNaC::pow(details::time_symbol() / scale, shape - 1)
        * GiNaC::exp(-GiNaC::pow(details::time_symbol() / scale, shape))};
}

/**
 *  \brief Probability independent of time
 */
inline auto constant (double prob) -> expression
{
    return GiNaC::ex(prob);
}

/**
 *  \brief 1 - some other distribution
 */
inline auto complement (expression const& other) -> expression
{
    return GiNaC::ex(1) - other.as_underlying_unsafe();
}

/**
 *  \brief Transforms vector of probabilities to matrix of probabilities
 */
template<class Ps> // TODO concept?
auto as_matrix (Ps const& probs) -> std::vector<std::array<expression, 2>>
{
    std::vector<std::array<expression, 2>> matrix;
    for (expression const& expr : probs)
    {
        matrix.push_back(std::array {complement(expr), expr});
    }
    return matrix;
}
} // namespace teddy::symprobs

#endif
#endif