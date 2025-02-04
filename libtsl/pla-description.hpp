#ifndef LIBTEDDY_TSL_PLA_DESCRIPTION_HPP
#define LIBTEDDY_TSL_PLA_DESCRIPTION_HPP

#include <string>
#include <vector>

namespace teddy::tsl {

struct pla_line {
  std::vector<int> input_;
  std::vector<int> output_;
};

struct binary_pla_description {
  int id_desc_;
  bool is_valid_;
  int input_count_;
  int output_count_;
  int product_count_;
  std::vector<std::string> input_labels_;
  std::vector<std::string> output_labels_;
  std::vector<pla_line> values_;
  std::string raw_pla_;
};

auto operator<< (
  std::ostream &ost,
  const binary_pla_description &desc
) -> std::ostream &;

struct mvl_pla_description {
  int id_desc_;
  bool is_valid_;
  int input_count_;
  int product_count_;
  std::vector<int> domains_;
  int codomain_;
  std::vector<pla_line> values_;
  std::string raw_pla_;
};

auto operator<< (
  std::ostream &ost,
  const mvl_pla_description &desc
) -> std::ostream &;

}  // namespace teddy::tsl

#endif
