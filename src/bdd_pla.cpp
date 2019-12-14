#include "bdd_pla.hpp"

#include <fstream>
#include "utils/file_reader.hpp"
#include "utils/string_utils.hpp"
#include "utils/parsing_utils.hpp"

namespace mix::dd
{
    auto pla_file::read 
        (const std::string& filePath) -> pla_file
    {
        utils::file_reader ifst {filePath};
        ifst.throw_if_cant_read();

    // TODO lepšie rozparsovať
        const auto iWords {utils::to_words(ifst.next_line_except())};
        const auto oWords {utils::to_words(ifst.next_line_except())};
        const auto pWords {utils::to_words(ifst.next_line_except())};

        if (iWords.size() < 2 || oWords.size() < 2 || pWords.size() < 2)
        {
            throw std::runtime_error {"Invalid pla header format."};
        }

        const auto varCount     {utils::parse_except<uint32_t>(iWords[1])};
        const auto diagramCount {utils::parse_except<uint32_t>(oWords[1])};
        const auto lineCount    {utils::parse_except<uint32_t>(pWords[1])};

        std::vector<pla_line> lines;
        lines.reserve(lineCount);
        for (size_t row {0}; row < lineCount; ++row)
        {
            const auto lineWords {utils::to_words(ifst.next_line_except())};
            if (lineWords.size() < 2)
            {
                throw std::runtime_error {"Invalid pla line."};
            }

            const std::string variablesStr {std::move(lineWords[0])};
            const std::string valuesStr    {std::move(lineWords[1])};

            if (variablesStr.size() != varCount || valuesStr.size() != diagramCount)
            {
                throw std::runtime_error {"Invalid pla line."};
            }

            std::vector<log_val_t> variables;
            variables.reserve(varCount);
            for (const auto c : variablesStr)
            {
                variables.push_back(pla_file::char_to_log_val(c));
            }

            std::vector<log_val_t> values;
            values.reserve(diagramCount);
            for (const auto c : valuesStr)
            {
                values.push_back(pla_file::char_to_log_val(c));
            }

            lines.push_back({std::move(variables), std::move(values)});
        }

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

    auto pla_file::char_to_log_val 
        (const char c) -> log_val_t
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

    pla_file::pla_file(std::vector<pla_line>&& pLines) :
        lines {std::move(pLines)}
    {
    }
}
