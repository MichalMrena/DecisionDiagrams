#ifndef LIBTEDDY_TESTS_SETUP_HPP
#define LIBTEDDY_TESTS_SETUP_HPP

#include <libteddy/inc/core.hpp>
#include <libteddy/inc/reliability.hpp>

#include <libtsl/expressions.hpp>
#include <libtsl/iterators.hpp>
#include <libtsl/utilities.hpp>

#include <functional>
#include <iterator>
#include <numeric>
#include <variant>
#include <vector>

namespace teddy::tests {

/**
 * \brief Specifies that order should be randomly generated
 */
struct random_order_tag { };

/**
 * \brief Specifies that order should simply follow the indices
 */
struct default_order_tag { };

/**
 * \brief Explicitly gives the order
 */
struct given_order_tag {
  std::vector<int> order_;
};

/**
 * \brief Specifies that domains should be randomly generated
 */
struct random_domains_tag { };

/**
 * \brief Explicitly gives the domains
 */
struct given_domains_tag {
  std::vector<int> domains_;
};

/**
 * \brief Describes how to initialize bdd_manager
 */
struct bdd_manager_settings {
  int domain_max_ {2};
  int varcount_;
  int nodecount_;
  std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
};

/**
 * \brief Describes how to initialize mdd_manager
 */
template<int M>
struct mdd_manager_settings {
  int domain_max_ {M};
  int varcount_;
  int nodecount_;
  std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
};

/**
 * \brief Describes how to initialize imdd_manager
 */
template<int M>
struct imdd_manager_settings {
  int domain_max_ {M};
  int varcount_;
  int nodecount_;
  std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
  std::variant<random_domains_tag, given_domains_tag> domains_;
};

/**
 * \brief Describes how to initialize ifmdd_manager
 */
template<int M>
struct ifmdd_manager_settings {
  int domain_max_ {M};
  int varcount_;
  int nodecount_;
  std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
  std::variant<random_domains_tag, given_domains_tag> domains_;
};

/**
 * \brief Describes how to initialize bss_manager
 */
struct bss_manager_settings {
  int domain_max_ {2};
  int varcount_;
  int nodecount_;
  std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
};

/**
 * \brief Describes how to initialize mss_manager
 */
template<int M>
struct mss_manager_settings {
  int domain_max_ {M};
  int varcount_;
  int nodecount_;
  std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
};

/**
 * \brief Describes how to initialize imss_manager
 */
template<int M>
struct imss_manager_settings {
  int domain_max_ {M};
  int varcount_;
  int nodecount_;
  std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
  std::variant<random_domains_tag, given_domains_tag> domains_;
};

/**
 * \brief Describes how to initialize ifmss_manager
 */
template<int M>
struct ifmss_manager_settings {
  int domain_max_ {M};
  int varcount_;
  int nodecount_;
  std::variant<random_order_tag, default_order_tag, given_order_tag> order_;
  std::variant<random_domains_tag, given_domains_tag> domains_;
};

/**
 * \brief Settings for the generation of minmax expression
 */
struct minmax_expression_settings {
  int varcount_;
  int termcount_;
  int termsize_;
};

/**
 * \brief Settings for the generations of expression tree
 */
struct expression_tree_settings {
  int varcount_;
};

/**
 * \brief Makes BDD manager
 */
inline auto make_manager (bdd_manager_settings const &settings, tsl::rng_t &rng)
  -> bdd_manager;

/**
 * \brief Makes MDD manager
 */
template<int M>
auto make_manager (mdd_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> mdd_manager<M>;

/**
 * \brief Makes iMDD manager
 */
template<int M>
auto make_manager (imdd_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> imdd_manager;

/**
 * \brief Makes ifMDD manager
 */
template<int M>
auto make_manager (ifmdd_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> ifmdd_manager<M>;

/**
 * \brief Makes bss_manager
 */
inline auto make_manager (bss_manager_settings const &settings, tsl::rng_t &rng)
  -> bss_manager;

/**
 * \brief Makes mss_manager
 */
template<int M>
auto make_manager (mss_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> mss_manager<M>;

/**
 * \brief Makes imss_manager
 */
template<int M>
auto make_manager (imss_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> imss_manager;

/**
 * \brief Makes ifmss_manager
 */
template<int M>
auto make_manager (ifmss_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> ifmss_manager<M>;

/**
 * \brief Makes diagram representing \p expr
 */
template<class Degree, class Domain>
auto make_diagram (
  tsl::minmax_expr const &expr,
  diagram_manager<Degree, Domain> &manager,
  fold_type foldtype = fold_type::Left
);

/**
 * \brief Makes minmax expression with given settings
 */
inline auto make_expression (
  minmax_expression_settings const &settings,
  tsl::rng_t &rng
) -> tsl::minmax_expr;

/**
 * \brief Makes expression tree with given settings
 */
inline auto make_expression (
  expression_tree_settings const &settings,
  tsl::rng_t &rng
) -> std::unique_ptr<tsl::expr_node>;


// TODO(michal): find place for this one
inline auto make_vector (
  std::unique_ptr<tsl::expr_node> const &root,
  std::vector<int> const &domains
) -> std::vector<int> {
  auto vector = std::vector<int>();
  vector.reserve(
    as_usize(std::reduce(begin(domains), end(domains), 1, std::multiplies<>()))
  );
  auto domainit = tsl::domain_iterator(domains);
  auto evalit   = tsl::evaluating_iterator(domainit, *root);
  auto evalend  = tsl::evaluating_iterator_sentinel();
  auto output   = std::back_inserter(vector);
  while (evalit != evalend) {
    *output++ = *evalit++;
  }
  return vector;
}

// TODO(michal): find place for this one
template<class Degree, class Domain>
auto make_domain_iterator (diagram_manager<Degree, Domain> const &manager) {
  return tsl::domain_iterator(manager.get_domains(), manager.get_order());
}

} // namespace teddy::tests

#include "setup.inl"

#endif
