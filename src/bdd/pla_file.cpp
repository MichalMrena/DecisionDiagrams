#include "pla_file.hpp"

#include <fstream>

#include <map>
#include <set>
#include <algorithm>
#include <limits>
#include "../utils/string_utils.hpp"
#include "../utils/parsing_utils.hpp"
#include "../utils/file_reader.hpp"
#include "../data_structures/bit_vector.hpp"

namespace mix::dd
{
    using option_map = std::map<const std::string, std::string>;

    namespace
    {
        constexpr auto ncount {std::numeric_limits<uint32_t>::max()};

        auto char_to_bool_t (const char c) -> bool_t
        {
            switch (c)
            {
                case '0': return 0;
                case '1': return 1;
                case '-': return X;
                case '~': return X;
                default:
                    throw std::runtime_error {"Invalid pla line. Unknown variable value."};
            }
        }

        auto bool_t_to_char (const bool_t b) -> char
        {
            switch (b)
            {
                case 0:  return '0';
                case 1:  return '1';
                default: return '-';
            }
        }

        auto is_option_line (const std::string& line) -> bool
        {
            return ! line.empty() && '.' == line.at(0);
        }

        auto is_comment_line (const std::string& line) -> bool
        {
            return '#' == line.at(0);
        }

        auto read_options (utils::file_reader& reader) -> option_map
        {
            option_map options;

            for (;;)
            {
                const auto keyVal 
                {
                    utils::to_head_tail(utils::shrink_spaces(utils::trim(reader.peek_line_except())))
                };

                if (keyVal.first.empty() || is_comment_line(keyVal.first))
                {
                    reader.read_line_except();
                    continue;
                }

                if (! is_option_line(keyVal.first))
                {
                    break;
                }

                options[keyVal.first] = keyVal.second;
                
                reader.read_line_except();
            }
            
            return options;
        }

        auto has_keys ( const option_map& options
                      , std::initializer_list<const std::string> keys ) -> bool
        {
            for (const auto& key : keys)
            {
                if (options.find(key) == options.end())
                {
                    return false;
                }
            }

            return true;
        }

        auto read_input_labels (const option_map& options) -> std::vector<std::string>
        {
            const auto inputCount {utils::parse_except<uint32_t>(options.at(".i"))};
            const auto labelsIt   {options.find(".ilb")};

            if (labelsIt != options.end())
            {
                return utils::to_words(std::move((*labelsIt).second));
            }
            
            std::vector<std::string> labels;
            labels.reserve(inputCount);

            for (size_t i {0}; i < inputCount; ++i)
            {
                labels.emplace_back("x" + std::to_string(i));
            }

            return labels;
        }

        auto read_output_labels (const option_map& options) -> std::vector<std::string>
        {
            const auto outputCount {utils::parse_except<uint32_t>(options.at(".o"))};
            const auto labelsIt    {options.find(".ob")};

            if (labelsIt != options.end())
            {
                return utils::to_words(std::move((*labelsIt).second));
            }
            
            std::vector<std::string> labels;
            labels.reserve(outputCount);

            for (size_t i {0}; i < outputCount; ++i)
            {
                labels.emplace_back("y" + std::to_string(i));
            }

            return labels;
        }

