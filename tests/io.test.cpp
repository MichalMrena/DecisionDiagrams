#include <libteddy/inc/io.hpp>

#include <libtsl/pla-description.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_suite.hpp>

namespace teddy::tests {

const std::array binary_plas = {
  tsl::binary_pla_description {
    .id_desc_ = 1,
    .is_valid_ = true,
    .input_count_ = 5,
    .output_count_ = 1,
    .product_count_ = 16,
    .input_labels_ = {"d", "c", "b", "a", "e"},
    .output_labels_ = {"xor5"},
    .values_ = {
      tsl::pla_line{.input_ = {1,1,1,1,1}, .output_ = {1}},
      tsl::pla_line{.input_ = {0,1,1,1,0}, .output_ = {1}},
      tsl::pla_line{.input_ = {1,0,1,1,0}, .output_ = {1}},
      tsl::pla_line{.input_ = {0,0,1,1,1}, .output_ = {1}},
      tsl::pla_line{.input_ = {1,1,0,1,0}, .output_ = {1}},
      tsl::pla_line{.input_ = {0,1,0,1,1}, .output_ = {1}},
      tsl::pla_line{.input_ = {1,0,0,1,1}, .output_ = {1}},
      tsl::pla_line{.input_ = {0,0,0,1,0}, .output_ = {1}},
      tsl::pla_line{.input_ = {1,1,1,0,0}, .output_ = {1}},
      tsl::pla_line{.input_ = {0,1,1,0,1}, .output_ = {1}},
      tsl::pla_line{.input_ = {1,0,1,0,1}, .output_ = {1}},
      tsl::pla_line{.input_ = {0,0,1,0,0}, .output_ = {1}},
      tsl::pla_line{.input_ = {1,1,0,0,1}, .output_ = {1}},
      tsl::pla_line{.input_ = {0,1,0,0,0}, .output_ = {1}},
      tsl::pla_line{.input_ = {1,0,0,0,0}, .output_ = {1}},
      tsl::pla_line{.input_ = {0,0,0,0,1}, .output_ = {1}}
    },
    .raw_pla_ =
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
      ".e"
  }
};

const std::array<tsl::mvl_pla_description, 1> mvl_plas = {
  tsl::mvl_pla_description {
    .id_desc_ = 1,
    .is_valid_ = true,
    .input_count_ = 7,
    .product_count_ = 14,
    .domains_ = {2,2,2,2,2,4,14,10},
    .codomain_ = 10,
    .values_ = {
      tsl::pla_line{.input_ = {0,0,0,1,0,0,0}, .output_ = {2}},
      tsl::pla_line{.input_ = {1,0,0,1,0,0,1}, .output_ = {0}},
      tsl::pla_line{.input_ = {0,0,1,1,1,0,2}, .output_ = {3}},
      tsl::pla_line{.input_ = {0,0,1,0,0,0,3}, .output_ = {3}},
      tsl::pla_line{.input_ = {0,0,0,0,0,0,4}, .output_ = {0}},
      tsl::pla_line{.input_ = {0,0,0,1,0,0,5}, .output_ = {3}},
      tsl::pla_line{.input_ = {0,1,0,0,1,0,6}, .output_ = {8}},
      tsl::pla_line{.input_ = {0,1,0,1,0,0,7}, .output_ = {0}},
      tsl::pla_line{.input_ = {0,0,0,0,0,0,8}, .output_ = {0}},
      tsl::pla_line{.input_ = {1,0,0,0,0,0,9}, .output_ = {5}},
      tsl::pla_line{.input_ = {1,1,1,0,0,0,10}, .output_ = {2}},
      tsl::pla_line{.input_ = {1,0,0,1,0,0,11}, .output_ = {3}},
      tsl::pla_line{.input_ = {1,1,1,1,1,0,12}, .output_ = {2}},
      tsl::pla_line{.input_ = {1,1,1,1,1,3,13}, .output_ = {9}},
    },
    .raw_pla_ =
      ".mv 8 5 4 14 10\n"
      ".p 14\n"
      "0-010|1000|10000000000000|0010000000\n"
      "10-10|1000|01000000000000|1000000000\n"
      "0-111|1000|00100000000000|0001000000\n"
      "0-10-|1000|00010000000000|0001000000\n"
      "00000|1000|00001000000000|1000000000\n"
      "00010|1000|00000100000000|0010000000\n"
      "01001|1000|00000010000000|0000000010\n"
      "0101-|1000|00000001000000|1000000000\n"
      "0-0-0|1000|00000000100000|1000000000\n"
      "10000|1000|00000000010000|0000010000\n"
      "11100|1000|00000000001000|0010000000\n"
      "10-10|1000|00000000000100|0001000000\n"
      "11111|1000|00000000000010|0010000000\n"
      "11111|0001|00000000000001|0000000001\n"
  }
};

BOOST_AUTO_TEST_SUITE(io)

BOOST_DATA_TEST_CASE(from_pla_binary, binary_plas, pla_desc) {
  std::stringstream ist(pla_desc.raw_pla_);

  std::optional<pla_file_binary> file = load_binary_pla(ist, nullptr);
  if (not pla_desc.is_valid_) {
    BOOST_TEST(not file.has_value());
    return;
  }

  BOOST_REQUIRE_MESSAGE(file.has_value(), "Load PLA");
  BOOST_REQUIRE_EQUAL(file->input_count_, pla_desc.input_count_);
  BOOST_REQUIRE_EQUAL(file->output_count_, pla_desc.output_count_);
  BOOST_REQUIRE_EQUAL(file->product_count_, pla_desc.product_count_);
  for (int i = 0; i < pla_desc.input_count_; ++i) {
    BOOST_REQUIRE_EQUAL(
      file->input_labels_[as_uindex(i)],
      pla_desc.input_labels_[as_uindex(i)]);
  }

  bdd_manager manager(file->input_count_, 1'000);
  std::vector<bdd_manager::diagram_t> ds = teddy::io::from_pla(manager, *file);
  BOOST_REQUIRE_EQUAL(ssize(ds), pla_desc.output_count_);

  for (const auto &[in, out] : pla_desc.values_) {
    for (int i = 0; i < pla_desc.output_count_; ++i) {
      BOOST_REQUIRE_EQUAL(manager.evaluate(ds[i], in), out[i]);
    }
  }
}

BOOST_DATA_TEST_CASE(from_pla_mvl, mvl_plas, pla_desc) {
  std::istringstream ist(pla_desc.raw_pla_);

  std::optional<pla_file_mvl> file = load_mvl_pla(ist, nullptr);
  if (not pla_desc.is_valid_) {
    BOOST_TEST(not file.has_value());
    return;
  }

  BOOST_REQUIRE_MESSAGE(file.has_value(), "Load simple mvl PLA.");
  BOOST_REQUIRE_EQUAL(file->input_count_, pla_desc.input_count_);
  BOOST_REQUIRE_EQUAL(file->product_count_, pla_desc.product_count_);
  BOOST_REQUIRE_EQUAL(file->codomain_, pla_desc.codomain_);
  for (int i = 0; i < pla_desc.input_count_; ++i) {
    BOOST_REQUIRE_EQUAL(
      file->domains_[as_uindex(i)],
      pla_desc.domains_[as_uindex(i)]);
  }

  // TODO(michal): evaluate
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace teddy::tests
