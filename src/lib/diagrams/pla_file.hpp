#ifndef MIX_DD_PLA_FILE_HPP
#define MIX_DD_PLA_FILE_HPP

#include "typedefs.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/parsing_utils.hpp"
#include "../utils/file_reader.hpp"
#include "../utils/more_vector.hpp"
#include "../utils/more_iterator.hpp"
#include "../data_structures/bit_vector.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <map>
#include <set>
#include <string_view>
#include <filesystem>

namespace mix::dd
{
    /**
        @brief Represents single line of a pla file.
     */
    struct pla_line
    {
        using cube_t   = bit_vector<2, bool_t>;
        using f_vals_t = bit_vector<2, bool_t>;
        cube_t   cube;
        f_vals_t fVals;
    };

    /**
        @brief Converts cube into a std::vector of `bool_var` structs.
        Example: "--1001" -> { {2, false}, {3, true}, {4, true}, {5, false} }
     */
    auto cube_to_bool_vars (pla_line::cube_t const& cube) -> std::vector<bool_var>;

    /**
        @brief Pla file loaded in memory.
     */
    class pla_file
    {
    public:
        using pla_line_v = std::vector<pla_line>;
        using string_v   = std::vector<std::string>;
        using index_v    = std::vector<index_t>;

    public:
        static auto load_file    (std::string_view path) -> pla_file;
        static auto save_to_file (std::string_view path, pla_file const& file) -> void;

    public:
        auto variable_count    () const    -> index_t;
        auto function_count    () const    -> index_t;
        auto line_count        () const    -> index_t;
        auto get_lines         () const &  -> pla_line_v const&;
        auto get_lines         () const && -> pla_line_v;
        auto get_indices       () const    -> index_v;
        auto get_input_labels  () const &  -> string_v const&;
        auto get_input_labels  () const && -> string_v;
        auto get_output_labels () const &  -> string_v const&;
        auto get_output_labels () const && -> string_v;

        auto swap_vars (std::size_t const i1, std::size_t const i2) -> void;

    private:
        pla_file (pla_line_v lines, string_v inputLabels, string_v outputLabels);

    private:
        pla_line_v lines_;
        string_v   inputLabels_;
        string_v   outputLabels_;
    };

    auto swap (pla_line& lhs, pla_line& rhs) noexcept -> void;

    auto operator== (pla_line const& lhs, pla_line const& rhs) -> bool;
    auto operator!= (pla_line const& lhs, pla_line const& rhs) -> bool;

    auto operator== (pla_file const& lhs, pla_file const& rhs) -> bool;
    auto operator!= (pla_file const& lhs, pla_file const& rhs) -> bool;

// definitions:

    namespace pla_impl
    {
        using option_map = std::map<std::string const, std::string>;

        inline auto constexpr ncount = std::numeric_limits<std::uint32_t>::max();

        inline auto char_to_bool_t (char const c) -> bool_t
        {
            auto constexpr U = log_val_traits<2>::undefined;

            switch (c)
            {
                case '0': return 0;
                case '1': return 1;
                case '-':
                case '~': return U;
                default:
                    throw std::runtime_error {"Invalid pla line. Unknown variable value."};
            }
        }

        inline auto bool_t_to_char (bool_t const b) -> char
        {
            switch (b)
            {
                case 0:  return '0';
                case 1:  return '1';
                default: return '-';
            }
        }

        inline auto is_option_line (std::string_view line) -> bool
        {
            return ! line.empty() && '.' == line.front();
        }

        inline auto is_comment_line (std::string_view line) -> bool
        {
            return '#' == line.front();
        }

        inline auto read_options (utils::file_reader& reader) -> option_map
        {
            auto options = option_map {};

            using namespace utils;
            for (;;)
            {
                auto const keyVal = to_head_tail(shrink_spaces(trim(std::string {reader.peek_line_except()})));

                if (keyVal.first.empty() || is_comment_line(keyVal.first))
                {
                    reader.read_line_except();
                    continue;
                }

                if (! is_option_line(keyVal.first))
                {
                    break;
                }

                options.insert_or_assign(std::move(keyVal.first), std::move(keyVal.second));

                reader.read_line_except();
            }

            return options;
        }

