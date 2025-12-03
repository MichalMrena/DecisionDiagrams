#ifndef LIBTEDDY_LIBTSL_PROBABILITIES_HPP
#define LIBTEDDY_LIBTSL_PROBABILITIES_HPP

#include <libteddy/inc/core.hpp>

#include <random>

namespace teddy::tsl {

/**
 * \brief Makes random component state probabilities vector
 */
template<class Degree, class Domain>
auto make_prob_vector (
  diagram_manager<Degree, Domain> const &manager,
  std::ranlux48 &rng
) -> std::vector<double>;

/**
 * \brief Makes random component state probabilities matrix
 */
template<class Degree, class Domain>
auto make_prob_matrix (
  diagram_manager<Degree, Domain> const &manager,
  std::ranlux48 &rng
) -> std::vector<std::vector<double>>;

}  // namespace teddy::tsl

#include <libtsl/impl/probabilities.inl>

#endif
