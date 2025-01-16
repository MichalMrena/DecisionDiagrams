#include <libteddy/inc/io.hpp>

namespace teddy {

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

} // namespace teddy
