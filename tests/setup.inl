#include "setup.hpp"

namespace teddy::tests {

namespace details {

template<class... Ts>
struct match : Ts... {
  using Ts::operator()...;
};

template<class... Ts>
match(Ts...) -> match<Ts...>;

inline auto make_order(
  const std::variant<random_order_tag, default_order_tag, given_order_tag> &tag,
  const int var_count,
  tsl::rng_t &rng
) -> std::vector<int> {
  return std::visit(
    match {
      [&] (random_order_tag) {
        auto indices
          = tsl::fill_vector(var_count, [] (int x) { return x; });
        std::ranges::shuffle(indices, rng);
        return indices;
      },
      [&] (default_order_tag) {
        auto indices
          = tsl::fill_vector(var_count, [] (int x) { return x; });
        return indices;
      },
      [] (given_order_tag const &indices) { return indices.order_; }
    },
    tag
  );
}

inline auto make_domains (
  const std::variant<random_domains_tag, given_domains_tag> &tag,
  const int var_count,
  const int domain_max,
  tsl::rng_t &rng
) -> std::vector<int> {
  return std::visit(
    match {
      [&] (random_domains_tag) {
        auto dist = std::uniform_int_distribution<int>(2, domain_max);
        return tsl::fill_vector(var_count, [&rng, &dist] (auto) {
          return dist(rng);
        });
      },
      [] (given_domains_tag const &t) { return t.domains_; }
    },
    tag
  );
}

}  // namespace details

inline auto make_manager (bdd_manager_settings const &settings, tsl::rng_t &rng)
  -> bdd_manager {
  return {
    settings.varcount_,
    settings.nodecount_,
    details::make_order(settings.order_, settings.varcount_, rng)
  };
}

template<int M>
auto make_manager (mdd_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> mdd_manager<M> {
  return {
    settings.varcount_,
    settings.nodecount_,
    details::make_order(settings.order_, settings.varcount_, rng)
  };
}

template<int M>
auto make_manager (imdd_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> imdd_manager {
  return {
    settings.varcount_,
    settings.nodecount_,
    details::make_domains(
      settings.domains_,
      settings.varcount_,
      settings.domain_max_,
      rng),
    details::make_order(settings.order_, settings.varcount_, rng)
  };
}

template<int M>
auto make_manager (ifmdd_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> ifmdd_manager<M> {
  return {
    settings.varcount_,
    settings.nodecount_,
    details::make_domains(
      settings.domains_,
      settings.varcount_,
      settings.domain_max_,
      rng),
    details::make_order(settings.order_, settings.varcount_, rng)
  };
}

inline auto make_manager (bss_manager_settings const &settings, tsl::rng_t &rng)
  -> bss_manager {
  return {
    settings.varcount_,
    settings.nodecount_,
    details::make_order(settings.order_, settings.varcount_, rng)
  };
}

template<int M>
auto make_manager (mss_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> mss_manager<M> {
  return {
    settings.varcount_,
    settings.nodecount_,
    details::make_order(settings.order_, settings.varcount_, rng)
  };
}

template<int M>
auto make_manager (imss_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> imss_manager {
  return imss_manager(
    settings.varcount_,
    settings.nodecount_,
    details::make_domains(
      settings.domains_,
      settings.varcount_,
      settings.domain_max_,
      rng),
    details::make_order(settings.order_, settings.varcount_, rng)
  );
}

template<int M>
auto make_manager (ifmss_manager_settings<M> const &settings, tsl::rng_t &rng)
  -> ifmss_manager<M> {
  return ifmss_manager<M>(
    settings.varcount_,
    settings.nodecount_,
    details::make_domains(
      settings.domains_,
      settings.varcount_,
      settings.domain_max_,
      rng),
    details::make_order(settings.order_, settings.varcount_, rng)
  );
}

template<class Degree, class Domain>
auto make_diagram (
  tsl::minmax_expr const &expr,
  diagram_manager<Degree, Domain> &manager,
  fold_type const foldtype
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
  auto termDs     = std::vector<diagram_t>();
  for (auto const &eTerm : expr.terms_) {
    auto vars = manager.variables(eTerm);
    termDs.emplace_back(min_fold(vars));
  }
  return max_fold(termDs);
}

inline auto make_expression (
  minmax_expression_settings const &settings,
  tsl::rng_t &rng
) -> tsl::minmax_expr {
  return tsl::make_minmax_expression(
    rng,
    settings.varcount_,
    settings.termcount_,
    settings.termsize_
  );
}

inline auto make_expression (
  expression_tree_settings const &settings,
  tsl::rng_t &rng
) -> std::unique_ptr<tsl::expr_node> {
  return tsl::make_expression_tree(settings.varcount_, rng, rng);
}

}  // namespace teddy::tests
