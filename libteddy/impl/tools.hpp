#ifndef LIBTEDDY_DETAILS_UTILS_HPP
#define LIBTEDDY_DETAILS_UTILS_HPP

#include <libteddy/impl/types.hpp>

#include <cctype>
#include <cstddef>
#include <string_view>
#include <vector>

/**
 * \file tools.hpp
 * \brief Contains (simplified) implementations of (STL) algorithms
 *
 * Tries to minimize dependencies.
 */

namespace teddy::tools {

/**
 *  \brief Exponentiation by squaring
 */
template<class Base>
auto constexpr int_pow(Base base, uint32 exponent) -> Base {
  Base result = 1;

  for (;;) {
    if (exponent & 1U) {
      result *= base;
    }

    exponent >>= 1U;

    if (0 == exponent) {
      break;
    }

    base *= base;
  }

  return result;
}

/**
 *  \brief Hash for pointers
 */
TEDDY_DEF_INL auto do_hash (void *const p) -> std::size_t {
  return reinterpret_cast<std::size_t>(p) >> 4U; // NOLINT
}

/**
 *  \brief Hash for int
 */
TEDDY_DEF_INL auto do_hash (int32 const x) -> std::size_t {
  return static_cast<std::size_t>(x);
}

/**
 *  \brief Hashes \p elem and combines the result with \p hash
 */
template<class T>
auto add_hash (std::size_t &hash, T const &elem) -> void {
  // see boost::hash_combine
  hash ^= do_hash(elem) + 0x9e3779b9 + (hash << 6U) + (hash >> 2U);
}

/**
 *  \brief Computes hash of the \p args
 */
template<class... Ts>
auto pack_hash (Ts const &...args) -> std::size_t {
  std::size_t result = 0;
  (add_hash(result, args), ...);
  return result;
}

/**
 *  \brief The min function
 */
template<class T>
auto constexpr min(T lhs, T rhs) -> T {
  return lhs < rhs ? lhs : rhs;
}

/**
 *  \brief The max function
 */
template<class T>
auto constexpr max(T lhs, T rhs) -> T {
  return lhs > rhs ? lhs : rhs;
}

/**
 *  \brief The min function for parameter packs
 */
template<class X>
auto constexpr pack_min(X x) -> X {
  return x;
}

/**
 *  \brief The min function for parameter packs
 */
template<class X, class... Xs>
auto constexpr pack_min(X x, Xs... xs) -> X {
  return min(x, pack_min(xs...));
}

/**
 *  \brief The max function for parameter packs
 */
template<class X>
auto constexpr pack_max(X x) -> X {
  return x;
}

/**
 *  \brief The max function for parameter packs
 */
template<class X, class... Xs>
auto constexpr pack_max(X x, Xs... xs) -> X {
  return max(x, pack_max(xs...));
}

/**
 *  \brief Maximum of a range
 *  Implementation of \c std::max_element
 */
template<class It>
auto constexpr max_elem(It first, It const last) -> It {
  It maxIt = first;
  while (first != last) {
    if (*first > *maxIt) {
      maxIt = first;
    }
    ++first;
  }
  return maxIt;
}

/**
  *  \brief Finds the first element satisfying \p test
  *  Implementation of \c std::find_if
  */
template<class It, class Predicate>
auto constexpr find_if(It first, It const last, Predicate test) -> It {
  while (first != last) {
    if (test(*first)) {
      return first;
    }
    ++first;
  }
  return last;
}

/**
  *  \brief Finds the first element not satisfying \p test
  *  Implementation of \c std::find_if_not
  */
template<class It, class Predicate>
auto constexpr find_if_not(It first, It const last, Predicate test) -> It {
  while (first != last) {
    if (not test(*first)) {
      return first;
    }
    ++first;
  }
  return last;
}

/**
 * \brief Trims string from both ends
 * \param str string to trim
 * \return trimmed view of \p str
 */
TEDDY_DEF auto trim(std::string_view str) -> std::string_view;

/**
 * \brief Splits \p str into words delimited by one of \p delimiters
 */
TEDDY_DEF auto to_words(
  std::string_view str,
  std::string_view delimiters
) -> std::vector<std::string_view>;

/**
 *  \brief Exchages value of \p var to \p newVal and returns the old value
 *  Simplified implementation of \c std::exchange
 */
template<class T, class U = T>
auto constexpr exchange(T &var, U newVal) noexcept -> T {
  auto oldVal = var;
  var         = newVal;
  return oldVal;
}

/**
 *  \brief Swaps values in \p first and \p second
 *  Simplified implementation of \c std::swap
 */
template<class T>
auto constexpr swap(T &first, T &second) noexcept -> void {
  auto tmp = first;
  first    = second;
  second   = tmp;
}

/**
 *  \brief Simple heapsort for vectors
 */
template<class T, class Compare>
auto sort (std::vector<T> &xs, Compare cmp) -> void {
  if (xs.empty()) {
    return;
  }

  auto const sift_down = [&xs, cmp] (uint32 parent, uint32 const size) {
    uint32 left  = 2 * parent + 1;
    uint32 right = left + 1;
    while (left < size) {
      uint32 swap = parent;
      if (cmp(xs[swap], xs[left])) {
        swap = left;
      }

      if (right < size && cmp(xs[swap], xs[right])) {
        swap = right;
      }

      if (swap == parent) {
        break;
      }

      tools::swap(xs[parent], xs[swap]);
      parent = swap;
      left   = 2 * parent + 1;
      right  = left + 1;
    }
  };

  auto const size = static_cast<uint32>(xs.size());

  // make-heap
  for (uint32 i = size / 2 + 1; i > 0;) {
    --i;
    sift_down(i, size);
  }

  // pop-heap
  for (uint32 last = size - 1; last > 0; --last) {
    tools::swap(xs[last], xs[0]);
    sift_down(0, last);
  }
}

/**
 *  \brief Checks if T and U are the same type
 */
template<class T, class U>
struct is_same {
  static bool constexpr value = false;
};

/**
 *  \brief Checks if T and U are the same type
 */
template<class T>
struct is_same<T, T> {
  static bool constexpr value = true;
};

/**
 *  \brief Checks if T and U are the same type
 */
template<class T, class U>
concept same_as = is_same<T, U>::value;

/**
 *  \brief Checks if T is \c std::vector
 */
template<class T>
concept is_std_vector
  = same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;

/**
 * \brief Simplified implementation of \c std::is_scalar
 */
template<class T>
concept is_scalar = same_as<int, T>;

/**
 *  \brief Provides member typedef based on the value of \p B
 *  Implementation of \c std::conditional
 */
template<bool B, class T, class F>
struct type_if;

/**
 *  \brief Specialization for B = true
 */
template<class T, class F>
struct type_if<true, T, F> {
  using type = T;
};

/**
 *  \brief Specialization for B = false
 */
template<class T, class F>
struct type_if<false, T, F> {
  using type = F;
};

/**
 *  \brief Helper for SFINE functions
 */
template<class X, class T>
using second_t = type_if<false, X, T>::type;

/**
 *  \brief Implementation of \c std::remove_reference
 */
template<class T>
struct remove_reference {
  using type = T;
};

/**
 *  \brief Specialization for lvalue ref
 */
template<class T>
struct remove_reference<T &> {
  using type = T;
};

/**
 *  \brief Specialization for rvalue ref
 */
template<class T>
struct remove_reference<T &&> {
  using type = T;
};

/**
 *  \brief List of types, provides basic queries
 */
template<class... Types>
struct type_list {
  template<class T>
  static bool constexpr Contains          = (is_same<T, Types>::value || ...);

  static std::size_t constexpr MaxSizeof  = tools::pack_max(sizeof(Types)...);

  static std::size_t constexpr MaxAlignof = tools::pack_max(alignof(Types)...);
};

/**
 *  \brief Implementation of \c std::move
 */
#define TEDDY_MOVE(...)                                                        \
  static_cast<::teddy::tools::remove_reference<decltype(__VA_ARGS__)>::type    \
                &&>(__VA_ARGS__)

/**
 *  \brief Implementation of \c std::forward
 */
#define TEDDY_FORWARD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

}  // namespace teddy::tools

// Include definitions in header-only mode
#ifndef TEDDY_NO_HEADER_ONLY
#include <libteddy/impl/tools.cpp>
#endif

// Always include inline definitions
#include <libteddy/impl/tools.inl>

#endif
