#include <libteddy/impl/pla.hpp>

#include <fstream>
#include <span>

namespace teddy {

namespace details {

TEDDY_ANON_NAMESPACE_BEGIN

template<class... Args>
auto err_out(std::ostream *errst, int32 line_num, Args&&... args) -> void {
  if (errst != nullptr) {
    *errst << "Line " << line_num << ": ";
    ((*errst << args), ...); // NOLINT
    *errst << "\n";
  }
}

TEDDY_DEF auto read_int_option(
  const std::span<const std::string_view> tokens,
  const std::string_view key,
  const int32 line_num,
  std::ostream *errst,
  int32 &out
) -> bool {
  if (out != Undefined) {
    details::err_out(errst, line_num, "Multiple definitions of ", key);
    return false;
  }
  if (tokens.size() != 1) {
    err_out(errst, line_num, key, " option requires single argument");
    return false;
  }
  const std::optional<int32> in_count_opt = tools::parse<int32>(tokens[0]);
  if (not in_count_opt.has_value()) {
    err_out(
        errst,
        line_num,
        key,
        " option requires integer argument. Got ",
        tokens[1],
        " instead");
    return false;
  }
  out = *in_count_opt;
  return true;
}

TEDDY_DEF auto read_labels(
  const std::span<const std::string_view> tokens,
  const std::string_view key,
  const int32 count,
  const int32 line_num,
  std::ostream *errst,
  std::vector<std::string> &out
) -> bool {
  if (not out.empty()) {
    details::err_out(errst, line_num, "Multiple definitions of ", key);
    return false;
  }
  if (tokens.empty()) {
    details::err_out(errst, line_num, "No labels provided for ", key);
    return false;
  }
  if (count != Undefined && tokens.size() != as_usize(count)) {
    details::err_out(errst, line_num, key, " invalid label count.");
    return false;
  }
  for (const std::string_view token : tokens) {
    out.emplace_back(token);
  }
  return true;
}

TEDDY_DEF auto decode_mvl_value(
  const std::string_view str,
  const int domain,
  const int line_num,
  std::ostream *errst,
  int &out
) -> bool {
  if (ssize(str) != domain) {
    details::err_out(
      errst,
      line_num,
      "Invalid token size. Expected ",
      domain,
      " found ",
      ssize(str));
    return false;
  }
  int val = 0;
  while (val < ssize(str) && str[as_uindex(val)] != '1') {
    ++val;
  }
  if (val == ssize(str)) {
    details::err_out(errst, line_num, "Did not find any 1");
    return false;
  }
  out = val;
  return true;
}

TEDDY_ANON_NAMESPACE_END

}  // namespace details

TEDDY_DEF auto load_binary_pla(
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

TEDDY_DEF auto load_binary_pla(
  std::istream &ist,
  std::ostream *errst
) -> std::optional<pla_file_binary> {
  // Initialize an empty file
  pla_file_binary result;
  result.input_count_ = Undefined;
  result.output_count_ = Undefined;
  result.product_count_ = Undefined;

  // Optional option, if provided, we can pre-allocate space for lines
  int32 product_count = Undefined;

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
    const std::vector<std::string_view> tokens = tools::to_words(line, " ");
    const std::string_view key = tokens[0];

    // Number of inputs
    if (key == ".i") {
      if (not details::read_int_option(
        std::span(tokens).subspan(1),
        ".i",
        line_num,
        errst,
        result.input_count_)
      ) {
        return std::nullopt;
      }
    }

    // Number of outputs
    if (key == ".o") {
      if (not details::read_int_option(
        std::span(tokens).subspan(1),
        ".o",
        line_num,
        errst,
        result.output_count_)
      ) {
        return std::nullopt;
      }
    }

    // Optional number of products
    if (key == ".p") {
      if (not details::read_int_option(
        std::span(tokens).subspan(1), ".p", line_num, errst, product_count)
      ) {
        return std::nullopt;
      }
    }

    // Input labels
    if (key == ".ilb") {
      if (not details::read_labels(
        std::span(tokens).subspan(1),
        ".ilb",
        result.input_count_,
        line_num,
        errst,
        result.input_labels_)
      ) {
        return std::nullopt;
      };
    }

    // Output labels
    if (key == ".ob") {
      if (not details::read_labels(
        std::span(tokens).subspan(1),
        ".ob",
        result.output_count_,
        line_num,
        errst,
        result.output_labels_)
      ) {
        return std::nullopt;
      };
    }

    // Multiple-valued variables
    if (key == ".mv") {
      details::err_out(
        errst,
        line_num,
        ".mv option is invalid for binary PLA, use load_mvl_pla instead");
      return std::nullopt;
    }
  }

  // No cubes
  if (not has_next_line) {
    details::err_out(errst, line_num, "Expected products, found nothing");
    return std::nullopt;
  }

  // Verify that we have input count
  if (result.input_count_ == Undefined) {
    details::err_out(errst, line_num, "Required option .i not provided");
    return std::nullopt;
  }

  // Verify that we have output count
  if (result.input_count_ == Undefined) {
    details::err_out(errst, line_num, "Required option .o not provided");
    return std::nullopt;
  }

  // Reserve data for products if the product count is provided
  if (product_count != Undefined) {
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
    while (i < ssize(line) && inputs_read < result.input_count_) {
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
          details::err_out(
            errst, line_num, "Unexpected end of line, expected more inputs.");
          return std::nullopt;
      }

      ++inputs_read;
    }

    // Verify that we read enough inputs
    if (inputs_read != result.input_count_) {
      details::err_out(errst, line_num, "Not enough inputs.");
      return std::nullopt;
    }

    // Read outputs
    int32 outputs_read = 0;
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
          details::err_out(
            errst, line_num, "Unexpected end of line, expected more outputs.");
          return std::nullopt;
      }

