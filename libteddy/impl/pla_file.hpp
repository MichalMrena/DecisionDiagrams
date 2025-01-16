#ifndef LIBTEDDY_DETAILS_PLA_FILE_HPP
#define LIBTEDDY_DETAILS_PLA_FILE_HPP

#include <libteddy/impl/cube.hpp>
#include <libteddy/impl/debug.hpp>
#include <libteddy/impl/tools.hpp>
#include <libteddy/impl/tools_io.hpp>
#include <libteddy/impl/types.hpp>

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace teddy {

/**
 * \brief TODO
 */
struct pla_file_binary {
  int32 input_count_;
  int32 output_count_;
  std::vector<cube> inputs_;
  std::vector<cube> outputs_;
  std::vector<std::string> input_labels_;
  std::vector<std::string> output_labels_;
};

/**
  * \brief Loads PLA file from a file at given path
  * \param path Path to the file
  * \param errst Optional output stream used for error logs
  * \return Optional holding instance of \c pla_file_binary or \c std::nullopt
  *
  * Recognizes format described in:
  * https://user.engineering.uiowa.edu/~switchin/OldSwitching/espresso.5.html
  *
  * Currently recognizes the following required options:
  *   .i, .o,
  * and the following optional options:
  *   .p, .ilb, .ob
  */
inline auto load_binary_pla(
  const std::filesystem::path &path,
  std::ostream *errst = nullptr
) -> std::optional<pla_file_binary>;

/**
  * \brief Loads PLA file from a file at given path
  * \param ist input stream
  * \param errst Optional output stream used for error logs
  * \return Optional holding instance of \c pla_file_binary or \c std::nullopt
  *
  * Recognizes format described in:
  * https://user.engineering.uiowa.edu/~switchin/OldSwitching/espresso.5.html
  *
  * Currently recognizes the following required options:
  *   .i, .o,
  * and the following optional options:
  *   .p, .ilb, .ob
  */
inline auto load_binary_pla(
  std::istream &ist,
  std::ostream *errst = nullptr
) -> std::optional<pla_file_binary>;


/**
 * \brief TODO
 */
struct pla_file_mvl {
  std::vector<int32> domains_;
};

/**
 * \brief TODO
 */
inline auto load_mvl_pla(
  const std::filesystem::path &path,
  std::ostream *errst = nullptr
) -> std::optional<pla_file_binary>;

/**
 * \brief TODO
 */
inline auto load_mvl_pla(
  const std::istream &ist,
  std::ostream *errst = nullptr
) -> std::optional<pla_file_binary>;

} // namespace teddy

// Include definitions in header-only mode
#ifndef TEDDY_NO_HEADER_ONLY
#include <libteddy/impl/pla_file.cpp>
#endif

#endif