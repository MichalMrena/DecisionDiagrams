#ifndef LIBTEDDY_DETAILS_PLA_FILE_HPP
#define LIBTEDDY_DETAILS_PLA_FILE_HPP

#include <libteddy/details/debug.hpp>
#include <libteddy/details/tools.hpp>
#include <libteddy/details/types.hpp>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace teddy
{
/**
 *  \brief Bool cube.
 */
class bool_cube
{
public:
    inline static constexpr auto DontCare = std::uint8_t(0b11);

public:
    bool_cube(int32 size);

    auto size () const -> int32;
    auto get (int32 index) const -> int32;
    auto set (int32 index, int32 value) -> void;

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
     *  \return Optional holding instance of \c pla_file or
     *  \c std::nullopt if the loading failed
     */
    static auto load_file (std::string const& path) -> std::optional<pla_file>;

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
     *  \brief Returns number of variables in the file
     *  \return Number of variables
     */
    [[nodiscard]] auto variable_count () const -> int32;

    /**
     *  \brief Returns number of functions in the file
     *  \return Number of functions
     */
    [[nodiscard]] auto function_count () const -> int32;

    /**
     *  \brief Returns number of lines in the file
     *
     *  Does not count headers and comments, only lines containing cubes
     *
     *  \return Number of lines
     */
    [[nodiscard]] auto get_line_count () const -> int64;

    /**
     *  \brief Returns reference to a vector holding lines
     *  \return Reference to a vector
     */
    [[nodiscard]] auto get_lines () const& -> std::vector<pla_line> const&;

    /**
     *  \brief Returns copy of a vector holding lines
     *  \return Copy of a vector
     */
    [[nodiscard]] auto get_lines () && -> std::vector<pla_line>;

    /**
     *  \brief Returns reference to a vector holding
     *  labels of input variables
     *  \return Reference to a vector
     */
    [[nodiscard]] auto get_input_labels () const& -> std::vector<std::string> const&;

    /**
     *  \brief Returns copy of a vector holding
     *  labels of input variables
     *  \return Copy of a vector
     */
    [[nodiscard]] auto get_input_labels () && -> std::vector<std::string>;

    /**
     *  \brief Return reference to a vector holding labels of functions
     *  \return Reference to a vector
     */
    [[nodiscard]] auto get_output_labels () const& -> std::vector<std::string> const&;

    /**
     *  \brief Return copy of a vector holding labels of functions.
     *  \return Copy a vector
     */
    [[nodiscard]] auto get_output_labels () && -> std::vector<std::string>;

private:
    pla_file(
        std::vector<pla_line> lines,
        std::vector<std::string> inputLabels,
        std::vector<std::string> outputLabels
    );

private:
    std::vector<pla_line> lines_;
    std::vector<std::string> inputLabels_;
    std::vector<std::string> outputLabels_;
};

// bool_cube definitions:

inline bool_cube::bool_cube(int32 const size) :
    size_(size),
    values_(as_usize(size / 4 + 1), byte {0, 0, 0, 0})
{
}

inline auto bool_cube::size() const -> int32
{
    return size_;
}

inline auto bool_cube::get(int32 const i) const -> int32
{
    auto const byteIndex = i / 4;
    auto const uByteIndex = as_uindex(byteIndex);

    assert(byteIndex >= 0 && byteIndex < ssize(values_));

    switch (i % 4)
    {
    case 0:
        return values_[uByteIndex].b0;
    case 1:
        return values_[uByteIndex].b1;
    case 2:
        return values_[uByteIndex].b2;
    case 3:
        return values_[uByteIndex].b3;
    }
    return -1;
}

inline auto bool_cube::set(int32 const index, int32 const value) -> void
{
    auto const byteIndex = index / 4;
    auto const uByteIndex = as_uindex(byteIndex);

    assert((byteIndex >= 0 && byteIndex < ssize(values_)));
    assert(value == 0 || value == 1 || value == DontCare);

    switch (index % 4)
    {
    case 0:
        values_[uByteIndex].b0 = value & 0b11;
        break;
    case 1:
        values_[uByteIndex].b1 = value & 0b11;
        break;
    case 2:
        values_[uByteIndex].b2 = value & 0b11;
        break;
    case 3:
        values_[uByteIndex].b3 = value & 0b11;
        break;
    }
}

// pla_file definitions:

inline auto pla_file::load_file(std::string const& path)
    -> std::optional<pla_file>
{
    // Utils:
    auto constexpr is_space = [] (auto const character)
    {
        return static_cast<bool>(std::isspace(character));
    };

    auto constexpr to_words = [is_space] (auto const str)
    {
        auto words       = std::vector<std::string>();
        auto strIt       = begin(str);
        auto const endIt = end(str);

        while (strIt != endIt)
        {
            auto const wordBegin = std::find_if_not(strIt, endIt, is_space);
            auto const wordEnd   = std::find_if(wordBegin, endIt, is_space);
            if (wordBegin != wordEnd)
            {
                words.emplace_back(wordBegin, wordEnd);
            }
            strIt = wordEnd;
        }

        return words;
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
        auto const first
            = std::find_if_not(std::begin(line), std::end(line), is_space);
        auto const last = std::end(line);
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
        auto const keyLast  = std::find_if(first, last, is_space);
        auto const valFirst = keyLast == last
                                ? last
                                : std::find_if_not(keyLast + 1, last, is_space);
        auto key            = std::string(first, keyLast);
        if (valFirst != last)
        {
            auto valLast = last;
            while (is_space(*(valLast - 1)))
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
    auto const optionsEnd  = std::end(options);
    auto const varCountIt  = options.find(".i");
    auto const fCountIt    = options.find(".o");
    auto const lineCountIt = options.find(".p");
    if (varCountIt == optionsEnd || fCountIt == optionsEnd
        || lineCountIt == optionsEnd)
    {
        return std::nullopt;
    }

    auto const varCount  = utils::parse<int32>(varCountIt->second);
    auto const fCount    = utils::parse<int32>(fCountIt->second);
    auto const lineCount = utils::parse<int64>(lineCountIt->second);

    if (not varCount || not fCount || not lineCount)
    {
        return std::nullopt;
    }

    // Read data.
    auto lines = std::vector<pla_file::pla_line>();
    lines.reserve(as_usize(*lineCount));
    do
    {
        auto const first
            = std::find_if_not(std::begin(line), std::end(line), is_space);
        auto const last = std::end(line);
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
        auto const varsLast = std::find_if(first, last, is_space);
        if (varsLast == last)
        {
            return std::nullopt;
        }
        auto const fsFirst = std::find_if_not(varsLast + 1, last, is_space);
        auto const fsLast  = std::find_if(fsFirst, last, is_space);
        auto const varsStr = std::string(first, varsLast);
        auto const fStr    = std::string(fsFirst, fsLast);

        if (ssize(varsStr) != *varCount || ssize(fStr) != *fCount)
        {
            return std::nullopt;
        }

        // Parse variables into cube.
        auto variables = bool_cube(*varCount);
        for (auto i = 0; i < *varCount; ++i)
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
                return std::nullopt;
            }
        }

        // Parse functions into cube.
        auto functions = bool_cube(*fCount);
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
                return std::nullopt;
            }
        }

        lines.push_back({std::move(variables), std::move(functions)});

    } while (std::getline(ifst, line));

    // Read labels.
    auto const inLbIt = options.find(".ilb");
    auto const ouLbIt = options.find(".ob");
    auto inputLabels  = inLbIt != std::end(options) ? to_words(inLbIt->second)
                                                    : std::vector<std::string>();
    auto outputLabels = ouLbIt != std::end(options)
                          ? to_words(ouLbIt->second)
                          : std::vector<std::string>();

    return pla_file(
        std::move(lines),
        std::move(inputLabels),
        std::move(outputLabels)
    );
}

