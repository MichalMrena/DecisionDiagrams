#define BOOST_TEST_MODULE ZDDTest
#include <boost/test/included/unit_test.hpp>

#include <libteddy/impl/zdd_manager.hpp>

BOOST_AUTO_TEST_SUITE(zdd_tests)

BOOST_AUTO_TEST_CASE(empty_vector) 
{
  std::vector<int> v = {};
  teddy::zdd_manager manager(20, 100, 100);
  auto* d = manager.from_vector(v);
  BOOST_CHECK(d == nullptr);
}

BOOST_AUTO_TEST_CASE(zdd_first_rule) 
{
  std::vector<int> v = {1, 0};
  teddy::zdd_manager manager(20, 100, 100);
  auto* d = manager.from_vector(v);
  BOOST_CHECK(d != nullptr);
  BOOST_CHECK(d->get_value() == 1);
}

BOOST_AUTO_TEST_SUITE_END()
