#define BOOST_TEST_MODULE ZDDTest
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <libteddy/impl/zdd_manager.hpp>

#include <ostream>

struct zdd_vector_test {
  int var_count_;
  std::vector<int> vector_;
  std::vector<std::pair<std::vector<int>, int>> tests_;
};

const std::array zdd_vectors = {
  zdd_vector_test{
    .var_count_ = 3,
    .vector_ = {0,0,0,1,1,0,0,1},
    .tests_ = {
      {{0,0,0},0},
      {{0,0,1},0},
      {{0,1,0},0},
      {{0,1,1},1},
      {{1,0,0},1},
      {{1,0,1},0},
      {{1,1,0},0},
      {{1,1,1},1}
    }
  },

  zdd_vector_test{
    .var_count_ = 2,
    .vector_ = {0,1,1,0},
    .tests_ = {
      {{0,0},0},
      {{0,1},1},
      {{1,0},1},
      {{1,1},0}
    }
  }
};

auto operator<<(std::ostream& os, const zdd_vector_test& t) -> std::ostream&
{
    os << "zdd_vector_test(var_count=" << t.var_count_ << ")";
    return os;
}

BOOST_AUTO_TEST_SUITE(zdd_tests)

BOOST_AUTO_TEST_CASE(empty_vector) {
  std::vector<int> v = {};
  teddy::zdd_manager manager(3, 100, 100);
  auto* d = manager.from_vector(v);
  BOOST_CHECK(d == nullptr);
}

BOOST_AUTO_TEST_CASE(zdd_first_rule) {
  std::vector<int> v = {1, 0};
  teddy::zdd_manager manager(3, 100, 100);
  auto* d = manager.from_vector(v);
  BOOST_CHECK(d != nullptr);
  BOOST_CHECK(d->get_value() == 1);
}

BOOST_DATA_TEST_CASE(from_vector_correctness, zdd_vectors, test_desc) { // NOLINT
  teddy::zdd_manager manager(test_desc.var_count_, 10'000, 100);
  auto* root = manager.from_vector(test_desc.vector_);

  for (const auto& [input, expected] : test_desc.tests_) {
    BOOST_REQUIRE_EQUAL( manager.evaluate(teddy::zdd_manager::diagram_t(root), input), expected );
  } 
}

BOOST_AUTO_TEST_SUITE_END()
