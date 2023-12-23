#ifndef LIBTEDDY_DETAILS_PLA_FILE_HPP
#define LIBTEDDY_DETAILS_PLA_FILE_HPP

#include <libteddy/details/debug.hpp>
#include <libteddy/details/tools.hpp>
#include <libteddy/details/types.hpp>

#include <cassert>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace teddy
{
namespace utils
{
    /**
    *  \brief Finds the first element satisfying \p test
    *  Implementation of std::find_if
    */
    template<class It, class Predicate>
    auto constexpr find_if (It first, It const last, Predicate test) -> It
    {
        while (first != last)
        {
            if (test(*first))
            {
                return first;
            }
            ++first;
        }
        return last;
    }

    // TODO move to pla input
    /**
    *  \brief Finds the first element not satisfying \p test
    *  Implementation of std::find_if_not
    */
    template<class It, class Predicate>
    auto constexpr find_if_not (It first, It const last, Predicate test) -> It
    {
        while (first != last)
        {
            if (not test(*first))
            {
                return first;
            }
            ++first;
        }
        return last;
    }

    /**
    *  \brief Tries to parse \p input to \p Num
    *  \param input input string
    *  \return optinal result
    */
    template<class Num>
    auto parse (std::string_view const input) -> std::optional<Num>
    {
        auto ret = Num {};
        auto result
            = std::from_chars(input.data(), input.data() + input.size(), ret);
        return std::errc {} == result.ec
                    && result.ptr == input.data() + input.size()
                ? std::optional<Num>(ret)
                : std::nullopt;
    }
}

/**
 *  \brief Bool cube.
 */
class bool_cube
{
public:
    static constexpr std::uint8_t DontCare = 0b11;

public:
    bool_cube(int32 size) :
        size_(size),
        values_(as_usize(size / 4 + 1), byte {0, 0, 0, 0})
    {
    }

    auto size () const -> int32
    {
        return size_;
    }

    auto get (int32 index) const -> int32
    {
        int32 const byteIndex = index / 4;

        assert(byteIndex >= 0 && byteIndex < ssize(values_));

        switch (index % 4)
        {
        case 0:
            return values_[as_uindex(byteIndex)].b0;
        case 1:
            return values_[as_uindex(byteIndex)].b1;
        case 2:
            return values_[as_uindex(byteIndex)].b2;
        case 3:
            return values_[as_uindex(byteIndex)].b3;
        }
        return -1;
    }

    auto set (int32 index, int32 value) -> void
    {
        int32 const byteIndex = index / 4;

        assert((byteIndex >= 0 && byteIndex < ssize(values_)));
        assert(value == 0 || value == 1 || value == DontCare);

        switch (index % 4)
        {
        case 0:
            values_[as_uindex(byteIndex)].b0 = value & 0b11;
            break;
        case 1:
            values_[as_uindex(byteIndex)].b1 = value & 0b11;
            break;
        case 2:
            values_[as_uindex(byteIndex)].b2 = value & 0b11;
            break;
        case 3:
            values_[as_uindex(byteIndex)].b3 = value & 0b11;
            break;
        }
    }

private:
    struct byte
    {
        std::uint8_t b0 : 2;
        std::uint8_t b1 : 2;
        std::uint8_t b2 : 2;
        std::uint8_t b3 : 2;
    };

private:
    int32 size_;
    std::vector<byte> values_;
};

/**
 *  \brief Representation of a PLA file
 */
class pla_file
{
public:
    /**
     *  \brief Loads PLA file from a file at given path
     *  \param path Path to the file
     *  \param verbose Enables logging
     *  \return Optional holding instance of \c pla_file or
     *  \c std::nullopt if the loading failed
     */
    static auto load_file (
        std::string const& path,
        bool const verbose = false
    ) -> std::optional<pla_file>;

public:
    /**
     *  \brief Represents one line of a PLA file.
     */
    struct pla_line
    {
        bool_cube cube_;
        bool_cube fVals_;
    };

public:
    /**
     *  \brief Returns the number of variables in the file
     *  \return Number of variables
     */
    [[nodiscard]]
    auto get_variable_count () const -> int32
    {
        return lines_.front().cube_.size();
    }

    /**
     *  \brief Returns number of functions in the file
     *  \return Number of functions
     */
    [[nodiscard]]
    auto get_function_count () const -> int32
    {
        return lines_.front().fVals_.size();
    }

    /**
     *  \brief Returns the number of lines in the file
     *
     *  Does not count headers and comments, only lines containing cubes
     *
     *  \return Number of lines
     */
    [[nodiscard]]
    auto get_line_count () const -> int64
    {
        return static_cast<int64>(lines_.size());
    }

    /**
     *  \brief Returns reference to the vector holding lines
     *  \return Reference to the vector
     */
    [[nodiscard]] auto get_lines () const& -> std::vector<pla_line> const&
    {
        return lines_;
    }

    /**
     *  \brief Returns copy of the vector holding lines
     *  \return Copy of the vector
     */
    [[nodiscard]]
    auto get_lines () && -> std::vector<pla_line>
    {
        return lines_;
    }

    /**
     *  \brief Returns reference to the vector holding input labels
     *  \return Reference to the vector
     */
    [[nodiscard]]
    auto get_input_labels () const& -> std::vector<std::string> const&
    {
        return inputLabels_;
    }

    /**
     *  \brief Returns copy of the vector holding input labels
     *  \return Copy of the vector
     */
    [[nodiscard]]
    auto get_input_labels () && -> std::vector<std::string>
    {
        return inputLabels_;
    }

    /**
     *  \brief Returns reference to the vector holding function labels
     *  \return Reference to the vector
     */
    [[nodiscard]]
    auto get_output_labels () const& -> std::vector<std::string> const&
    {
        return outputLabels_;
    }

    /**
     *  \brief Returns copy of the vector holding function labels
     *  \return Copy the vector
     */
    [[nodiscard]]
    auto get_output_labels () && -> std::vector<std::string>
    {
        return outputLabels_;
    }

private:
    pla_file(
        std::vector<pla_line> lines,
        std::vector<std::string> inputLabels,
        std::vector<std::string> outputLabels
    ) :
        lines_(TEDDY_MOVE(lines)),
        inputLabels_(TEDDY_MOVE(inputLabels)),
        outputLabels_(TEDDY_MOVE(outputLabels))
    {
    }

private:
    std::vector<pla_line> lines_;
    std::vector<std::string> inputLabels_;
    std::vector<std::string> outputLabels_;
};

// pla_file definitions:

inline auto pla_file::load_file(
    std::string const& path,
    bool const verbose
) -> std::optional<pla_file>
{
    auto const to_words =
        [] (std::string const& str) -> std::vector<std::string>
    {
        std::vector<std::string> words;
        auto strIt       = str.begin();
        auto const endIt = str.end();

        while (strIt != endIt)
        {
            auto const wordBegin = utils::find_if_not(strIt, endIt, ::isspace);
            auto const wordEnd   = utils::find_if(wordBegin, endIt, ::isspace);
            if (wordBegin != wordEnd)
            {
                words.emplace_back(wordBegin, wordEnd);
            }
            strIt = wordEnd;
        }

        return words;
    };

    int32 lineNum = 0;
    std::ifstream ifst(path);
    if (not ifst.is_open())
    {
        if (verbose)
        {
            std::cerr << "Failed to open: " << path << "\n";
        }
        return std::nullopt;
    }

    // Read options.
    std::unordered_map<std::string, std::string> options;
    std::string line;
    while (std::getline(ifst, line))
    {
        ++lineNum;
        // Skip leading spaces.
        auto const first
            = utils::find_if_not(line.begin(), line.end(), ::isspace);
        auto const last = line.end();
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
        auto const keyLast = utils::find_if(first, last, ::isspace);
        auto const valFirst
            = keyLast == last
                ? last
                : utils::find_if_not(keyLast + 1, last, ::isspace);
        std::string key(first, keyLast);
        if (valFirst != last)
        {
            // Option with argument
            auto valLast = last;
            while (valLast != valFirst && ::isspace(*(valLast - 1)))
            {
                --valLast;
            }
            std::string val(valFirst, valLast);
            options.emplace(TEDDY_MOVE(key), TEDDY_MOVE(val));
        }
        else
        {
            // Option with no argument
            options.emplace(TEDDY_MOVE(key), std::string());
        }
    }

    // Parse header.
    auto const varCountIt  = options.find(".i");
    auto const fCountIt    = options.find(".o");
    auto const lineCountIt = options.find(".p");
    if (varCountIt == options.end())
    {
        if (verbose)
        {
            std::cerr << "Missing input count option .i" << "\n";
        }
        return std::nullopt;
    }

    if (fCountIt == options.end())
    {
        if (verbose)
        {
            std::cerr << "Missing function count option .o" << "\n";
        }
        return std::nullopt;
    }

    if (lineCountIt == options.end() && verbose)
    {
        std::cerr << "Missing line count option .p" << "\n";
    }

    std::optional<int32> varCount  = utils::parse<int32>(varCountIt->second);
    std::optional<int32> fCount    = utils::parse<int32>(fCountIt->second);
    std::optional<int32> lineCount = std::nullopt;

    if (not varCount)
    {
        if (verbose)
        {
            std::cerr << "Failed to parse input count: "
                      << varCountIt->second
                      << "\n";
        }
        return std::nullopt;
    }

    if (not fCount)
    {
        if (verbose)
        {
            std::cerr << "Failed to parse output count: "
                      << fCountIt->second
                      << "\n";
        }
        return std::nullopt;
    }

    if (lineCountIt != options.end())
    {
        lineCount = utils::parse<int32>(fCountIt->second);
        if (not lineCount)
        {
            std::cerr << "Failed to parse line count: "
                      << lineCountIt->second
                      << "\n";
        }
    }

    // Read data.
    std::vector<pla_file::pla_line> lines;
    if (lineCount)
    {
        lines.reserve(as_usize(*lineCount));
    }

    do
    {
        ++lineNum;
        auto const first
            = utils::find_if_not(line.begin(), line.end(), ::isspace);
        auto const last = line.end();
        if (first == last)
        {
            // Skip empty line.
            continue;
        }

        if (*first == '.')
        {
            // This can only be the .e line.
            continue;
        }

        if (*first == '#')
        {
            // Skip comment.
            continue;
        }

        // Split on the first space.
        auto const varsLast = utils::find_if(first, last, ::isspace);
        if (varsLast == last)
        {
            if (verbose)
            {
                std::cerr << "Missing output values on line "
                          << (lineNum - 1)
                          << "\n";
            }
            return std::nullopt;
        }
        auto const fsFirst = utils::find_if_not(varsLast + 1, last, ::isspace);
        auto const fsLast  = utils::find_if(fsFirst, last, ::isspace);
        std::string const varsStr(first, varsLast);
        std::string const fStr(fsFirst, fsLast);

        if (ssize(varsStr) != *varCount)
        {
            if (verbose)
            {
                std::cerr << "Invalid input count on line "
                          << (lineNum - 1)
                          << "\n";
            }
            return std::nullopt;
        }

        if (ssize(fStr) != *fCount)
        {
            if (verbose)
            {
                std::cerr << "Invalid output count on line "
                          << (lineNum - 1)
                          << "\n";
            }
            return std::nullopt;
        }

        // Parse variables into cube.
        bool_cube variables(*varCount);
        for (int32 i = 0; i < *varCount; ++i)
        {
            switch (varsStr[as_uindex(i)])
            {
            case '0':
                variables.set(i, 0);
                break;
            case '1':
                variables.set(i, 1);
                break;
            case '~':
            case '-':
                variables.set(i, bool_cube::DontCare);
                break;
            default:
                if (verbose)
                {
                    std::cerr << "Invalid cube character '"
                              << varsStr[as_uindex(i)]
                              << "' on line "
                              << lineNum
                              << "\n";
                }
                return std::nullopt;
            }
        }

        // Parse functions into cube.
        bool_cube functions(*fCount);
        for (auto i = 0; i < *fCount; ++i)
        {
            switch (fStr[as_uindex(i)])
            {
            case '0':
                functions.set(i, 0);
                break;
            case '1':
                functions.set(i, 1);
                break;
            case '-':
            case '~':
                functions.set(i, bool_cube::DontCare);
                break;
            default:
                if (verbose)
                {
                    std::cerr << "Invalid cube character '"
                              << varsStr[as_uindex(i)]
                              << "' on line "
                              << lineNum
                              << "\n";
                }
                return std::nullopt;
            }
        }

        lines.push_back(pla_file::pla_line {variables, functions});

    } while (std::getline(ifst, line));

    // Read labels.
    auto const inLbIt = options.find(".ilb");
    auto const ouLbIt = options.find(".ob");
    std::vector<std::string> inputLabels;
    if (inLbIt != options.end())
    {
        inputLabels = to_words(inLbIt->second);
    }

    std::vector<std::string> outputLabels;
    if (ouLbIt != options.end())
    {
        outputLabels = to_words(ouLbIt->second);
    }

    return pla_file(
        TEDDY_MOVE(lines),
        TEDDY_MOVE(inputLabels),
        TEDDY_MOVE(outputLabels)
    );
}

} // namespace teddy

#endif