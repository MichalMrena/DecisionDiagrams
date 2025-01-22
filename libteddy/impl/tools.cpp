#include <libteddy/impl/tools.hpp>

namespace teddy::tools {

TEDDY_DEF auto trim(std::string_view str) -> std::string_view {
  const std::string_view::iterator end = str.end();
  std::string_view::iterator front = str.begin();
  std::string_view::iterator back = str.end() - 1;
  while (front < end && static_cast<bool>(std::isspace(*front))) {
    ++front;
  }
  while (back > front && static_cast<bool>(std::isspace(*back))) {
    --back;
  }
  return {front, back + 1};
}

TEDDY_DEF auto to_words(
  std::string_view str,
  std::string_view delimiters
) -> std::vector<std::string_view> {
  const auto is_delim = [&delimiters](const char c) {
    for (const char delim : delimiters) {
      if (c == delim) {
        return true;
      }
    }
    return false;
  };

  std::vector<std::string_view> words;
  std::string_view::iterator in = str.begin();
  const std::string_view::iterator end = str.end();
  while (in != end) {
    const std::string_view::iterator w_begin = find_if_not(in, end, is_delim);
    const std::string_view::iterator w_end = find_if(w_begin, end, is_delim);
    if (w_begin != w_end) {
      words.emplace_back(w_begin, w_end);
    }
    in = w_end;
  }
  return words;
}

}  // namespace teddy::tools
