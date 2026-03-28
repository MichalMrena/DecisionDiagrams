#define BOOST_TEST_MODULE ZDDTest
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <libteddy/impl/zdd_manager.hpp>

#include <ostream>

#include <boost/test/tools/detail/print_helper.hpp>

namespace boost::test_tools::tt_detail {

template<>
struct print_log_value<std::vector<int>> {
    void operator()(std::ostream& os, const std::vector<int>& v) {
        os << "[";
        for (size_t i = 0; i < v.size(); ++i) {
            os << v[i];
            if (i + 1 != v.size()) {
              os << ",";
            }
        }
        os << "]";
    }
};

} // namespace boost::test_tools::tt_detail

struct zdd_vector_test {
  int var_count_;
  std::vector<int> vector_;
  std::vector<std::pair<std::vector<int>, int>> tests_;
};

const std::array zdd_vectors = { //NOLINT
  zdd_vector_test{
    .var_count_ = 3,
    .vector_ = {0,0,0,1,1,0,0,1},
    .tests_ = {
      {{0,1,1},1},
      {{1,0,0},1},
      {{1,1,1},1}
    }
  },

  zdd_vector_test{
    .var_count_ = 2,
    .vector_ = {0,1,1,0},
    .tests_ = {
      {{0,1},1},
      {{1,0},1}
    }
  },

  zdd_vector_test{
  .var_count_ = 4, // 2^4 = 16
  .vector_ = { 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1 },
  .tests_ = {
    {{0,0,0,1},1}, // 1
    {{0,0,1,1},1}, // 3
    {{0,1,0,0},1}, // 4
    {{0,1,1,0},1}, // 6
    {{1,0,0,0},1}, // 8
    {{1,0,1,0},1}, // 10
    {{1,1,0,1},1}, // 13
    {{1,1,1,1},1} // 15
  }
},

  zdd_vector_test{
    .var_count_ = 5,
    .vector_ = []{
      std::vector<int> v(32, 0);
      v[3]  = 1;  // 00011
      v[5]  = 1;  // 00101
      v[6]  = 1;  // 00110
      v[9]  = 1;  // 01001
      v[10] = 1;  // 01010
      v[12] = 1;  // 01100
      v[17] = 1;  // 10001
      v[21] = 1;  // 10101
      v[25] = 1;  // 11001
      v[31] = 1;  // 11111
      return v;
    }(),
    .tests_ = {
      {{0,0,0,1,1},1}, // 3
      {{0,0,1,0,1},1}, // 5
      {{0,0,1,1,0},1}, // 6
      {{0,1,0,0,1},1}, // 9
      {{0,1,0,1,0},1}, // 10
      {{0,1,1,0,0},1}, // 12
      {{1,0,0,0,1},1}, // 17
      {{1,0,1,0,1},1}, // 21
      {{1,1,0,0,1},1}, // 25
      {{1,1,1,1,1},1}  // 31
    }
  }
};

auto operator<<(std::ostream& os, const zdd_vector_test& t) -> std::ostream& {
    os << "Var count: " << t.var_count_ << "\n";
    return os;
}

auto index_to_input(uint32_t i, int vars) -> std::vector<int> {
    std::vector<int> input(vars);
    for (uint32_t j = 0; j < vars; ++j) {
        input[vars - j - 1] = static_cast<int>((i >> j) & 1U);
    }
    return input;
}

BOOST_AUTO_TEST_SUITE(zdd_creation_tests)

BOOST_AUTO_TEST_CASE(input_vector_tests) {
  std::vector<int> v = {};
  teddy::zdd_manager manager(3, 100, 100);

  auto* d = manager.from_vector(v);
  BOOST_CHECK(d == nullptr);

  v = {0};
  d = manager.from_vector(v);
  BOOST_CHECK(d != nullptr);

  v = {0,1,1};
  d = manager.from_vector(v);
  BOOST_CHECK(d == nullptr);

  v = {0,1,1,1};
  d = manager.from_vector(v);
  BOOST_CHECK(d != nullptr);
}

BOOST_AUTO_TEST_CASE(zdd_zero_suppression) {
  std::vector<int> v = {1, 0};
  teddy::zdd_manager manager(1, 100, 100);

  auto* d = manager.from_vector(v);
  BOOST_REQUIRE(d != nullptr);

  BOOST_CHECK(d->is_terminal());
  BOOST_CHECK_EQUAL(d->get_value(), 1);
}

