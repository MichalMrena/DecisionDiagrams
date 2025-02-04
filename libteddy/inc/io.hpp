#ifndef LIBTEDDY_INC_IO_HPP
#define LIBTEDDY_INC_IO_HPP

#include <libteddy/impl/io_impl.hpp>
#include <libteddy/impl/pla.hpp>
#include <libteddy/inc/core.hpp>
#include <libteddy/inc/reliability.hpp>

#include <cstdlib>
#include <initializer_list>
#include <iterator>

namespace teddy {

struct io {
  /**
   *  \brief Creates BDDs defined by PLA file
   *
   *  \param manager Diagram manager
   *  \param file PLA file
   *  \return Vector of diagrams
   */
  TEDDY_DEF static auto from_pla (
    binary_manager &manager,
    const pla_file_binary &file
  ) -> std::vector<binary_manager::diagram_t>;

  /**
   *  \brief Creates MDDs defined by PLA file
   *
   *  \param manager Diagram manager
   *  \param file PLA file
   *  \return Diagram
   */
  template<class Degree, class Domain>
  static auto from_pla(
    diagram_manager<Degree, Domain> &manager,
    const pla_file_mvl &file
  ) -> diagram_manager<Degree, Domain>::diagram_t;

  /**
   *  \brief Creates diagram from a truth vector of a function
   *
   *  Example for the function f(x) = max(x0, x1, x2):
   *  \code
   *  // Truth table:
   *  +----+----+----+----++----+-----+----+---+
   *  | x1 | x2 | x3 | f  || x1 |  x2 | x3 | f |
   *  +----+----+----+----++----+-----+----+---+
   *  | 0  | 0  | 0  | 0  || 1  |  0  | 0  | 1 |
   *  | 0  | 0  | 1  | 1  || 1  |  0  | 1  | 1 |
   *  | 0  | 0  | 2  | 2  || 1  |  0  | 2  | 2 |
   *  | 0  | 1  | 0  | 1  || 1  |  1  | 0  | 1 |
   *  | 0  | 1  | 1  | 1  || 1  |  1  | 1  | 1 |
   *  | 0  | 1  | 2  | 2  || 1  |  1  | 2  | 2 |
   *  +----+----+----+----++----+-----+----+---+
   *  \endcode
   *
   *  \code
   *  // Truth vector:
   *  [0 1 2 1 1 2 1 1 2 1 1 2]
   *  \endcode
   *
   *  \code
   *  // Teddy code:
   *  teddy::ifmdd_manager<3> manager(3, 100, {2, 2, 3});
   *  std::vector<int> vec {0, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2};
   *  diagram_t f = manager.from_vector(vec);
   *  \endcode
   *
   *  \tparam I Iterator type for the input range
   *  \tparam S Sentinel type for \p I (end iterator)
   *  \param manager Diagram manager
   *  \param first Iterator to the first element of the truth vector
   *  \param last Sentinel for \p first (end iterator)
   *  \return Diagram representing function given by the truth vector
   */
  template<
    class Degree,
    class Domain,
    std::input_iterator I,
    std::sentinel_for<I> S>
  static auto from_vector (
    diagram_manager<Degree, Domain> &manager,
    I first,
    S last
  ) -> diagram_manager<Degree, Domain>::diagram_t;

  /**
   *  \brief Creates diagram from a truth vector of a function
   *
   *  See the other overload for details.
   *
   *  \tparam R Type of the range that contains the truth vector
   *  \param vector Range representing the truth vector
   *  Elements of the range must be convertible to int
   *  \return Diagram representing function given by the truth vector
   */
  template<class Degree, class Domain, std::ranges::input_range R>
  static auto from_vector (
    diagram_manager<Degree, Domain> &manager,
    R const &vector
  ) -> diagram_manager<Degree, Domain>::diagram_t;

  /**
   *  \brief Creates diagram from a truth vector of a function
   *
   *  See the other overload for details.
   *
   *  \param vector Range representing the truth vector
   *  Elements of the range must be convertible to int
   *  \return Diagram representing function given by the truth vector
   */
  template<class Degree, class Domain>
  static auto from_vector (
    diagram_manager<Degree, Domain> &manager,
    std::initializer_list<int> vector
  ) -> diagram_manager<Degree, Domain>::diagram_t;

  /**
   *  \brief Creates truth vector from the diagram
   *
   *  Significance of variables is the same as in the \c from_vector
   *  function i.e., variable on the last level of the diagram is
   *  least significant. The following assertion holds:
   *  \code
   *  assert(manager.from_vector(manager.to_vector(d)))
   *  \endcode
   *
   *  \param manager Diagram manager
   *  \param diagram Diagram
   *  \return Vector of ints representing the truth vector
   */
  template<class Degree, class Domain>
  static auto to_vector (
    diagram_manager<Degree, Domain> const &manager,
    diagram_manager<Degree, Domain>::diagram_t const &diagram
  ) -> std::vector<int32>;

  /**
   *  \brief Creates truth vector from the diagram
   *  \tparam O Output iterator type
   *  \param manager Diagram manager
   *  \param diagram Diagram
   *  \param out Output iterator that is used to output the truth vector
   */
  template<class Degree, class Domain, std::output_iterator<teddy::int32> O>
  static auto to_vector_g (
    diagram_manager<Degree, Domain> const &manager,
    diagram_manager<Degree, Domain>::diagram_t const &diagram,
    O out
  ) -> void;

  /**
   *  \brief Prints dot representation of the graph
   *
   *  Prints dot representation of the entire multi rooted graph to
   *  the output stream.
   *
   *  \param manager Diagram manager
   *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
   */
  template<class Degree, class Domain>
  static auto to_dot (
    diagram_manager<Degree, Domain> const &manager,
    std::ostream &out
  ) -> void;

  /**
   *  \brief Prints dot representation of the diagram
   *
   *  Prints dot representation of the diagram to
   *  the output stream.
   *
   *  \param manager Diagram manager
   *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
   *  \param diagram Diagram
   */
  template<class Degree, class Domain>
  static auto to_dot (
    diagram_manager<Degree, Domain> const &manager,
    std::ostream &out,
    diagram_manager<Degree, Domain>::diagram_t const &diagram
  ) -> void;
};

} // namespace teddy

// Include definitions in header-only mode
#ifndef TEDDY_NO_HEADER_ONLY
#include <libteddy/impl/io.cpp>
#endif

// Always include inline definitions
#include <libteddy/impl/io.inl>

#endif
