#include <libteddy/impl/node_manager.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <vector>

namespace teddy::test
{
struct bdd_nodes_fixture
{
  using manager_t = node_manager<degrees::fixed<2>, domains::fixed<2>>;
  using node_t    = manager_t::node_t;
  int32 varCount_ = 10;
  std::vector<int32> order_   = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<int32> domains_ = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
  int64 nodePoolSize_         = 10'000;
  int64 extraNodePoolSize_    = 2'000;
};

struct mdd_nodes_fixture
{
  using manager_t = node_manager<degrees::fixed<3>, domains::fixed<3>>;
  using node_t    = manager_t::node_t;
  int32 varCount_ = 10;
  std::vector<int32> order_   = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<int32> domains_ = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
  int64 nodePoolSize_         = 10'000;
  int64 extraNodePoolSize_    = 2'000;
};

struct imdd_nodes_fixture
{
  using manager_t             = node_manager<degrees::mixed, domains::mixed>;
  using node_t                = manager_t::node_t;
  int32 varCount_             = 10;
  std::vector<int32> order_   = {8, 4, 5, 3, 6, 7, 9, 0, 1, 2};
  std::vector<int32> domains_ = {2, 3, 4, 3, 3, 3, 4, 4, 2, 2};
  int64 nodePoolSize_         = 10'000;
  int64 extraNodePoolSize_    = 2'000;
};

struct ifmdd_nodes_fixture
{
  using manager_t             = node_manager<degrees::fixed<4>, domains::mixed>;
  using node_t                = manager_t::node_t;
  int32 varCount_             = 10;
  std::vector<int32> order_   = {8, 4, 5, 3, 6, 7, 9, 0, 1, 2};
  std::vector<int32> domains_ = {2, 3, 4, 3, 3, 3, 4, 4, 2, 2};
  int64 nodePoolSize_         = 10'000;
  int64 extraNodePoolSize_    = 2'000;
};

auto make_manager (bdd_nodes_fixture const& fix)
{
  return bdd_nodes_fixture::manager_t(
    fix.varCount_,
    fix.nodePoolSize_,
    fix.extraNodePoolSize_,
    fix.order_
  );
}

auto make_manager (mdd_nodes_fixture const& fix)
{
  return mdd_nodes_fixture::manager_t(
    fix.varCount_,
    fix.nodePoolSize_,
    fix.extraNodePoolSize_,
    fix.order_
  );
}

auto make_manager (imdd_nodes_fixture const& fix)
{
  return imdd_nodes_fixture::manager_t(
    fix.varCount_,
    fix.nodePoolSize_,
    fix.extraNodePoolSize_,
    fix.order_,
    domains::mixed(fix.domains_)
  );
}

auto make_manager (ifmdd_nodes_fixture const& fix)
{
  return ifmdd_nodes_fixture::manager_t(
    fix.varCount_,
    fix.nodePoolSize_,
    fix.extraNodePoolSize_,
    fix.order_,
    domains::mixed(fix.domains_)
  );
}

using nodes_fixtures = boost::mpl::vector<
  bdd_nodes_fixture,
  mdd_nodes_fixture,
  imdd_nodes_fixture,
  ifmdd_nodes_fixture>;

BOOST_AUTO_TEST_SUITE(node)

BOOST_FIXTURE_TEST_CASE(terminal_node, bdd_nodes_fixture)
{
  manager_t manager = make_manager(*this);
  node_t* zero      = manager.make_terminal_node(0);
  node_t* one       = manager.make_terminal_node(1);
  BOOST_REQUIRE(zero->is_used());
  BOOST_REQUIRE(one->is_used());
  BOOST_REQUIRE(zero->is_terminal());
  BOOST_REQUIRE(one->is_terminal());
  BOOST_REQUIRE(not zero->is_internal());
  BOOST_REQUIRE(not one->is_internal());
  BOOST_REQUIRE_EQUAL(0, zero->get_value());
  BOOST_REQUIRE_EQUAL(1, one->get_value());
  BOOST_REQUIRE_EQUAL(0, zero->get_ref_count());
  BOOST_REQUIRE_EQUAL(0, one->get_ref_count());
  BOOST_REQUIRE_EQUAL(nullptr, zero->get_next());
  BOOST_REQUIRE_EQUAL(nullptr, one->get_next());
}

BOOST_FIXTURE_TEST_CASE(internal_node, bdd_nodes_fixture)
{
  using sons_t      = node_t::son_container;
  manager_t manager = make_manager(*this);
  node_t* zero      = manager.make_terminal_node(0);
  node_t* one       = manager.make_terminal_node(1);
  sons_t x1sons     = node_t::make_son_container(2);
  x1sons[0]         = zero;
  x1sons[1]         = one;
  node_t* x1        = manager.make_internal_node(1, TEDDY_MOVE(x1sons));
  BOOST_REQUIRE(x1->is_used());
  BOOST_REQUIRE(x1->is_internal());
  BOOST_REQUIRE(not x1->is_terminal());
  BOOST_REQUIRE_EQUAL(1, x1->get_index());
  BOOST_REQUIRE_EQUAL(0, x1->get_ref_count());
  BOOST_REQUIRE_EQUAL(1, zero->get_ref_count());
  BOOST_REQUIRE_EQUAL(1, one->get_ref_count());
  BOOST_REQUIRE_EQUAL(nullptr, x1->get_next());
  BOOST_REQUIRE_EQUAL(zero, x1->get_son(0));
  BOOST_REQUIRE_EQUAL(one, x1->get_son(1));

  sons_t x0sons = node_t::make_son_container(2);
  x0sons[0]     = zero;
  x0sons[1]     = x1;
  node_t* x0    = manager.make_internal_node(0, TEDDY_MOVE(x0sons));
  x0->inc_ref_count();
  BOOST_REQUIRE_EQUAL(0, x0->get_index());
  BOOST_REQUIRE_EQUAL(1, x0->get_ref_count());
  BOOST_REQUIRE_EQUAL(2, zero->get_ref_count());
  x0->set_next(x1);
  BOOST_REQUIRE_EQUAL(x1, x0->get_next());
  BOOST_REQUIRE_EQUAL(nullptr, x1->get_next());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(getters, Fixture, nodes_fixtures, Fixture)
{
  typename Fixture::manager_t manager = make_manager(*this);

  BOOST_REQUIRE_EQUAL(Fixture::varCount_, manager.get_var_count());
  BOOST_REQUIRE_EQUAL(Fixture::varCount_, manager.get_leaf_level());

  std::vector<int32> const& actualOrder = manager.get_order();
  for (int32 i = 0; i < manager.get_var_count(); ++i)
  {
    BOOST_REQUIRE_EQUAL(
      Fixture::order_[as_uindex(i)],
      actualOrder[as_uindex(i)]
    );
  }

  std::vector<int32> const actualDomains = manager.get_domains();
  for (int32 i = 0; i < manager.get_var_count(); ++i)
  {
    BOOST_REQUIRE_EQUAL(
      Fixture::domains_[as_uindex(i)],
      actualDomains[as_uindex(i)]
    );
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(
  domain_product,
  Fixture,
  nodes_fixtures,
  Fixture
)
{
  typename Fixture::manager_t manager = make_manager(*this);
  int64 expectedProduct               = 1;
  for (int32 const mi : Fixture::domains_)
  {
    expectedProduct *= mi;
  }
  int64 const actualProduct
    = manager.domain_product(0, manager.get_var_count());
  BOOST_REQUIRE_EQUAL(expectedProduct, actualProduct);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace teddy::test