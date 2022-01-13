#ifndef MIX_DD_PLA_FILE_HPP
#define MIX_DD_PLA_FILE_HPP

// #include "../utils/string_utils.hpp"
// #include "../utils/parsing_utils.hpp"
// #include "../utils/file_reader.hpp"
// #include "../utils/more_vector.hpp"
// #include "../utils/more_iterator.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "types.hpp"
#include "utils.hpp"

namespace teddy
{
    /**
     *  @brief Bool cube.
     */
    class cube_t
    {
    public:
        inline static constexpr auto Undefined = std::uint8_t(0b11);

    public:
        cube_t (std::size_t);

        auto size () const              -> std::size_t;
        auto get  (std::size_t) const   -> unsigned int;
        auto set  (std::size_t, uint_t) -> void;

    private:
        struct byte
        {
            std::uint8_t b0 : 2;
            std::uint8_t b1 : 2;
            std::uint8_t b2 : 2;
            std::uint8_t b3 : 2;
        };

    private:
        std::size_t       size_;
        std::vector<byte> values_;
    };

    /**
     *  @brief Representation of a PLA file.
     */
    class pla_file
    {
    public:
        static auto load_file
            (std::string const&) -> std::optional<pla_file>;

    public:
        struct pla_line
        {
            cube_t cube;
            cube_t fVals;
        };

    public:
        auto variable_count    () const    -> std::size_t;
        auto function_count    () const    -> std::size_t;
        auto line_count        () const    -> std::size_t;
        auto get_lines         () const &  -> std::vector<pla_line> const&;
        auto get_lines         ()       && -> std::vector<pla_line>;
        auto get_input_labels  () const &  -> std::vector<std::string> const&;
        auto get_input_labels  ()       && -> std::vector<std::string>;
        auto get_output_labels () const &  -> std::vector<std::string> const&;
        auto get_output_labels ()       && -> std::vector<std::string>;

    private:
        pla_file ( std::vector<pla_line>    lines
                 , std::vector<std::string> inputLabels
                 , std::vector<std::string> outputLabels );

    private:
        std::vector<pla_line>    lines_;
        std::vector<std::string> inputLabels_;
        std::vector<std::string> outputLabels_;
    };

// cube_t definitions:

    inline cube_t::cube_t
        (std::size_t const size) :
        size_   (size),
        values_ (size / 4 + 1, byte {0, 0, 0, 0})
    {
    }

    inline auto cube_t::size
        () const -> std::size_t
    {
        return size_;
    }

    inline auto cube_t::get
        (std::size_t const i) const -> unsigned int
    {
        assert(i < size_);
        switch (i % 4)
        {
            case 0: return values_[i / 4].b0;
            case 1: return values_[i / 4].b1;
            case 2: return values_[i / 4].b2;
            case 3: return values_[i / 4].b3;
        }
        return static_cast<uint_t>(-1);
    }

    inline auto cube_t::set
        (std::size_t const i, uint_t const val) -> void
    {
        assert(i < size_);
        assert(val < 4);
        switch (i % 4)
        {
            case 0: values_[i / 4].b0 = val & 0b11; break;
            case 1: values_[i / 4].b1 = val & 0b11; break;
            case 2: values_[i / 4].b2 = val & 0b11; break;
            case 3: values_[i / 4].b3 = val & 0b11; break;
        }
    }

// pla_file definitions:

