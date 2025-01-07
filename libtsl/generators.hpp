#ifndef LIBTEDDY_TSL_GENERATORS_HPP
#define LIBTEDDY_TSL_GENERATORS_HPP

#include <libteddy/impl/probabilities.hpp>
#include <libteddy/impl/symbolic_probabilities.hpp>
#include <libteddy/inc/core.hpp>

#include <libtsl/expressions.hpp>

#include <array>
#include <random>
#include <vector>

namespace teddy::tsl {
template<class Degree, class Domain>
auto make_diagram (
  minmax_expr const &expr,
  diagram_manager<Degree, Domain> &manager,
  fold_type const foldtype = fold_type::Left
) {
  auto const min_fold = [&manager, foldtype] (auto &diagrams) {
    return foldtype == fold_type::Left
           ? manager.template left_fold<ops::MIN>(diagrams)
           : manager.template tree_fold<ops::MIN>(diagrams);
  };

  auto const max_fold = [&manager, foldtype] (auto &diagrams) {
    return foldtype == fold_type::Left
           ? manager.template left_fold<ops::MAX>(diagrams)
           : manager.template tree_fold<ops::MAX>(diagrams);
  };

  using diagram_t = typename diagram_manager<Degree, Domain>::diagram_t;
  std::vector<diagram_t> termDs;
  for (auto const &eTerm : expr.terms_) {
    auto vars = manager.variables(eTerm);
    termDs.push_back(min_fold(vars));
  }
  return max_fold(termDs);
}

template<class Degree, class Domain>
auto make_diagram (
  std::unique_ptr<tsl::expr_node> const &expr,
  diagram_manager<Degree, Domain> &manager
) {
  return manager.from_expression_tree(*expr);
}

template<class Degree, class Domain>
auto make_diagram (
  tsl::expr_node const &expr,
  diagram_manager<Degree, Domain> &manager
) {
  return manager.from_expression_tree(expr);
}

inline auto make_probability_vector (int32 const varCount, rng_t &rng)
  -> std::vector<double> {
  std::vector<double> probs(as_usize(varCount));
  for (int32 i = 0; i < ssize(probs); ++i) {
    std::uniform_real_distribution<double> dist(.0, 1.0);
    probs[as_uindex(i)] = dist(rng);
  }
  return probs;
}

template<std::size_t M>
auto make_probability_matrix (std::vector<int32> const &domains, rng_t &rng)
  -> std::vector<std::array<double, M>> {
  std::vector<std::array<double, M>> probs(domains.size());
  for (int32 i = 0; i < ssize(probs); ++i) {
    std::uniform_real_distribution<double> dist(.0, 1.0);
    for (int32 j = 0; j < domains[as_uindex(i)]; ++j) {
      probs[as_uindex(i)][as_uindex(j)] = dist(rng);
    }
    double const sum = std::reduce(
      begin(probs[as_uindex(i)]),
      end(probs[as_uindex(i)]),
      0.0,
      std::plus<>()
    );
    for (int32 j = 0; j < domains[as_uindex(i)]; ++j) {
      probs[as_uindex(i)][as_uindex(j)] /= sum;
    }
  }
  return probs;
}

inline auto make_time_probability_vector (int32 const varCount, rng_t &rng)
  -> std::vector<probs::prob_dist> {
  std::vector<probs::prob_dist> probs;

  auto const mkExponential = [] (rng_t &gen) -> probs::prob_dist {
    double const from = 0.2;
    double const to   = 1.0;
    std::uniform_real_distribution<double> distRate(from, to);
    return probs::exponential(distRate(gen));
  };

  // auto const mkWeibull = [] (rng_t& gen) -> probs::prob_dist
  // {
  //     std::uniform_real_distribution<double> distShape(0.5, 1.0);
  //     return probs::weibull(1.0, distShape(gen));
  // };

  // auto const mkConstant = [] (rng_t& gen) -> probs::prob_dist
  // {
  //     std::uniform_real_distribution<double> distProb(0.2, 1.0);
  //     return probs::constant(distProb(gen));
  // };

  std::vector<probs::prob_dist (*)(rng_t &)> distGenerators(
    // {+mkExponential, +mkWeibull, +mkConstant}
    {+mkExponential}
  );

  std::uniform_int_distribution<std::size_t> distGen(
    std::size_t {0},
    distGenerators.size() - 1
  );
  for (int i = 0; i < varCount; ++i) {
    auto const gen = distGenerators[distGen(rng)];
    probs.push_back(gen(rng));
  }

  return probs;
}

#ifdef LIBTEDDY_SYMBOLIC_RELIABILITY

inline auto make_time_symprobability_vector (int32 const varCount, rng_t &rng)
  -> std::vector<symprobs::expression> {
  std::vector<symprobs::expression> probs;

  auto const mkExponential = [] (rng_t &gen) -> symprobs::expression {
    std::uniform_real_distribution<double> distRate(0.2, 1.0);
    return symprobs::exponential(distRate(gen));
  };

  // auto const mkWeibull = [] (rng_t& gen) -> symprobs::expression
  // {
  //     std::uniform_real_distribution<double> distShape(0.5, 1.0);
  //     return symprobs::weibull(1.0, distShape(gen));
  // };

  // auto const mkConstant = [] (rng_t& gen) -> symprobs::expression
  // {
  //     std::uniform_real_distribution<double> distProb(0.2, 1.0);
  //     return symprobs::constant(distProb(gen));
  // };

  std::vector<symprobs::expression (*)(rng_t &)> distGenerators(
    // {+mkExponential, +mkWeibull, +mkConstant}
    {+mkExponential}
  );

  std::uniform_int_distribution<std::size_t> distGen(
    std::size_t {0},
    distGenerators.size() - 1
  );
  probs.reserve(as_usize(varCount));
  for (int i = 0; i < varCount; ++i) {
    probs.push_back(mkExponential(rng));
  }

  return probs;
}

#endif
} // namespace teddy::tsl

#endif