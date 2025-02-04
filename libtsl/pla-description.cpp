#include "pla-description.hpp"

#include <fmt/core.h>

namespace teddy::tsl {

auto operator<< (
  std::ostream &ost,
  const binary_pla_description &desc
) -> std::ostream & {
  ost << fmt::format("[binary_pla_{}]", desc.id_desc_);
  return ost;
}

auto operator<< (
  std::ostream &ost,
  const mvl_pla_description &desc
) -> std::ostream & {
  ost << fmt::format("[mvl_pla_{}]", desc.id_desc_);
  return ost;
}

}  // namespace teddy::tsl