        auto read_data ( utils::file_reader& reader
                       , const size_t        varCount    
                       , const size_t        diagramCount
                       , const size_t        lineCount ) -> std::vector<pla_line>
        {
            std::vector<pla_line> lines;
            if (ncount != lineCount)
            {
                lines.reserve(lineCount);
            }

            do
            {
                const auto lineWords 
                {
                    utils::to_head_tail(utils::trim(utils::shrink_spaces(reader.read_line_except())))
                };

                if (lineWords.first.empty() || is_comment_line(lineWords.first))
                {
                    continue;
                }

                if (lineWords.second.empty())
                {
                    throw std::runtime_error {"Invalid pla line. Expected function definition."};
                }

                const std::string variablesStr {std::move(lineWords.first)};
                const std::string valuesStr    {std::move(lineWords.second)};

                if (variablesStr.size() != varCount || valuesStr.size() != diagramCount)
                {
                    throw std::runtime_error {"Invalid pla line. Expected function definition."};
                }

                bit_vector<2, bool_t> variables (varCount);
                for (const auto c : variablesStr)
                {
                    variables.push_back(char_to_bool_t(c));
                }

                bit_vector<2, bool_t> values (diagramCount);
                for (const auto c : valuesStr)
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
    
    auto swap (pla_line& lhs, pla_line& rhs) -> void
    {
        using std::swap;
        swap(lhs.cube, rhs.cube);
        swap(lhs.fVals, rhs.fVals);
    }

    auto operator== (const pla_line& lhs, const pla_line& rhs) -> bool
    {
        return (lhs.fVals == rhs.fVals) && (lhs.cube == rhs.cube);
    }

    auto operator!= (const pla_line& lhs, const pla_line& rhs) -> bool
    {
        return ! (lhs == rhs);
    }

    auto operator== (const pla_file& lhs, const pla_file& rhs) -> bool
    {
        return lhs.get_lines() == rhs.get_lines();
    }

    auto operator!= (const pla_file& lhs, const pla_file& rhs) -> bool
    {
        return ! (lhs == rhs);
    }

    auto pla_file::load_from_file 
        (const std::string& filePath) -> pla_file
    {
        utils::file_reader reader {filePath};
        reader.throw_if_cant_read();

        const auto options {read_options(reader)};
        
        if (! has_keys(options, {".i", ".o"}))
        {
            throw std::runtime_error {"Invalid pla header. '.i' and '.o' must be set."};
        }
        
        const auto varCount     {utils::parse_except<uint32_t>(options.at(".i"))};
        const auto diagramCount {utils::parse_except<uint32_t>(options.at(".o"))};
        const auto lineCount
        {
            options.find(".p") != options.end()
                ? utils::parse_except<uint32_t>(options.at(".p"))
                : ncount
        };

        return pla_file 
        {
            read_data(reader, varCount, diagramCount, lineCount)
          , read_input_labels(options)
          , read_output_labels(options)
        };
    }

    auto pla_file::save_to_file
        (const std::string& filePath, const pla_file& file) -> void
    {
        using utils::concat_range;

        std::ofstream ost {filePath};

        ost << ".i "   << file.variable_count()                       << '\n';
        ost << ".o "   << file.function_count()                       << '\n';
        ost << ".ilb " << concat_range(file.get_input_labels(), " ")  << '\n';
        ost << ".ob "  << concat_range(file.get_output_labels(), " ") << '\n';
        ost << ".p "   << file.line_count()                           << '\n';

        for (const auto& line : file.lines_)
        {
            for (const auto var : line.cube)
            {
                ost << bool_t_to_char(var);
            }

            ost << ' ';

            for (const auto fval : line.fVals)
            {
                ost << bool_t_to_char(fval);
            }

            ost << '\n';
        }

        ost << ".e\n";
    }

    pla_file::pla_file( std::vector<pla_line> pLines
                      , std::vector<std::string> pInputLabels
                      , std::vector<std::string> pOutputLabels ) :
        lines_        {std::move(pLines)}
      , inputLabels_  {pInputLabels}
      , outputLabels_ {pOutputLabels}
    {
    }

    auto pla_file::variable_count
        () const -> int32_t
    {
        return this->lines_.front().cube.size();
    }

    auto pla_file::function_count
        () const -> int32_t
    {
        return this->lines_.front().fVals.size();
    }

    auto pla_file::line_count
        () const -> int32_t
    {
        return static_cast<int32_t>(this->lines_.size());
    }

    auto pla_file::get_lines
        () const -> const std::vector<pla_line>&
    {
        return this->lines_;
    }

    auto pla_file::get_indices
        () const -> std::vector<index_t>
    {
        std::set<index_t> indices;

        for (const auto& line : this->lines_)
        {
            index_t index {0};
            for (const auto& var : line.cube)
            {
                if (X != var)
                {
                    indices.insert(index);
                }
                ++index;
            }
        }

        return std::vector<index_t> {indices.begin(), indices.end()};
    }

    auto pla_file::get_input_labels
        () const -> const std::vector<std::string>&
    {
        return this->inputLabels_;
    }

    auto pla_file::get_output_labels
        () const -> const std::vector<std::string>&
    {
        return this->outputLabels_;
    }

    auto pla_file::swap_vars
        (const size_t i1, const size_t i2) -> void
    {
        using std::swap;
        
        for (auto& line : this->lines_)
        {
            swap(line.cube[i1], line.cube[i2]);
        }

        swap(inputLabels_[i1], inputLabels_[i2]);
    }
}