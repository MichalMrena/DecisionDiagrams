#include <libteddy/inc/io.hpp>

namespace teddy {

template<class Degree, class Domain>
static auto from_pla(
  diagram_manager<Degree, Domain> &manager,
  const pla_file_mvl &file
) -> diagram_manager<Degree, Domain> {
  using mdd_t = diagram_manager<Degree, Domain>::diagram_t;

  // Zero as neutral element for MAX
  mdd_t result = manager.constant(0);

  // For each input line
  for (int li = 0; li < file.product_count_; ++li) {
    // One as neutral element for AND
    mdd_t product = manager.constant(1);

    for (int i = 0; i < file.input_count_; ++i) {
      // Value of the variable
      const int var_val = file.inputs_[as_uindex(li)][i];

      // Basic single variable diagram
      mdd_t var = manager.variable(i);

      // Transform it so that it is 1 for var_val and 0 otherwise
      var = manager.transform(var, [var_val](const int val){
        return static_cast<int>(val == var_val);
      });

      // Add it to the product just like in binary pla
      product = manager.template apply<ops::AND>(product, var);
    }

    // Output of given product
    const int output = file.output_[as_uindex(li)];

    // Transform product from {0,1} to [0,1,...,output-1]
    product = manager.transform(product, [output](const int val){
      return static_cast<int>(val == output);
    });

    // Add product to the result
    result = manager.template apply<ops::MAX>(result, product);
  }

  return result;
}

template<class Degree, class Domain>
auto io::to_dot(
  diagram_manager<Degree, Domain> const &manager,
  std::ostream &out
) -> void {
  details::io_impl::to_dot_graph_common(
    manager,
    out,
    [&manager] (auto const &f) { manager.nodes_.for_each_node(f); }
  );
}

template<class Degree, class Domain>
auto io::to_dot(
  diagram_manager<Degree, Domain> const &manager,
  std::ostream &out,
  diagram_manager<Degree, Domain>::diagram_t const &diagram
) -> void {
  details::io_impl::to_dot_graph_common(
    manager,
    out,
    [&manager, &diagram] (auto const &f) {
      manager.nodes_.traverse_level(diagram.unsafe_get_root(), f);
    }
  );
}

template<
  class Degree,
  class Domain,
  std::input_iterator I,
  std::sentinel_for<I> S>
auto io::from_vector(
  diagram_manager<Degree, Domain> &manager,
  I first,
  S last
) -> diagram_manager<Degree, Domain>::diagram_t {
  using manager_t     = diagram_manager<Degree, Domain>;
  using diagram_t     = typename manager_t::diagram_t;
  using son_container = typename manager_t::son_container;
  using node_t        = typename manager_t::node_t;

  using stack_frame   = struct {
    node_t *node;
    int32 level;
  };

  if (0 == manager.get_var_count()) {
    assert(first != last && std::next(first) == last);
    return manager.constant(*first);
  }

  int32 const terminalLevel = manager.get_var_count();

#ifndef NDEBUG
  if constexpr (std::random_access_iterator<I>) {
    int64 const count = manager.nodes_.domain_product(0, terminalLevel);
    int64 const dist  = std::distance(first, last);
    assert(dist > 0 && dist == count);
  }
#endif

  std::vector<stack_frame> stack;
  while (first != last) {
    node_t *const node = manager.nodes_.make_terminal_node(*first++);
    stack.push_back(stack_frame {node, terminalLevel});

    for (;;) {
      int32 const currentLevel = stack.back().level;
      if (0 == currentLevel) {
        break;
      }

      auto const endId = rend(stack);
      auto stackIt     = rbegin(stack);
      int64 count      = 0;
      while (stackIt != endId && stackIt->level == currentLevel) {
        ++stackIt;
        ++count;
      }
      int32 const newIndex  = manager.nodes_.get_index(currentLevel - 1);
      int32 const newDomain = manager.nodes_.get_domain(newIndex);

      if (count < newDomain) {
        break;
      }

      son_container sons = node_t::make_son_container(newDomain);
      for (int32 k = 0; k < newDomain; ++k) {
        sons[k] = stack[as_uindex(ssize(stack) - newDomain + k)].node;
      }
      node_t *const newNode
        = manager.nodes_.make_internal_node(newIndex, TEDDY_MOVE(sons));
      stack.erase(end(stack) - newDomain, end(stack));
      stack.push_back(stack_frame {newNode, currentLevel - 1});
    }
  }

  assert(ssize(stack) == 1);
  return diagram_t(stack.back().node);
}

template<class Degree, class Domain, std::ranges::input_range R>
auto io::from_vector(diagram_manager<Degree, Domain> &manager, R const &vector)
  -> diagram_manager<Degree, Domain>::diagram_t {
  return io::from_vector(manager, begin(vector), end(vector));
}

template<class Degree, class Domain>
auto io::from_vector(
  diagram_manager<Degree, Domain> &manager,
  std::initializer_list<int> const vector
) -> diagram_manager<Degree, Domain>::diagram_t {
  return io::from_vector(manager, begin(vector), end(vector));
}

template<class Degree, class Domain>
auto io::to_vector(
  diagram_manager<Degree, Domain> const &manager,
  diagram_manager<Degree, Domain>::diagram_t const &diagram
) -> std::vector<int32> {
  std::vector<int32> vector;
  vector.reserve(
    as_usize(manager.nodes_.domain_product(0, manager.get_var_count()))
  );
  io::to_vector_g(manager, diagram, std::back_inserter(vector));
  return vector;
}

template<class Degree, class Domain, std::output_iterator<teddy::int32> O>
auto io::to_vector_g(
  diagram_manager<Degree, Domain> const &manager,
  diagram_manager<Degree, Domain>::diagram_t const &diagram,
  O out
) -> void {
  // TODO(michal): move for-each-in-domain to tools
  if (manager.get_var_count() == 0) {
    assert(diagram.unsafe_get_root()->is_terminal());
    *out++ = diagram.unsafe_get_root()->get_value();
    return;
  }

  std::vector<int32> vars(as_usize(manager.get_var_count()));
  bool wasLast = false;
  do {
    *out++        = manager.evaluate(diagram, vars);
    bool overflow = true;
    int32 level   = manager.nodes_.get_leaf_level();
    while (level > 0 && overflow) {
      --level;
      int32 const index = manager.nodes_.get_index(level);
      ++vars[as_uindex(index)];
      overflow = vars[as_uindex(index)] == manager.nodes_.get_domain(index);
      if (overflow) {
        vars[as_uindex(index)] = 0;
      }

      wasLast = overflow && 0 == level;
    }
  } while (not wasLast);
}

} // namespace teddy
