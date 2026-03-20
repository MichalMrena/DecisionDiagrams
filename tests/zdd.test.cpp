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

BOOST_AUTO_TEST_SUITE(zdd_tests)

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

BOOST_AUTO_TEST_CASE(all_zero_function)
{
  std::vector<int> v = {0,0,0,0};
  teddy::zdd_manager manager(2, 100, 100);

  auto* d = manager.from_vector(v);
  BOOST_REQUIRE(d != nullptr);

  BOOST_CHECK_EQUAL(manager.evaluate(teddy::zdd_manager::diagram_t(d), {0,0}), 0);
  BOOST_CHECK_EQUAL(manager.evaluate(teddy::zdd_manager::diagram_t(d), {0,1}), 0);
  BOOST_CHECK_EQUAL(manager.evaluate(teddy::zdd_manager::diagram_t(d), {1,0}), 0);
  BOOST_CHECK_EQUAL(manager.evaluate(teddy::zdd_manager::diagram_t(d), {1,1}), 0);
}

BOOST_AUTO_TEST_CASE(all_one_function)
{
  std::vector<int> v = {1,1,1,1};
  teddy::zdd_manager manager(2, 100, 100);

  auto* d = manager.from_vector(v);
  BOOST_REQUIRE(d != nullptr);

  BOOST_CHECK_EQUAL(manager.evaluate(teddy::zdd_manager::diagram_t(d), {0,0}), 1);
  BOOST_CHECK_EQUAL(manager.evaluate(teddy::zdd_manager::diagram_t(d), {0,1}), 1);
  BOOST_CHECK_EQUAL(manager.evaluate(teddy::zdd_manager::diagram_t(d), {1,0}), 1);
  BOOST_CHECK_EQUAL(manager.evaluate(teddy::zdd_manager::diagram_t(d), {1,1}), 1);
}

BOOST_DATA_TEST_CASE(from_vector_correctness, zdd_vectors, test_desc) { //NOLINT
  teddy::zdd_manager manager(test_desc.var_count_, 10'000, 100);
  auto* root = manager.from_vector(test_desc.vector_);

  for (const auto& [input, expected] : test_desc.tests_) {
    auto actual = manager.evaluate(teddy::zdd_manager::diagram_t(root), input);

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

BOOST_AUTO_TEST_SUITE_END()
