#ifndef LIBTEDDY_TSL_GENERATORS_HPP
#define LIBTEDDY_TSL_GENERATORS_HPP

#include <libtsl/inc/expressions.hpp>

#include <teddy/impl/probabilities.hpp>
#include <teddy/impl/symbolic_probabilities.hpp>
#include <teddy/core.hpp>

#include <array>
#include <random>
#include <vector>

namespace teddy::tsl {

template<class Degree, class Domain>
auto make_diagram (
  minmax_expr const &expr,
  diagram_manager<Degree, Domain> &manager,
  fold_type foldtype = fold_type::Left
);

template<class Degree, class Domain>
auto make_diagram (
  std::unique_ptr<tsl::expr_node> const &expr,
  diagram_manager<Degree, Domain> &manager
);

template<class Degree, class Domain>
auto make_diagram (
  tsl::expr_node const &expr,
  diagram_manager<Degree, Domain> &manager
);

auto make_probability_vector (int varCount, std::ranlux48 &rng)
  -> std::vector<double>;

template<std::size_t M>
auto make_probability_matrix (std::vector<int> const &domains, std::ranlux48 &rng)
  -> std::vector<std::array<double, M>>;

auto make_time_probability_vector (int varCount, std::ranlux48 &rng)
  -> std::vector<probs::prob_dist>;

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY

auto make_time_symprobability_vector (int varCount, std::ranlux48 &rng)
  -> std::vector<symprobs::expression>;

#endif

} // namespace teddy::tsl

#include <libtsl/impl/generators.inl>

#endif
