#include "pla_file.hpp"

#include <map>
#include <algorithm>
#include "utils/string_utils.hpp"
#include "utils/parsing_utils.hpp"
#include "utils/file_reader.hpp"

namespace mix::dd
{
// auxiliary parsing functions:

    using option_map = std::map<std::string, std::string>;

    static auto char_to_log_val (const char c) -> log_val_t
    {
        switch (c)
        {
        case '0': return 0;
        case '1': return 1;
        case '-': return X;
        default:
            throw std::runtime_error {"Invalid pla line. Unknown variable value."};
        }
    }

    static auto is_option_line (const std::string& line) -> bool
    {
        return ! line.empty() && '.' == line[0];
    }

    static auto read_options (utils::file_reader& reader) -> option_map
    {
        std::map<std::string, std::string> options;

        for (;;)
        {
            const auto keyVal 
            {
                utils::to_head_tail(utils::shrink_spaces(utils::trim(reader.peek_line_except())))
            };

            if (! is_option_line(keyVal.first))
            {
                break;
            }

            options[keyVal.first] = keyVal.second;
            
            reader.read_line_except();
        }
        
        return options;
    }

    static auto has_keys ( const option_map& map
                         , std::initializer_list<std::string> keys) -> bool
    {
        for (const auto k : keys)
        {
            if (map.find(k) == map.end())
            {
                return false;
            }
        }

        return true;
    }

    static auto read_data ( utils::file_reader& reader
                          , const uint32_t varCount    
                          , const uint32_t diagramCount
                          , const uint32_t lineCount) -> std::vector<pla_line>
    {
        std::vector<pla_line> lines;
        lines.reserve(lineCount);

        for (size_t row {0}; row < lineCount; ++row)
        {
            const auto lineWords 
            {
                utils::to_head_tail(reader.read_line_except())
            };

            if (lineWords.first.empty() || lineWords.second.empty())
            {
                throw std::runtime_error {"Invalid pla line: " + std::to_string(row)};
            }

            const std::string variablesStr {std::move(lineWords.first)};
            const std::string valuesStr    {std::move(lineWords.second)};

            if (variablesStr.size() != varCount || valuesStr.size() != diagramCount)
            {
                throw std::runtime_error {"Invalid pla line: " + std::to_string(row)};
            }

            std::vector<log_val_t> variables;
            variables.reserve(varCount);
            for (const auto c : variablesStr)
            {
                variables.push_back(char_to_log_val(c));
            }

            std::vector<log_val_t> values;
            values.reserve(diagramCount);
            for (const auto c : valuesStr)
            {
                values.push_back(char_to_log_val(c));
            }

            lines.push_back({std::move(variables), std::move(values)});
        }

        return lines;
    }

// public pla_file implementation:

    auto pla_file::read 
        (const std::string& filePath) -> pla_file
    {
        utils::file_reader reader {filePath};
        reader.throw_if_cant_read();

        const auto options {read_options(reader)};

        if (! has_keys(options, {"i", "o", "p"}))
        {
            throw std::runtime_error {"Invalid pla header format."};
        }

        const auto varCount     {utils::parse_except<uint32_t>(options.at("i"))};
        const auto diagramCount {utils::parse_except<uint32_t>(options.at("o"))};
        const auto lineCount    {utils::parse_except<uint32_t>(options.at("p"))};

        auto lines {read_data(reader, varCount, diagramCount, lineCount)};

        return pla_file {std::move(lines)};
    }

    auto pla_file::variable_count
        () const -> int32_t
    {
        return this->lines.front().varVals.size();
    }

    auto pla_file::function_count
        () const -> int32_t
    {
        return this->lines.front().fVals.size();
    }

    auto pla_file::line_count
        () const -> int32_t
    {
        return static_cast<int32_t>(this->lines.size());
    }

    auto pla_file::get_lines
        () const -> const std::vector<pla_line>&
    {
        return this->lines;
    }

// private constructor:

    pla_file::pla_file(std::vector<pla_line> pLines) :
        lines {std::move(pLines)}
    {
    }
}