    inline auto pla_file::load_file
        (std::string const& path) -> std::optional<pla_file>
    {
        namespace rs = std::ranges;

        // Utils:
        auto constexpr space = [](auto const c)
        {
            return std::isspace(c);
        };

        auto constexpr words = [space](auto const str)
        {
            auto ws        = std::vector<std::string>();
            auto it        = rs::begin(str);
            auto const end = rs::end(str);

            while (it != end)
            {
                auto const wordBegin = rs::find_if_not(it, end, space);
                auto const wordEnd   = rs::find_if(wordBegin, end, space);
                if (wordBegin != wordEnd)
                {
                    ws.emplace_back(wordBegin, wordEnd);
                }
                it = wordEnd;
            }

            return ws;
        };

        auto ifst = std::ifstream(path);
        if (not ifst.is_open())
        {
            return std::nullopt;
        }

        // Read options.
        auto options = std::unordered_map<std::string, std::string>();
        auto line    = std::string();
        while (std::getline(ifst, line))
        {
            // Skip leading spaces.
            auto const first = rs::find_if_not(line, space);
            auto const last  = std::end(line);
            if (first == last)
            {
                // Skip empty line.
                continue;
            }

            if (*first == '#')
            {
                // Skip comment.
                continue;
            }

            if (*first != '.')
            {
                // Not an option.
                break;
            }

            // Split into (key, val) pair on the first space.
            auto const keyLast  = rs::find_if(first, last, space);
            auto const valFirst = keyLast == last
                ? last
                : rs::find_if_not(keyLast + 1, last, space);
            auto key = std::string(first, keyLast);
            if (valFirst != last)
            {
                auto valLast = last;
                while (space(*(valLast - 1)))
                {
                    --valLast;
                }
                auto val = std::string(valFirst, valLast);
                options.emplace(std::move(key), std::move(val));
            }
            else
            {
                options.emplace(std::move(key), std::string());
            }
        }

        // Parse header.
        auto const optionsEnd  = rs::end(options);
        auto const varCountIt  = options.find(".i");
        auto const fCountIt    = options.find(".o");
        auto const lineCountIt = options.find(".p");
        if ( varCountIt  == optionsEnd
          or fCountIt    == optionsEnd
          or lineCountIt == optionsEnd )
        {
            return std::nullopt;
        }

        auto const varCount  = utils::parse<std::size_t>(varCountIt->second);
        auto const fCount    = utils::parse<std::size_t>(fCountIt->second);
        auto const lineCount = utils::parse<std::size_t>(lineCountIt->second);

        if (not varCount or not fCount or not lineCount)
        {
            return std::nullopt;
        }

        // Read data.
        auto lines = std::vector<pla_file::pla_line>();
        lines.reserve(*lineCount);
        do
        {
            auto const first = rs::find_if_not(line, space);
            auto const last  = rs::end(line);
            if (first == last)
            {
                // Skip empty line.
                continue;
            }

            if (*first == '.')
            {
                // This can only be the .e line.
                break;
            }

            // Split on the first space.
            auto const varsLast = rs::find_if(first, last, space);
            if (varsLast == last)
            {
                return std::nullopt;
            }
            auto const fsFirst = rs::find_if_not(varsLast + 1, last, space);
            auto const fsLast  = rs::find_if(fsFirst, last, space);
            auto const vars    = std::string(first, varsLast);
            auto const fs      = std::string(fsFirst, fsLast);

            if (vars.size() != *varCount or fs.size() != *fCount)
            {
                return std::nullopt;
            }

            // Parse variables into cube.
            auto variables = cube_t(*varCount);
            for (auto i = 0ul; i < *varCount; ++i)
            {
                switch (vars[i])
                {
                    case '0': variables.set(i, 0u); break;
                    case '1': variables.set(i, 1u); break;
                    case '~':
                    case '-': variables.set(i, cube_t::Undefined); break;
                    default:  return std::nullopt;
                }
            }

            // Parse functions into cube.
            auto functions = cube_t(*fCount);
            for (auto i = 0u; i < *fCount; ++i)
            {
                switch (fs[i])
                {
                    case '0': functions.set(i, 0u); break;
                    case '1': functions.set(i, 1u); break;
                    case '-':
                    case '~': functions.set(i, cube_t::Undefined); break;
                    default:  return std::nullopt;
                }
            }

            lines.push_back({std::move(variables), std::move(functions)});

        } while (std::getline(ifst, line));

        // Read labels.
        auto const inLbIt = options.find(".ilb");
        auto const ouLbIt = options.find(".ob");
        auto inputLabels  = inLbIt != rs::end(options)
            ? words(inLbIt->second)
            : std::vector<std::string>();
        auto outputLabels = ouLbIt != rs::end(options)
            ? words(ouLbIt->second)
            : std::vector<std::string>();

        return pla_file( std::move(lines)
                       , std::move(inputLabels)
                       , std::move(outputLabels) );
    }

    inline pla_file::pla_file
        ( std::vector<pla_line>    lines
        , std::vector<std::string> inputLabels
        , std::vector<std::string> outputLabels ) :
        lines_        (std::move(lines)),
        inputLabels_  (std::move(inputLabels)),
        outputLabels_ (std::move(outputLabels))
    {
    }

    inline auto pla_file::variable_count
        () const -> std::size_t
    {
        return static_cast<index_t>(lines_.front().cube.size());
    }

    inline auto pla_file::function_count
        () const -> std::size_t
    {
        return static_cast<index_t>(lines_.front().fVals.size());
    }

    inline auto pla_file::line_count
        () const -> std::size_t
    {
        return static_cast<index_t>(lines_.size());
    }

    inline auto pla_file::get_lines
        () const & -> std::vector<pla_line> const&
    {
        return lines_;
    }

    inline auto pla_file::get_lines
        () && -> std::vector<pla_line>
    {
        return std::move(lines_);
    }

    inline auto pla_file::get_input_labels
        () const & -> std::vector<std::string> const&
    {
        return inputLabels_;
    }

    inline auto pla_file::get_input_labels
        () && -> std::vector<std::string>
    {
        return std::move(inputLabels_);
    }

    inline auto pla_file::get_output_labels
        () const & -> std::vector<std::string> const&
    {
        return outputLabels_;
    }

    inline auto pla_file::get_output_labels
        () && -> std::vector<std::string>
    {
        return std::move(outputLabels_);
    }
}

#endif