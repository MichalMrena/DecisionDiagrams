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
  std::stringstream ist;
  ist <<
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

  std::optional<pla_file_binary> file = load_binary_pla(ist, nullptr);
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

BOOST_AUTO_TEST_CASE(from_pla_mvl_1) {
  std::stringstream ist;
  ist << ""
    ".mv 8 5 4 14 10"
    ".p 14"
    "0-010|1000|10000000000000|0010000000"
    "10-10|1000|01000000000000|1000000000"
    "0-111|1000|00100000000000|0001000000"
    "0-10-|1000|00010000000000|0001000000"
    "00000|1000|00001000000000|1000000000"
    "00010|1000|00000100000000|0010000000"
    "01001|1000|00000010000000|0000000010"
    "0101-|1000|00000001000000|0000000000"
    "0-0-0|1000|00000000100000|1000000000"
    "10000|1000|00000000010000|0000000000"
    "11100|1000|00000000001000|0010000000"
    "10-10|1000|00000000000100|0000000000"
    "11111|1000|00000000000010|0010000000"
    "11111|0001|00000000000001|0000000001";

  std::optional<pla_file_mvl> file = load_mvl_pla(ist, nullptr);
  BOOST_REQUIRE_MESSAGE(file.has_value(), "Load simple mvl PLA.");
  BOOST_REQUIRE_EQUAL(file->input_count_, 7);
  BOOST_REQUIRE_EQUAL(file->product_count_, 14);
  BOOST_REQUIRE_EQUAL(file->codomain_, 10);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace teddy::tests
