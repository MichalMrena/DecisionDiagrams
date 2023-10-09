#ifndef LIBTEDDY_PROBABILITIES_HPP
#define LIBTEDDY_PROBABILITIES_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/types.hpp>

#include <ginac/ex.h>

#include <array>
#include <cassert>
#include <cmath>
#include <concepts>
#include <functional>
#include <ostream>
#include <variant>
#include <vector>

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY
#    include <ginac/ginac.h>
#endif

namespace teddy::probs::details
{
/**
 *  \brief Helper for vector wrap
 */
template<class Vector>
class prob_vector_wrap_proxy
{
public:
    prob_vector_wrap_proxy(Vector const& vec, std::size_t const index) :
        index_(index),
        vec_(&vec)
    {
    }

    auto operator[] (std::size_t const value) const -> double
    {
        assert(value == 1 || value == 0);
        double const prob = (*vec_)[index_];
        return value == 1 ? prob : 1 - prob;
    }

private:
    std::size_t index_;
    Vector const* vec_;
};

/**
 *  \brief Wraps prob. vector so that it can be used as matrix
 */
template<class Vector>
class prob_vector_wrap
{
public:
    prob_vector_wrap(Vector const& vec) : vec_(&vec)
    {
    }

    auto operator[] (std::size_t const index) const
    {
        return prob_vector_wrap_proxy<Vector>(*vec_, index);
    }

private:
    Vector const* vec_;
};

/**
 *  \brief Base class for probability distributions.
 *  Just holds time member that is common for all.
 */
class dist_base
{
public:
    auto set_t (double const t) -> void
    {
        t_ = t;
    }

protected:
    double t_ {0};
};
} // namespace teddy::probs::details

namespace teddy::probs
{
/**
 *  \brief Vector of probabilities for BSS
 */
template<class Probabilities>
concept prob_vector = requires(Probabilities probs, int32 index) {
                          {
                              probs[index]
                          } -> std::convertible_to<double>;
                      };

/**
 *  \brief Matrix of probabilities for BSS and MSS
 */
template<class Probabilities>
concept prob_matrix = requires(Probabilities probs, int32 index, int32 value) {
                          {
                              probs[index][value]
                          } -> std::convertible_to<double>;
                      };

/**
 *  \brief Exponential distribution
 */
class exponential : public details::dist_base
{
public:
    exponential(double const rate) : rate_(rate)
    {
    }

    [[nodiscard]] operator double () const
    {
        return (*this)(dist_base::t_);
    }

    [[nodiscard]] auto operator() (double const t) const -> double
    {
        return rate_ * std::exp(-rate_ * t);
    }

private:
    double rate_;
};

/**
 *  \brief Weibull distribution
 */
class weibull : public details::dist_base
{
public:
    weibull(double const scale, double const shape) :
        scale_(scale),
        shape_(shape)
    {
    }

    [[nodiscard]] operator double () const
    {
        return (*this)(dist_base::t_);
    }

    [[nodiscard]] auto operator() (double const t) const -> double
    {
        return (shape_ / scale_) * std::pow(t / scale_, shape_ - 1)
             * std::exp(-std::pow(t / scale_, shape_));
    }

private:
    double scale_;
    double shape_;
};

/**
 *  \brief Probability independent of time
 */
class constant : public details::dist_base
{
public:
    constant(double const value) : value_(value)
    {
    }

    [[nodiscard]] operator double () const
    {
        return value_;
    }

    [[nodiscard]] auto operator() (double const) const -> double
    {
        return value_;
    }

private:
    double value_;
};

/**
 *  \brief User-defined distribution
 */
class custom_dist : public details::dist_base
{
public:
    custom_dist(std::function<double(double)> dist) : dist_(dist)
    {
    }

    [[nodiscard]] operator double () const
    {
        return (*this)(dist_base::t_);
    }

    [[nodiscard]] auto operator() (double const t) const -> double
    {
        return dist_(t);
    }

private:
    std::function<double(double)> dist_;
};

using dist_variant = std::variant<exponential, weibull, constant, custom_dist>;

/**
 *  \brief "Interface" for distributions that manages variant access
 */
class prob_dist
{
public:
    prob_dist(dist_variant dist) : dist_(dist)
    {
    }

    auto set_t (double const t) -> void
    {
        std::visit([t] (auto& d) { d.set_t(t); }, dist_);
    }

    [[nodiscard]] operator double () const
    {
        return std::visit(
            [] (auto const& dist) -> double { return dist; },
            dist_
        );
    }

    [[nodiscard]] auto operator() (double const t) const -> double
    {
        return std::visit(
            [t] (auto const& dist) -> double { return dist(t); },
            dist_
        );
    }

private:
    dist_variant dist_;
};

template<class T>
concept dist_vector = requires(T t, std::size_t i) {
                          {
                              t[i]
                          } -> std::convertible_to<prob_dist>;

                          t[i].set_t(3.14);
                      };

template<class T>
concept dist_matrix = requires(T t, std::size_t i) {
                          {
                              t[i][i]
                          } -> std::convertible_to<prob_dist>;

                          t[i][i].set_t(3.14);
                      };

template<dist_vector Ps>
auto at_time (Ps& distVector, double const t) -> Ps&
{
    for (auto& dist : distVector)
    {
        dist.set_t(t);
    }
    return distVector;
}

template<dist_matrix Ps>
auto at_time (Ps& distMatrix, double const t) -> Ps&
{
    for (auto& dists : distMatrix)
    {
        for (auto& dist : dists)
        {
            dist.set_t(t);
        }
    }
    return distMatrix;
}
} // namespace teddy::probs

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY
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