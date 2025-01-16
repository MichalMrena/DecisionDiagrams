#include <libteddy/impl/pla_file.hpp>

#include <fstream>

namespace teddy {

TEDDY_DEF_INLINE auto load_binary_pla(
  const std::filesystem::path &path,
  std::ostream *errst
) -> std::optional<pla_file_binary> {
  std::ifstream ifst(path);
  if (not ifst.is_open()) {
    if (errst != nullptr) {
      *errst << "load_binary_pla: Failed to open: " << path << "\n";
    }
  }
  return load_binary_pla(ifst, errst);
}

TEDDY_DEF_INLINE auto load_binary_pla(
  std::istream &ist,
  std::ostream *errst
) -> std::optional<pla_file_binary> {
  // Outputs error message for given line when error stream is provided
  auto const err_out = [errst] (const int32 line_num, auto const &...args) {
    if (errst != nullptr) {
      *errst << "Line " << line_num << ": ";
      ((*errst << args), ...); // NOLINT
      *errst << "\n";
    }
  };

  // Initialize an empty file
  pla_file_binary result;
  result.input_count_ = -1;
  result.output_count_ = -1;

  // Optional option, if provided, we can pre-allocate space for lines
  int32 product_count = -1;

  std::string raw_line;
  int32 line_num = 0;
  bool has_next_line = false;

  // Read header with options
  while (std::getline(ist, raw_line)) {
    ++line_num;
    const std::string_view line = tools::trim(raw_line);

    // Skip empty line
    if (line.empty()) {
      continue;
    }

    // Skip comment
    if (line[0] == '#') {
      continue;
    }

    // Not an option, start parsing cubes
    if (line[0] != '.') {
      has_next_line = true;
      break;
    }

    // Tokenize the line
    const std::vector<std::string_view> tokens = tools::to_words(line);
    const std::string_view key = tokens[0];

    // Number of inputs
    if (key == ".i") {
      if (tokens.size() < 2) {
        err_out(line_num, ".i option requires argument");
        return std::nullopt;
      }
      const std::optional<int32> in_count_opt = tools::parse<int32>(tokens[1]);
      if (not in_count_opt.has_value()) {
        err_out(
          line_num,
          ".i option requires positive integer argument. Got ",
          tokens[1],
          " instead");
        return std::nullopt;
      }
      result.input_count_ = *in_count_opt;
    }

    // Number of outputs
    if (key == ".o") {
      if (tokens.size() < 2) {
        err_out(line_num, ".o option requires argument");
        return std::nullopt;
      }
      const std::optional<int32> out_count_opt = tools::parse<int32>(tokens[1]);
      if (not out_count_opt.has_value()) {
        err_out(
          line_num,
          ".o option requires positive integer argument. Got ",
          tokens[1],
          " instead");
        return std::nullopt;
      }
      result.output_count_ = *out_count_opt;
    }

    // Input labels
    if (key == ".ilb") {
      if (tokens.size() != as_usize(result.input_count_)) {
        err_out(
          line_num,
          "Invalid input label count. Expected ",
          result.input_count_,
          " got ",
          tokens.size() - 1);
        return std::nullopt;
      }
      for (size_t i = 1; i < tokens.size(); ++i) {
        result.input_labels_.emplace_back(tokens[i]);
      }
    }

    // Output labels
    if (key == ".ob") {
      if (tokens.size() != as_usize(result.output_count_)) {
        err_out(
          line_num,
          "Invalid output label count. Expected ",
          result.output_count_,
          " got ",
          tokens.size() - 1);
        return std::nullopt;
      }
      for (size_t i = 1; i < tokens.size(); ++i) {
        result.output_labels_.emplace_back(tokens[i]);
      }
    }

    // Multiple-valued variables
    if (key == ".mv") {
      err_out(
        line_num,
        ".mv option is invalid for binary PLA, use load_mvl_pla instead.");
      return std::nullopt;
    }
  }

  // No cubes
  if (not has_next_line) {
    err_out(line_num, "Expected products, found nothing.");
    return std::nullopt;
  }

  // Verify that we have input count
  if (result.input_count_ == -1) {
    err_out(line_num, "Required option .i not provided.");
    return std::nullopt;
  }

  // Verify that we have output count
  if (result.input_count_ == -1) {
    err_out(line_num, "Required option .o not provided.");
    return std::nullopt;
  }

  // Reserve data for products if the product count is provided
  if (product_count != -1) {
    result.inputs_.reserve(as_usize(product_count));
    result.outputs_.reserve(as_usize(product_count));
  }

  // Parse cubes
  do {
    const std::string_view line = tools::trim(raw_line);

    // Skip empty line
    if (line.empty()) {
      continue;
    }

    // Skip comment
    if (line[0] == '#') {
      continue;
    }

    // Explicit end of file, rest is ignored
    if (line == ".e" || line == ".end") {
      break;
    }

    // Read inputs
    int32 inputs_read = 0;
    int32 i = 0;
    result.inputs_.emplace_back(result.input_count_);
    cube &in_cube = result.inputs_.back();
    while (as_usize(i) < line.length() && inputs_read < result.input_count_) {
      const char c = line[as_uindex(i)];

      // Allways move to the next char.
      ++i;

      // Skip delimiter
      if (c == ' ' || c == '|') {
        continue;
      }

      const int32 index = inputs_read;
      switch (c) {
        case '0':
          in_cube.set_value(index, 0);
          break;
        case '1':
        case '4':
          in_cube.set_value(index, 1);
          break;
        case '-':
        case '~':
        case '2':
        case '3':
          in_cube.set_value(index, cube::DC);
          break;
        default:
          err_out(line_num, "Unexpected end of line, expected more inputs.");
          return std::nullopt;
      }

      ++inputs_read;
    }

    // Verify that we read enough inputs
    if (inputs_read != result.input_count_) {
      err_out(line_num, "Not enough inputs.");
      return std::nullopt;
    }

    // Read outputs
    int32 outputs_read = 0;
    i = 0;
    result.outputs_.emplace_back(result.output_count_);
    cube &out_cube = result.outputs_.back();
    while (outputs_read < result.output_count_) {
      const char c = line[as_uindex(i)];

      // Allways move to the next char.
      ++i;

      // Skip delimiter
      if (c == ' ' || c == '|') {
        continue;
      }

      const int32 index = outputs_read;
      switch (c) {
        case '0':
          out_cube.set_value(index, 0);
          break;
        case '1':
        case '4':
          out_cube.set_value(index, 1);
          break;
        case '-':
        case '~':
        case '2':
        case '3':
          out_cube.set_value(index, cube::DC);
          break;
        default:
          err_out(line_num, "Unexpected end of line, expected more outputs.");
          return std::nullopt;
      }

      ++outputs_read;
    }

    // Verify that we read enough outputs
    if (outputs_read != result.output_count_) {
      err_out(line_num, "Not enough outputs.");
      return std::nullopt;
    }

    ++line_num;
  } while(std::getline(ist, raw_line));

  // Check for product count consistency if possible
  if (product_count != -1 && result.inputs_.size() != as_usize(product_count)) {
    err_out(line_num, "Product count not consistend with actual line count.");
    return std::nullopt;
  }

  // Finally, everything is correctly loaded, we can return the result
  return result;
}

TEDDY_DEF_INLINE auto load_mvl_pla(
  const std::filesystem::path &path,
  std::ostream *errst
) -> std::optional<pla_file_binary> {
  return std::nullopt;
}

TEDDY_DEF_INLINE auto load_mvl_pla(
  const std::istream &ist,
  std::ostream *errst
) -> std::optional<pla_file_binary> {
  return std::nullopt;
}

} // namespace teddy