inline pla_file::pla_file(
    std::vector<pla_line> lines,
    std::vector<std::string> inputLabels,
    std::vector<std::string> outputLabels
) :
    lines_(std::move(lines)),
    inputLabels_(std::move(inputLabels)),
    outputLabels_(std::move(outputLabels))
{
}

inline auto pla_file::variable_count() const -> int32
{
    return lines_.front().cube_.size();
}

inline auto pla_file::function_count() const -> int32
{
    return lines_.front().fVals_.size();
}

inline auto pla_file::get_line_count() const -> int64
{
    return static_cast<int64>(lines_.size());
}

inline auto pla_file::get_lines() const& -> std::vector<pla_line> const&
{
    return lines_;
}

inline auto pla_file::get_lines() && -> std::vector<pla_line>
{
    return std::move(lines_);
}

inline auto pla_file::get_input_labels(
) const& -> std::vector<std::string> const&
{
    return inputLabels_;
}

inline auto pla_file::get_input_labels() && -> std::vector<std::string>
{
    return std::move(inputLabels_);
}

inline auto pla_file::get_output_labels(
) const& -> std::vector<std::string> const&
{
    return outputLabels_;
}

inline auto pla_file::get_output_labels() && -> std::vector<std::string>
{
    return std::move(outputLabels_);
}
} // namespace teddy

#endif