      ++outputs_read;
    }

    // Verify that we read enough outputs
    if (outputs_read != result.output_count_) {
      details::err_out(errst, line_num, "Not enough outputs.");
      return std::nullopt;
    }

    ++line_num;
  } while(std::getline(ist, raw_line));

  // Check for product count consistency if possible
  if (product_count != Undefined &&
       result.inputs_.size() != as_usize(product_count)
  ) {
    details::err_out(
      errst,
      line_num,
      "Product count not consistent with the actual line count.");
    return std::nullopt;
  }

  result.product_count_ = static_cast<int32>(result.inputs_.size());

  // Finally, everything is correctly loaded, we can return the result
  return result;
}

TEDDY_DEF auto load_mvl_pla(
  const std::filesystem::path &path,
  std::ostream *errst
) -> std::optional<pla_file_mvl> {
  std::ifstream ifst(path);
  if (not ifst.is_open()) {
    if (errst != nullptr) {
      *errst << "load_mvl_pla: Failed to open: " << path << "\n";
    }
  }
  return load_mvl_pla(ifst, errst);
}

TEDDY_DEF auto load_mvl_pla(
  std::istream &ist,
  std::ostream *errst
) -> std::optional<pla_file_mvl> {
  // Initialize an empty file
  pla_file_mvl result;
  result.input_count_ = Undefined;
  result.codomain_ = Undefined;

  // Optional option, if provided, we can pre-allocate space for lines
  int32 product_count = Undefined;

  // Number of binary inputs
  int32 bin_input_count = Undefined;

  // Number of mvl variables
  int32 mvl_var_count = Undefined;

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
    const std::vector<std::string_view> tokens = tools::to_words(line, " ");
    const std::string_view key = tokens[0];

    // Definition of mvl variables
    if (key == ".mv") {
      if (not result.domains_.empty()) {
        details::err_out(errst, line_num, "Multiple definitions of .mv");
        return std::nullopt;
      }

      if (tokens.size() < 3) {
        details::err_out(
          errst, line_num, ".mb option requires at least 2 arguments");
        return std::nullopt;
      }

      // Total number of variables (including output)
      int32 var_count = Undefined;
      if (not details::read_int_option(
        std::span(tokens).subspan(1, 1),
        "[num_var]",
        line_num,
        errst,
        var_count)
      ) {
        return std::nullopt;
      }
      result.input_count_ = var_count - 1;

      // Verify variable count
      if (var_count < 2) {
        details::err_out(errst, line_num, ".mv requires at least 2 variables");
        return std::nullopt;
      }

      // Number of binary variables
      if (not details::read_int_option(
        std::span(tokens).subspan(2, 1),
        "[num_binary_var]",
        line_num,
        errst,
        bin_input_count)
      ) {
        return std::nullopt;
      }

      // Number of MVL variables
      mvl_var_count = var_count - bin_input_count;
      if (static_cast<int32>(tokens.size()) - 3 != mvl_var_count) {
        details::err_out(errst, line_num, ".mv invalid number of arguments");
        return std::nullopt;
      }

      // Populate the domains of binary variables
      for (int32 i = 0; i < bin_input_count; ++i) {
        result.domains_.push_back(2);
      }

      // Parse and populate domains of MVL variables
      for (int32 i = 0; i < mvl_var_count; ++i) {
        int32 domain = Undefined;
        if (not details::read_int_option(
          std::span(tokens).subspan(as_uindex(i + 3), 1),
          "[di]",
          line_num,
          errst,
          domain)
        ) {
          return std::nullopt;
        }
        result.domains_.push_back(domain);
      }

      // Initialize codomain as the domain of the last (output) variable
      result.codomain_ = result.domains_.back();
      result.domains_.pop_back();
    }
  }

  // Check if .mv was defined
  if (result.domains_.empty() || result.input_count_ == Undefined) {
    details::err_out(errst, line_num, "Missing required .mv option");
    return std::nullopt;
  }

  // No cubes
  if (not has_next_line) {
    details::err_out(errst, line_num, "Expected products, found nothing.");
    return std::nullopt;
  }

  // Reserve data for products if the product count is provided
  if (product_count != Undefined) {
    result.inputs_.reserve(as_usize(product_count));
    result.output_.reserve(as_usize(product_count));
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

    details::array<int32> inputs(result.input_count_);
    int32 output = Undefined;

    // Read leading binary inputs
    int bin_inputs_read = 0;
    int i = 0;
    while (i < ssize(line) && bin_inputs_read < bin_input_count) {
      const char c = line[as_uindex(i)];

      // Allways move to the next char.
      ++i;

      // Skip delimiter
      if (c == ' ' || c == '|') {
        continue;
      }

      const int32 index = bin_inputs_read;
      switch (c) {
        case '0':
          inputs[index] = 0;
          break;
        case '1':
        case '4':
          inputs[index] = 1;
          break;
        case '-':
        case '~':
        case '2':
        case '3':
          inputs[index] = Undefined;
          break;
        default:
          details::err_out(
            errst, line_num, "Unexpected end of line, expected more inputs.");
          return std::nullopt;
      }

      ++bin_inputs_read;
    }

    // Verify that we read enough inputs
    if (bin_inputs_read != bin_input_count) {
      details::err_out(errst, line_num, "Not enough binary inputs.");
      return std::nullopt;
    }

    // Move to the next char -- this should be a delimiter of mvl variables
    ++i;

    // Split rest of the lines on delimiters
    const std::vector<std::string_view> tokens =
      tools::to_words(line.substr(as_uindex(i)), " |");

    // Verify that we have correct count of mvl variables
    if (ssize(tokens) != mvl_var_count) {
      details::err_out(
        errst,
        line_num,
        "Invalid count of mvl variables. Expected ",
        mvl_var_count,
        " found ",
        ssize(tokens));
      return std::nullopt;
    }

    // Decode mvl variables
    for (int ti = 0; ti < mvl_var_count - 1; ++ti) {
      const int var_index = ti + bin_input_count;
      const std::string_view token = tokens[as_uindex(ti)];
      const int domain = result.domains_[as_uindex(var_index)];
      int val = Undefined;
      if (not details::decode_mvl_value(token, domain, line_num, errst, val)) {
        return std::nullopt;
      }
      inputs[var_index] = val;
    }

    // Decode the last -- output -- variable
    const std::string_view token = tokens.back();
    const int domain = result.domains_.back();
    if (not details::decode_mvl_value(token, domain, line_num, errst, output)) {
      return std::nullopt;
    }

    // Now that we have all inputs and output, add them to the result
    result.inputs_.push_back(TEDDY_MOVE(inputs));
    result.output_.push_back(output);

    ++line_num;
  } while(std::getline(ist, raw_line));

  result.product_count_ = static_cast<int32>(result.inputs_.size());

  return result;
}

} // namespace teddy
