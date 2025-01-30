#ifndef LIBTEDDY_TSL_PLA_DESCRIPTION_HPP
#define LIBTEDDY_TSL_PLA_DESCRIPTION_HPP

#include <fmt/core.h>

#include <string>
#include <vector>

namespace teddy::tsl {

struct pla_line {
  std::vector<int> input_;
  int output_;
};

struct mvl_pla_description {
  int id_desc_;
  int input_count_;
  int product_count_;
  std::vector<int> domains_;
  int codomain_;
  std::vector<pla_line> values_;
  std::string raw_pla_;
};

// TODO(michal): inl
inline auto operator<< (
  std::ostream &ost,
  const mvl_pla_description &desc
) -> std::ostream & {
  ost << fmt::format("[mvl_pla_{}]", desc.id_desc_);
  return ost;
}

}  // namespace teddy::tsl

#endif