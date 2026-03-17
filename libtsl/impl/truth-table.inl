#include <libtsl/inc/truth-table.hpp>

#include <teddy/impl/types.hpp>

#include <functional>

namespace teddy::tsl {

/**
 *  \brief Invokes \p f with each element of the domain.
 */
template<class F>
auto domain_for_each (
  int const varcount,
  std::vector<int> const &vector,
  std::vector<int> const &domains,
  F f
) -> void {
  auto element = std::vector<int>(as_usize(varcount), 0);
  auto wasLast = false;
  auto k       = 0;
  do {
    // Invoke f.
    std::invoke(f, vector[as_uindex(k)], element);

    // Move to the next element of the domain.
    auto overflow = true;
    auto i        = varcount;
    while (i > 0 && overflow) {
      --i;
      ++element[as_uindex(i)];
      overflow = element[as_uindex(i)] == domains[as_uindex(i)];
      if (overflow) {
        element[as_uindex(i)] = 0;
      }
    }

    wasLast = overflow && i == 0;
    ++k;
  } while (not wasLast);
}

/**
 *  \brief Invokes \p f with each element of the domain.
 */
template<class F>
auto domain_for_each (truth_table const &table, F f) -> void {
  domain_for_each(
    table.get_var_count(),
    table.get_vector(),
    table.get_domains(),
    f
  );
}

template<class Op>
auto apply (truth_table const &lhs, truth_table const &rhs, Op operation)
  -> truth_table {
  assert(ssize(lhs.vector_) == ssize(rhs.vector_));
  assert(lhs.domain_ == rhs.domain_);

  auto result = std::vector<int>();
  for (auto i = 0; i < ssize(rhs.vector_); ++i) {
    lhs.vector_[as_uindex(i)]
      = operation(lhs.vector_[as_uindex(i)], rhs.vector_[as_uindex(i)]);
  }
  return {result, lhs.get_domains()};
}

template<class Op>
auto apply_mutable (truth_table &lhs, truth_table const &rhs, Op operation)
  -> void {
  assert(ssize(lhs.vector_) == ssize(rhs.vector_));
  assert(lhs.domain_ == rhs.domain_);

  for (auto i = 0; i < ssize(rhs.vector_); ++i) {
    lhs.vector_[as_uindex(i)]
      = operation(lhs.vector_[as_uindex(i)], rhs.vector_[as_uindex(i)]);
  }
}

/**
 *  \brief Element-wise comparison of vectors using \p cmp
 *  \param lhs first vector
 *  \param rhs first vector
 *  \param op comparison operation, e.g. <
 *  \return true iff each element of \p lsh is \p cmp than \p rhs
 */
template<class T, class Cmp>
auto compare (std::vector<T> const &lhs, std::vector<T> const &rhs, Cmp cmp)
  -> bool {
  assert(ssize(lhs) == ssize(rhs));
  auto lhsIt  = begin(lhs);
  auto lhsEnd = end(lhs);
  auto rhsIt  = begin(rhs);
  while (lhsIt != lhsEnd) {
    if (not cmp(*lhsIt, *rhsIt)) {
      return false;
    }
    ++lhsIt;
    ++rhsIt;
  }
  return true;
}

}  // namespace teddy::tsl