        inline auto has_keys ( option_map const& options
                             , std::initializer_list<char const*> keys ) -> bool
        {
            return std::all_of(std::begin(keys), std::end(keys), [&options](auto const key)
            {
                return options.find(key) != std::end(options);
            });
        }

        inline auto read_input_labels (option_map const& options) -> std::vector<std::string>
        {
            auto const inputCount = utils::parse_except<std::uint32_t>(options.at(".i"));
            auto const labelsIt   = options.find(".ilb");

            if (labelsIt != options.end())
            {
                return utils::to_words(std::move((*labelsIt).second));
            }

            auto labels = utils::vector<std::string>(inputCount);
            for (auto i = 0u; i < inputCount; ++i)
            {
                labels.emplace_back(utils::concat("x", i));
            }

            return labels;
        }

        inline auto read_output_labels (option_map const& options) -> std::vector<std::string>
        {
            auto const outputCount = utils::parse_except<std::uint32_t>(options.at(".o"));
            auto const labelsIt    = options.find(".ob");

            if (labelsIt != options.end())
            {
                return utils::to_words(std::move((*labelsIt).second));
            }

            auto labels = utils::vector<std::string>(outputCount);

            for (auto i = 0u; i < outputCount; ++i)
            {
                labels.emplace_back(utils::concat("y", i));
            }

            return labels;
        }

        inline auto read_data ( utils::file_reader& reader
                              , std::size_t const   varCount
                              , std::size_t const   diagramCount
                              , std::size_t const   lineCount ) -> std::vector<pla_line>
        {
            auto lines = std::vector<pla_line> {};
            if (ncount != lineCount)
            {
                lines.reserve(lineCount);
            }

            do
            {
                using namespace utils;
                auto const lineWords = to_head_tail(trim(shrink_spaces(reader.read_line_except())));

                if (lineWords.first.empty() || is_comment_line(lineWords.first))
                {
                    continue;
                }

                if (lineWords.second.empty())
                {
                    throw std::runtime_error {"Invalid pla line. Expected function definition."};
                }

                auto const variablesStr = std::string {std::move(lineWords.first)};
                auto const valuesStr    = std::string {std::move(lineWords.second)};

                if (variablesStr.size() != varCount || valuesStr.size() != diagramCount)
                {
                    throw std::runtime_error {"Invalid pla line. Expected function definition."};
                }

                auto variables = bit_vector<2, bool_t>(varCount);
                for (auto const c : variablesStr)
                {
                    variables.push_back(char_to_bool_t(c));
                }

                auto values = bit_vector<2, bool_t>(diagramCount);
                for (auto const c : valuesStr)
                {
                    values.push_back(char_to_bool_t(c));
                }

                lines.push_back({std::move(variables), std::move(values)});

            }  while (reader.has_next_line() 
                     && ! utils::starts_with(reader.peek_line_except(), ".e")
                     && ! utils::starts_with(reader.peek_line_except(), ".end"));

            return lines;
        }
    }

    inline auto cube_to_bool_vars
        (pla_line::cube_t const& cube) -> std::vector<bool_var>
    {
        auto vars = utils::vector<bool_var>(cube.size());
        auto is   = utils::range(0u, vars.size());
        for (auto const [val, i] : utils::zip(cube, is))
        {
            if (!is_undefined<2>(val))
            {
                vars.emplace_back(bool_var {static_cast<index_t>(i), 0 == val});
            }
        }
        return vars;
    }

    inline auto swap (pla_line& lhs, pla_line& rhs) noexcept -> void
    {
        using std::swap;
        swap(lhs.cube, rhs.cube);
        swap(lhs.fVals, rhs.fVals);
    }

    inline auto operator== (pla_line const& lhs, pla_line const& rhs) -> bool
    {
        return (lhs.fVals == rhs.fVals) && (lhs.cube == rhs.cube);
    }

    inline auto operator!= (pla_line const& lhs, pla_line const& rhs) -> bool
    {
        return ! (lhs == rhs);
    }

    inline auto operator== (pla_file const& lhs, pla_file const& rhs) -> bool
    {
        return lhs.get_lines() == rhs.get_lines();
    }

