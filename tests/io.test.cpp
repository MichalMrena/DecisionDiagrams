#include <libteddy/inc/io.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>

namespace teddy::tests {

BOOST_AUTO_TEST_SUITE(io)

BOOST_AUTO_TEST_CASE(from_pla_binary_1) {
  std::stringstream iost;
  iost <<
    ".i 5\n"
    ".o 1\n"
    ".ilb d c b a e\n"
    ".ob xor5\n"
    ".p 16\n"
    "11111 1\n"
    "01110 1\n"
    "10110 1\n"
    "00111 1\n"
    "11010 1\n"
    "01011 1\n"
    "10011 1\n"
    "00010 1\n"
    "11100 1\n"
    "01101 1\n"
    "10101 1\n"
    "00100 1\n"
    "11001 1\n"
    "01000 1\n"
    "10000 1\n"
    "00001 1\n"
    ".e";

  std::optional<pla_file_binary> file = load_binary_pla(iost, nullptr);
  BOOST_REQUIRE_MESSAGE(file.has_value(), "Load simple PLA.");

  BOOST_REQUIRE_EQUAL(file->input_count_, 5);
  BOOST_REQUIRE_EQUAL(file->output_count_, 1);
  BOOST_REQUIRE_EQUAL(file->product_count_, 16);
  BOOST_REQUIRE_EQUAL(file->input_labels_[0], "d");
  BOOST_REQUIRE_EQUAL(file->input_labels_[1], "c");
  BOOST_REQUIRE_EQUAL(file->input_labels_[2], "b");
  BOOST_REQUIRE_EQUAL(file->input_labels_[3], "a");
  BOOST_REQUIRE_EQUAL(file->input_labels_[4], "e");
  BOOST_REQUIRE_EQUAL(file->output_labels_[0], "xor5");

  bdd_manager manager(file->input_count_, 1'000);
  std::vector<bdd_manager::diagram_t> diagrams =
    teddy::io::from_pla(manager, *file);
  BOOST_REQUIRE_EQUAL(diagrams.size(), 1);
  const bdd_manager::diagram_t &d = diagrams.front();

  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {1, 1, 1, 1, 1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {0, 1, 1, 1, 0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {1, 0, 1, 1, 0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {0, 0, 1, 1, 1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {1, 1, 0, 1, 0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {0, 1, 0, 1, 1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {1, 0, 0, 1, 1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {0, 0, 0, 1, 0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {1, 1, 1, 0, 0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {0, 1, 1, 0, 1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {1, 0, 1, 0, 1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {0, 0, 1, 0, 0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {1, 1, 0, 0, 1}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {0, 1, 0, 0, 0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {1, 0, 0, 0, 0}), 1);
  BOOST_REQUIRE_EQUAL(manager.evaluate(d, std::array {0, 0, 0, 0, 1}), 1);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace teddy::tests