BOOST_DATA_TEST_CASE(from_vector_correctness, zdd_vectors, test_desc) { //NOLINT
  teddy::zdd_manager manager(test_desc.var_count_, 10'000, 100);
  auto* root = manager.from_vector(test_desc.vector_);

  for (const auto& [input, expected] : test_desc.tests_) {
    auto actual = manager.evaluate(root, input);

    BOOST_TEST_CONTEXT(
        "vector=" << test_desc.vector_
        << " input=" << input
        << " expected=" << expected
        << " actual=" << actual
    ) {
        BOOST_REQUIRE_EQUAL(actual, expected);
    }
  }
}

// v = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v subset1 x1 = {
//   {x2},
//   {x0, x2}
// }
// v subset0 x1 = {
//   {x0}
// }
BOOST_AUTO_TEST_CASE(subset_basic) {
  std::vector<int> v = {0,0,0,1,1,0,0,1};
  teddy::zdd_manager manager(3, 1000, 100);

  auto* root = manager.from_vector(v);
  BOOST_REQUIRE(root != nullptr);

  auto* s1 = manager.subset1(root, 1);

  BOOST_CHECK_EQUAL(manager.evaluate(s1, {0,0,0}), 0);
  BOOST_CHECK_EQUAL(manager.evaluate(s1, {0,1,0}), 0);
  BOOST_CHECK_EQUAL(manager.evaluate(s1, {0,0,1}), 1);
  BOOST_CHECK_EQUAL(manager.evaluate(s1, {1,0,1}), 1);

  auto* s0 = manager.subset0(root, 1);

  BOOST_CHECK_EQUAL(manager.evaluate(s0, {0,0,0}), 0);
  BOOST_CHECK_EQUAL(manager.evaluate(s0, {1,0,0}), 1);
  BOOST_CHECK_EQUAL(manager.evaluate(s0, {1,1,0}), 1);
  BOOST_CHECK_EQUAL(manager.evaluate(s0, {0,1,0}), 0);
}

// v = {
//   {x1},
//   {x0}
// }
// v subset1 x5 = {
//   {}
// }
// v subset0 x5 = {
//   {x1},
//   {x0}
// }
BOOST_AUTO_TEST_CASE(subset_variable_not_present) {
  std::vector<int> v = {0,1,1,0};
  teddy::zdd_manager manager(2, 1000, 100);

  auto* root = manager.from_vector(v);
  BOOST_REQUIRE(root != nullptr);

  auto* s1 = manager.subset1(root, 5);
  auto* s0 = manager.subset0(root, 5);

  BOOST_CHECK_EQUAL(manager.evaluate(s1, {0,0}), 0);
  BOOST_CHECK_EQUAL(manager.evaluate(s1, {1,1}), 0);

  BOOST_CHECK_EQUAL(manager.evaluate(s0, {0,1}), 1);
  BOOST_CHECK_EQUAL(manager.evaluate(s0, {1,0}), 1);
}

// v = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v subset1 x1 = {
//   {x2},
//   {x0, x2}
// }
// v subset0 x1 = {
//   {x0}
// }
BOOST_AUTO_TEST_CASE(subset_partition_property) {
  std::vector<int> v = {0,0,0,1,1,0,0,1};
  teddy::zdd_manager manager(3, 1000, 100);

  auto* root = manager.from_vector(v);
  BOOST_REQUIRE(root != nullptr);

  auto* s1 = manager.subset1(root, 1);
  auto* s0 = manager.subset0(root, 1);

  for (uint32_t i = 0; i < 8; ++i) {
    std::vector<int> input = {
      static_cast<int>((i >> 2U) & 1U),
      static_cast<int>((i >> 1U) & 1U),
      static_cast<int>(i & 1U)
    };

    int original = manager.evaluate(root, input);
    int val0 = manager.evaluate(s0, input);
    int val1 = manager.evaluate(s1, input);

    if (input[1] == 0) {
      BOOST_CHECK_EQUAL(original, val0);
    } else {
      BOOST_CHECK_EQUAL(original, val1);
    }
  }
}

// v = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v change x2 = {
//   {x1},
//   {x0, x2},
//   {x0, x1}
// }
BOOST_AUTO_TEST_CASE(change_basic) {
  std::vector<int> v = {0,0,0,1,1,0,0,1};
  teddy::zdd_manager manager(3, 1000, 100);

  auto* root = manager.from_vector(v);
  BOOST_REQUIRE(root != nullptr);

  auto* changed = manager.change(root, 2);

  BOOST_CHECK_EQUAL(manager.evaluate(changed, {0,1,0}), 1); // {x1}
  BOOST_CHECK_EQUAL(manager.evaluate(changed, {1,0,1}), 1); // {x0,x2}
  BOOST_CHECK_EQUAL(manager.evaluate(changed, {1,1,0}), 1); // {x0,x1}
}

// v = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v change x1 = {
//   {x2},
//   {x0, x1},
//   {x0, x2}
// }
// c1 change x1 = {
//   {x1,x2},
//   {x0},
//   {x0, x1, x2}
// }
BOOST_AUTO_TEST_CASE(change_involution) {
  std::vector<int> v = {0,0,0,1,1,0,0,1};
  teddy::zdd_manager manager(3, 1000, 100);

  auto* root = manager.from_vector(v);
  BOOST_REQUIRE(root != nullptr);

  auto* c1 = manager.change(root, 1);
  auto* c2 = manager.change(c1, 1);

  for (uint32_t i = 0; i < 8; ++i) {
    std::vector<int> input = {
      static_cast<int>((i >> 2U) & 1U),
      static_cast<int>((i >> 1U) & 1U),
      static_cast<int>(i & 1U)
    };

    int original = manager.evaluate(root, input);
    int result   = manager.evaluate(c2, input);

    BOOST_CHECK_EQUAL(original, result);
  }
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   {x2},
//   {x1}
// }
// v1 union v2 = {
//   {x2},
//   {x1},
//   {x1,x2},
//   {x0},
//   {x0,x1,x2}
// }
BOOST_AUTO_TEST_CASE(union_basic) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2 = {0,1,1,0,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);
  auto* u  = manager.unification(d1, d2);

  for (uint32_t i = 0; i < 8; ++i) {
    if (v1[i] == 1 || v2[i] == 1) {
      auto input = index_to_input(i, 3);
      BOOST_CHECK_EQUAL(manager.evaluate(u, input), 1);
    }
  }
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
// }
// v1 union v2 && v2 union v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
BOOST_AUTO_TEST_CASE(union_with_zero) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2(8, 0);

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* u1 = manager.unification(d1, d2);
  auto* u2 = manager.unification(d2, d1);

  for (int i = 0; i < 8; ++i) {
    if (v1[i] == 1) {
      auto input = index_to_input(i, 3);
      BOOST_CHECK_EQUAL(manager.evaluate(u1, input), 1);
      BOOST_CHECK_EQUAL(manager.evaluate(u2, input), 1);
    }
  }
}
// v = {
//   {x2},
//   {x1}
// }
// v union v = {
//   {x2},
//   {x1}
// }
BOOST_AUTO_TEST_CASE(union_idempotent) {
  std::vector<int> v = {0,1,1,0,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d = manager.from_vector(v);
  auto* u = manager.unification(d, d);

  for (uint32_t i = 0; i < 8; ++i) {
    if (v[i] == 1) {
      auto input = index_to_input(i, 3);
      BOOST_CHECK_EQUAL(manager.evaluate(u, input), 1);
    }
  }
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   {x2},
//   {x1}
// }
// v1 union v2 && v2 union v1 = {
//   {x2},
//   {x1},
//   {x1,x2},
//   {x0},
//   {x0,x1,x2}
// }
BOOST_AUTO_TEST_CASE(union_commutative) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2 = {0,1,1,0,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* u1 = manager.unification(d1, d2);
  auto* u2 = manager.unification(d2, d1);

  for (uint32_t i = 0; i < 8; ++i) {
    if (v1[i] == 1 || v2[i] == 1) {
      auto input = index_to_input(i, 3);
      BOOST_CHECK_EQUAL(manager.evaluate(u1, input), 1);
      BOOST_CHECK_EQUAL(manager.evaluate(u2, input), 1);
    }
  }
}

// a = {
//   {x2},
//   {x1, x2}
// }
// b = {
//   {x1}
// }
// c = {
//   {x0}
// }
// (a union b) union c && a union (b union c) = {
//   {x2},
//   {x1,x2},
//   {x1},
//   {x0}
// }
BOOST_AUTO_TEST_CASE(union_associative) {
  std::vector<int> v1 = {0,1,0,1,0,0,0,0};
  std::vector<int> v2 = {0,0,1,0,0,0,0,0};
  std::vector<int> v3 = {0,0,0,0,1,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);
  auto* d3 = manager.from_vector(v3);

  auto* u1  = manager.unification(manager.unification(d1,d2), d3);
  auto* u2 = manager.unification(d1, manager.unification(d2,d3));

  for (uint32_t i = 0; i < 8; ++i) {
    if (v1[i] == 1 || v2[i] == 1 || v3[i] == 1) {
      auto input = index_to_input(i, 3);
      BOOST_CHECK_EQUAL(manager.evaluate(u1, input), 1);
      BOOST_CHECK_EQUAL(manager.evaluate(u2, input), 1);
    }
  }
}

// v1 = {
//   0
// }
// v2 = {
//   {x1, x2}
// }
// v1 union v2 = {
//   0,
//   {x1,x2}
// }
BOOST_AUTO_TEST_CASE(union_with_one_terminal) {
  std::vector<int> v1 = {1,0,0,0,0,0,0,0};
  std::vector<int> v2   = {0,0,0,1,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* u = manager.unification(d1, d2);

  for (uint32_t i = 0; i < 8; ++i) {
    if (v1[i] == 1 || v2[i] == 1) {
      auto input = index_to_input(i, 3);
      BOOST_CHECK_EQUAL(manager.evaluate(u, input), 1);
    }
  }
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   {x2},
//   {x1, x2},
//   {x0, x1, x2}
// }
// v1 intersect v2 = {
//   {x1,x2},
//   {x0,x1,x2}
// }
BOOST_AUTO_TEST_CASE(intersect_basic) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2 = {0,1,0,1,0,0,0,1};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* i = manager.intersect(d1, d2);

  std::vector<int> expected_idx = {3, 7};

  for (int idx : expected_idx) {
    auto input = index_to_input(idx, 3);
    BOOST_CHECK_EQUAL(manager.evaluate(i, input), 1);
  }
}

// a = {
//   {x2}
// }
// b = {
//   {x1}
// }
// a intersect b = {
// }
BOOST_AUTO_TEST_CASE(intersect_disjoint) {
  std::vector<int> v1 = {0,1,0,0,0,0,0,0};
  std::vector<int> v2 = {0,0,1,0,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* I = manager.intersect(d1, d2);

  for (uint32_t i = 0; i < 8; ++i) {
    auto input = index_to_input(i, 3);
    BOOST_CHECK_EQUAL(manager.evaluate(I, input), 0);
  }
}

// v = {
//   {x2},
//   {x1, x2}
// }
// v intersect v = {
//   {x2},
//   {x1, x2}
// }
BOOST_AUTO_TEST_CASE(intersect_idempotent) {
  std::vector<int> v = {0,1,0,1,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d = manager.from_vector(v);
  auto* i = manager.intersect(d, d);

  for (uint32_t idx = 0; idx < 8; ++idx) {
    if (v[idx] == 1) {
      auto input = index_to_input(idx, 3);
      BOOST_CHECK_EQUAL(manager.evaluate(i, input), 1);
    }
  }
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   {x2},
//   {x1, x2},
//   {x0, x1, x2}
// }
// v1 intersect v2 && v2 intersect v1 = {
//   {x1,x2},
//   {x0,x1,x2}
// }
BOOST_AUTO_TEST_CASE(intersect_commutative) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2 = {0,1,0,1,0,0,0,1};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* i1 = manager.intersect(d1, d2);
  auto* i2 = manager.intersect(d2, d1);

  for (int i = 0; i < 8; ++i) {
    auto input = index_to_input(i, 3);

    BOOST_CHECK_EQUAL(
      manager.evaluate(i1, input),
      manager.evaluate(i2, input)
    );
  }
}

// v1 = {
//   {x2},
//   {x1, x2}
// }
// v2 = {
//   {x1},
//   {x1, x2}
// }
// v3 = {
//   {x1, x2}
// }
// (v1 intersect v2) intersect v3 && v1 intersect (v2 intersect v3) = {
//   {x1,x2}
// }
BOOST_AUTO_TEST_CASE(intersect_associative) {
  std::vector<int> v1 = {0,1,0,1,0,0,0,0};
  std::vector<int> v2 = {0,0,1,1,0,0,0,0};
  std::vector<int> v3 = {0,0,0,1,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);
  auto* d3 = manager.from_vector(v3);

  auto* i1  = manager.intersect(manager.intersect(d1,d2), d3);
  auto* i2 = manager.intersect(d1, manager.intersect(d2,d3));

  for (int i = 0; i < 8; ++i) {
    auto input = index_to_input(i, 3);

    BOOST_CHECK_EQUAL(
      manager.evaluate(i1, input),
      manager.evaluate(i2, input)
    );
  }
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//
// }
// v1 intersect v2 && v2 intersect v1 = {
//
// }
BOOST_AUTO_TEST_CASE(intersect_with_zero) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2(8, 0);

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* i1 = manager.intersect(d1, d2);
  auto* i2 = manager.intersect(d2, d1);

  for (int i = 0; i < 8; ++i) {
    auto input = index_to_input(i, 3);

    BOOST_CHECK_EQUAL(manager.evaluate(i1, input), 0);
    BOOST_CHECK_EQUAL(manager.evaluate(i2, input), 0);
  }
}

// v1 = {
//   0
// }
// v2 = {
//  {x1, x2}
// }
// v1 intersect v2 = {
//
// }
BOOST_AUTO_TEST_CASE(intersect_with_one_terminal) {
  std::vector<int> v1 = {1,0,0,0,0,0,0,0};
  std::vector<int> v2   = {0,0,0,1,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* i = manager.intersect(d1, d2);

  BOOST_CHECK_EQUAL(manager.count(i), 0);
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   {x2},
//   {x1, x2},
//   {x0, x1, x2}
// }
// v1 diff v2 = {
//   {x0}
// }
BOOST_AUTO_TEST_CASE(difference_basic) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2 = {0,1,0,1,0,0,0,1};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* diff = manager.difference(d1, d2);

  std::vector<int> expected_idx = {4};

  for (int idx : expected_idx) {
    auto input = index_to_input(idx, 3);
    BOOST_CHECK_EQUAL(manager.evaluate(diff, input), 1);
  }
}

// v1 = {
//   {x2},
// }
// v2 = {
//   {x1}
// }
// v1 diff v2 = {
//   {x2}
// }
BOOST_AUTO_TEST_CASE(difference_disjoint) {
  std::vector<int> v1 = {0,1,0,0,0,0,0,0};
  std::vector<int> v2 = {0,0,1,0,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* diff = manager.difference(d1, d2);

  auto input = index_to_input(1, 3);
  BOOST_CHECK_EQUAL(manager.evaluate(diff, input), 1);
}

// v = {
//   {x2},
//   {x1, x2}
// }
// v diff v = {
//
// }
BOOST_AUTO_TEST_CASE(difference_idempotent) {
  std::vector<int> v = {0,1,0,1,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d = manager.from_vector(v);
  auto* diff = manager.difference(d, d);

  for (int i = 0; i < 8; ++i) {
    auto input = index_to_input(i, 3);
    BOOST_CHECK_EQUAL(manager.evaluate(diff, input), 0);
  }
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   
// }
// v1 diff v2 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
BOOST_AUTO_TEST_CASE(difference_with_zero) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2(8, 0);

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* diff = manager.difference(d1, d2);

  for (int i = 0; i < 8; ++i) {
    auto input = index_to_input(i, 3);

    if (v1[i] == 1) {
      BOOST_CHECK_EQUAL(manager.evaluate(diff, input), 1);
    }
  }
}

// v1 = {
//   {x2},
//   {x1, x2}
// }
// v2 = {
//   {x2},
//   {x1, x2}
// }
// v1 diff v2 = {
//
// }
BOOST_AUTO_TEST_CASE(difference_remove_all) {
  std::vector<int> v1 = {0,1,0,1,0,0,0,0};
  std::vector<int> v2 = {0,1,0,1,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* diff = manager.difference(d1, d2);

  for (int i = 0; i < 8; ++i) {
    auto input = index_to_input(i, 3);
    BOOST_CHECK_EQUAL(manager.evaluate(diff, input), 0);
  }
}

// v1 = {
//   {x2},
//   {x1},
//   {x1, x2}
// }
// v2 = {
//   {x1}
// }
// v1 diff v2 = {
//   {x2},
//   {x1, x2}
// }
BOOST_AUTO_TEST_CASE(difference_partial_overlap) {
  std::vector<int> v1 = {0,1,1,1,0,0,0,0};
  std::vector<int> v2 = {0,0,1,0,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* diff = manager.difference(d1, d2);

  std::vector<int> expected_idx = {1,3};

  for (int idx : expected_idx) {
    auto input = index_to_input(idx, 3);
    BOOST_CHECK_EQUAL(manager.evaluate(diff, input), 1);
  }
}

// v1 = {
//   {x2},
//   {x1, x2}
// }
// v2 = {
//   {x1, x2}
// }
// v1 diff v2 = {
//   {x2}
// }
// v2 diff v1 = {
//   
// }
BOOST_AUTO_TEST_CASE(difference_not_commutative) {
  std::vector<int> v1 = {0,1,0,1,0,0,0,0};
  std::vector<int> v2 = {0,0,0,1,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* diag1 = manager.from_vector(v1);
  auto* diag2 = manager.from_vector(v2);

  auto* diff1 = manager.difference(diag1, diag2);
  auto* diff2 = manager.difference(diag2, diag1);

  BOOST_CHECK_EQUAL(manager.evaluate(diff1, index_to_input(1,3)), 1);

  for (int i = 0; i < 8; ++i) {
    auto input = index_to_input(i, 3);
    BOOST_CHECK_EQUAL(manager.evaluate(diff2, input), 0);
  }
}

// v = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// count(v) = 3
BOOST_AUTO_TEST_CASE(count_basic) {
  std::vector<int> v = {0,0,0,1,1,0,0,1};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d = manager.from_vector(v);

  BOOST_REQUIRE(d != nullptr);
  BOOST_CHECK_EQUAL(manager.count(d), 3);
}

// v = {
//
// }
// count(v) = 0
BOOST_AUTO_TEST_CASE(count_zero) {
  std::vector<int> v(8, 0);

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d = manager.from_vector(v);

  BOOST_REQUIRE(d != nullptr);
  BOOST_CHECK_EQUAL(manager.count(d), 0);
}

// v = {
//   {x0}
// }
// count(v) = 1
BOOST_AUTO_TEST_CASE(count_single_set) {
  std::vector<int> v = {0,0,0,0,1,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d = manager.from_vector(v);

  BOOST_CHECK_EQUAL(manager.count(d), 1);
}

// v = {
//   0,
//   {x2},
//   {x1},
//   {x1, x2},
//   {x0},
//   {x0, x2},
//   {x0, x1},
//   {x0, x1, x2}
// }
// count(v) = 8
BOOST_AUTO_TEST_CASE(count_all_sets) {
  std::vector<int> v(8, 1);

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d = manager.from_vector(v);

  BOOST_CHECK_EQUAL(manager.count(d), 8);
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   {x2},
//   {x1, x2}
// }
// v1 union v2 = {
//   {x2},
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// count(v1 union v2) = 4
BOOST_AUTO_TEST_CASE(count_after_union) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2 = {0,1,0,1,0,0,0,0};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* result = manager.unification(d1, d2);

  BOOST_CHECK_EQUAL(manager.count(result), 4);
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   {x2},
//   {x1, x2},
//   {x0, x1, x2}
// }
// v1 intersection v2 = {
//   {x1, x2},
//   {x0, x1, x2}
// count(v1 intersection v2) = 2
BOOST_AUTO_TEST_CASE(count_after_intersection) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2 = {0,1,0,1,0,0,0,1};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* result = manager.intersect(d1, d2);

  BOOST_CHECK_EQUAL(manager.count(result), 2);
}

// v1 = {
//   {x1, x2},
//   {x0},
//   {x0, x1, x2}
// }
// v2 = {
//   {x2},
//   {x1, x2},
//   {x0, x1, x2}
// }
// v1 diff v2 = {
//   {x0}
// }
// count(v1 diff v2) = 1
BOOST_AUTO_TEST_CASE(count_after_difference) {
  std::vector<int> v1 = {0,0,0,1,1,0,0,1};
  std::vector<int> v2 = {0,1,0,1,0,0,0,1};

  teddy::zdd_manager manager(3, 1000, 100);

  auto* d1 = manager.from_vector(v1);
  auto* d2 = manager.from_vector(v2);

  auto* result = manager.difference(d1, d2);

  BOOST_CHECK_EQUAL(manager.count(result), 1);
}

BOOST_AUTO_TEST_SUITE_END()