    inline auto operator!= (pla_file const& lhs, pla_file const& rhs) -> bool
    {
        return ! (lhs == rhs);
    }

    inline auto pla_file::load_file
        (std::string_view path) -> pla_file
    {
        auto reader = utils::file_reader {std::string {path}};
        reader.throw_if_cant_read();

        auto const options = pla_impl::read_options(reader);
        if (!pla_impl::has_keys(options, {".i", ".o"}))
        {
            throw std::runtime_error {"Invalid pla header. '.i' and '.o' must be set."};
        }

        auto const varCount     = utils::parse_except<std::uint32_t>(options.at(".i"));
        auto const diagramCount = utils::parse_except<std::uint32_t>(options.at(".o"));
        auto const lineCount    = options.find(".p") != options.end()
                                    ? utils::parse_except<std::uint32_t>(options.at(".p"))
                                    : pla_impl::ncount;

        return pla_file { pla_impl::read_data(reader, varCount, diagramCount, lineCount)
                        , pla_impl::read_input_labels(options)
                        , pla_impl::read_output_labels(options) };
    }

    inline auto pla_file::save_to_file
        (std::string_view path, pla_file const& file) -> void
    {
        using utils::concat_range;

        auto ost = std::ofstream {std::filesystem::path {path}};

        ost << ".i "   << file.variable_count()                       << '\n';
        ost << ".o "   << file.function_count()                       << '\n';
        ost << ".ilb " << concat_range(file.get_input_labels(), " ")  << '\n';
        ost << ".ob "  << concat_range(file.get_output_labels(), " ") << '\n';
        ost << ".p "   << file.line_count()                           << '\n';

        for (auto const& line : file.lines_)
        {
            for (auto const var : line.cube)
            {
                ost << pla_impl::bool_t_to_char(var);
            }

            ost << ' ';

            for (auto const fval : line.fVals)
            {
                ost << pla_impl::bool_t_to_char(fval);
            }

            ost << '\n';
        }

        ost << ".e\n";
    }

    inline pla_file::pla_file
        ( pla_line_v lines
        , string_v   inputLabels
        , string_v   outputLabels ) :
        lines_        {std::move(lines)},
        inputLabels_  {std::move(inputLabels)},
        outputLabels_ {std::move(outputLabels)}
    {
    }

    inline auto pla_file::variable_count
        () const -> index_t
    {
        return static_cast<index_t>(lines_.front().cube.size());
    }

    inline auto pla_file::function_count
        () const -> index_t
    {
        return static_cast<index_t>(lines_.front().fVals.size());
    }

    inline auto pla_file::line_count
        () const -> index_t
    {
        return static_cast<index_t>(lines_.size());
    }

    inline auto pla_file::get_lines
        () const & -> pla_line_v const&
    {
        return lines_;
    }

    inline auto pla_file::get_lines
        () const && -> pla_line_v
    {
        return lines_;
    }

    inline auto pla_file::get_indices
        () const -> index_v
    {
        auto constexpr U = log_val_traits<2>::undefined;
        auto indices = std::set<index_t> {};

        for (auto const& line : lines_)
        {
            auto index = index_t {0};
            for (auto const& var : line.cube)
            {
                if (U != var)
                {
                    indices.insert(index);
                }
                ++index;
            }
        }

        return index_v {indices.begin(), indices.end()};
    }

    inline auto pla_file::get_input_labels
        () const & -> string_v const&
    {
        return inputLabels_;
    }

    inline auto pla_file::get_input_labels
        () const && -> string_v
    {
        return inputLabels_;
    }

    inline auto pla_file::get_output_labels
        () const & -> string_v const&
    {
        return outputLabels_;
    }

    inline auto pla_file::get_output_labels
        () const && -> string_v
    {
        return outputLabels_;
    }

    inline auto pla_file::swap_vars
        (size_t const i1, size_t const i2) -> void
    {
        using std::swap;

        for (auto& line : lines_)
        {
            swap(line.cube[i1], line.cube[i2]);
        }

        swap(inputLabels_[i1], inputLabels_[i2]);
    }
}

#endif