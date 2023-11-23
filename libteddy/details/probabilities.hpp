#ifndef LIBTEDDY_PROBABILITIES_HPP
#define LIBTEDDY_PROBABILITIES_HPP

#include <libteddy/details/config.hpp>
#include <libteddy/details/types.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <concepts>
#include <functional>
#include <ostream>
#include <variant>
#include <vector>

namespace teddy::probs
{
namespace details
{
/**
 *  \brief Helper for vector wrap
 *
 *  Returns probability that the component is in state 1 p_{i,1} from \p vec
 *  or in case we need the probability that it is in state 0 it calculates
 *  it as `1 - p_{i,1}`.
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
 *
 *  Algorithms working with probability require matrix \c ps such that
 *  \c ps[i][s] returns probability that \c i th component is in state \c s
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

/**
 *  \brief Mixin that adds "memory" to any \c ProbDist
 *
 *  If asked (by calling \c cache_eval_at ), remembers computed value
 */
template<class ProbDist>
class make_cached
{
public:
    auto cache_eval_at (double const t) -> void
    {
        value_ = (*static_cast<ProbDist*>(this))(t);
    }

    auto get_cached_value () const -> double
    {
        return value_;
    }

private:
    double value_;
};

/**
 *  \brief Exponential distribution
 */
class exponential : public make_cached<exponential>
{
public:
    exponential(double const rate) : rate_(rate)
    {
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
class weibull : public make_cached<weibull>
{
public:
    weibull(double const scale, double const shape) :
        scale_(scale),
        shape_(shape)
    {
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
class constant : public make_cached<constant>
{
public:
    constant(double const value) : value_(value)
    {
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
class custom_dist : public make_cached<custom_dist>
{
public:
    custom_dist(std::function<double(double)> dist) : dist_(std::move(dist))
    {
    }

    [[nodiscard]] auto operator() (double const t) const -> double
    {
        return dist_(t);
    }

private:
    std::function<double(double)> dist_;
};
} // namespace details

using dist_variant = std::variant<
    details::exponential,
    details::weibull,
    details::constant,
    details::custom_dist
>;

/**
 *  \brief "Interface" for distributions, manages variant access
 */
class prob_dist
{
public:
    prob_dist(dist_variant dist) : dist_(dist)
    {
    }

    auto cache_eval_at (double const t) -> void
    {
        std::visit([t] (auto& d) { d.cache_eval_at(t); }, dist_);
    }

    [[nodiscard]] operator double () const
    {
        return std::visit(
            [] (auto const& dist) -> double { return dist.get_cached_value(); },
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

/**
 *  \brief Creates instance of Exponential distribution
 */
inline auto exponential (double const rate) -> prob_dist
{
    return prob_dist(details::exponential(rate));
}

/**
 *  \brief Creates instance of Weibull distribution
 */
inline auto weibull (double const scale, double const shape) -> prob_dist
{
    return prob_dist(details::weibull(scale, shape));
}

/**
 *  \brief Creates instance of Constant disrtibution
 */
inline auto constant (double prob) -> prob_dist
{
    return prob_dist(details::constant(prob));
}

/**
 *  \brief Creates instance of Constant disrtibution
 */
inline auto custom (std::function<double(double)> dist) -> prob_dist
{
    return prob_dist(details::custom_dist(std::move(dist)));
}

/**
 *  \brief Vector of time-independent probabilities (just for BSS)
 */
template<class Probabilities>
concept prob_vector = requires(Probabilities probs, int32 index) {
                          {
                              probs[index]
                          } -> std::convertible_to<double>;
                      };

/**
 *  \brief Matrix of time-independent probabilities
 */
template<class Probabilities>
concept prob_matrix = requires(Probabilities probs, int32 index, int32 value) {
                          {
                              probs[index][value]
                          } -> std::convertible_to<double>;
                      };

/**
 *  \brief Vector of time-dependent probabilities (distributions) (just for BSS)
 */
template<class T>
concept dist_vector = requires(T t, std::size_t i) {
                          {
                              t[i]
                          } -> std::convertible_to<prob_dist>;

                          t[i].cache_eval_at(3.14);
                      };

/**
 *  \brief Matrix of time-dependent probabilities (distributions)
 */
template<class T>
concept dist_matrix = requires(T t, std::size_t i) {
                          {
                              t[i][i]
                          } -> std::convertible_to<prob_dist>;

                          t[i][i].cache_eval_at(3.14);
                      };

/**
 *  \brief Evaluates each dist in \p distVector at time \p t
 */
template<dist_vector Ps>
auto eval_at (Ps& distVector, double const t) -> Ps&
{
    for (auto& dist : distVector)
    {
        dist.cache_eval_at(t);
    }
    return distVector;
}

/**
 *  \brief Evaluates each dist in \p distMatrix at time \p t
 */
template<dist_matrix Ps>
auto eval_at (Ps& distMatrix, double const t) -> Ps&
{
    for (auto& dists : distMatrix)
    {
        for (auto& dist : dists)
        {
            dist.cache_eval_at(t);
        }
    }
    return distMatrix;
}
} // namespace teddy::probs

#endif