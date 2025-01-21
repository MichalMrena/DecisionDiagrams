#ifdef TEDDY_NO_HEADER_ONLY
#include <libteddy/inc/io.hpp>
#endif

#include <libteddy/impl/config.hpp>

namespace teddy {

TEDDY_DEF_INL auto io::from_pla(
  binary_manager &manager,
  const pla_file_binary &file
) -> std::vector<binary_manager::diagram_t> {
  using teddy::ops::AND;
  using teddy::ops::OR;

  std::vector<bdd_t> outputs;
  outputs.reserve(as_usize(file.output_count_));
  const size_t product_count = file.inputs_.size();

  // For each output
  for (int32 oi = 0; oi < file.output_count_; ++oi) {
    // Zero as neutral element for OR
    bdd_t output = manager.constant(0);

    // For each input line (cube pair)
    for (int32 li = 0; as_usize(li) < product_count; ++li) {
      // Skip outputs that are not 1, we work with ON-set
      if (file.outputs_[as_uindex(li)].get_value(oi) != 1) {
        continue;
      }

      // One as neural element for AND
      bdd_t product = manager.constant(1);
      for (int32 i = 0; i < file.input_count_; ++i) {
        if (file.inputs_[as_uindex(li)].get_value(i) == 1) {
          product = manager.apply<AND>(product, manager.variable(i));
        } else if (file.inputs_[as_uindex(li)].get_value(i) == 0) {
          product = manager.apply<AND>(product, manager.variable_not(i));
        }
      }

      output = manager.apply<OR>(output, product);
    }

    outputs.push_back(output);
  }

  return outputs;
}

} // namespace teddy
