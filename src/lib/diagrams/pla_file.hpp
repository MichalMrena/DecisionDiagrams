#ifndef MIX_DD_PLA_FILE_HPP
#define MIX_DD_PLA_FILE_HPP

#include "typedefs.hpp"
#include "bool_cube.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/parsing_utils.hpp"
#include "../utils/file_reader.hpp"
#include "../utils/more_vector.hpp"
#include "../utils/more_iterator.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <string_view>

namespace teddy
{
    /**
     *  @brief Representation of PLA file.
     */
    class pla_file
    {
    public:
        static auto load_file    (std::string        path) -> pla_file;
        static auto save_to_file (std::string const& path, pla_file const& file) -> void;

    public:
        struct pla_line
        {
            bool_cube cube;
            bool_cube fVals;
        };

    public:
        auto variable_count    () const    -> std::size_t;
        auto function_count    () const    -> std::size_t;
        auto line_count        () const    -> std::size_t;
        auto get_lines         () const &  -> std::vector<pla_line> const&;
        auto get_lines         () const && -> std::vector<pla_line>;
        auto get_input_labels  () const &  -> std::vector<std::string> const&;
        auto get_input_labels  () const && -> std::vector<std::string>;
        auto get_output_labels () const &  -> std::vector<std::string> const&;
        auto get_output_labels () const && -> std::vector<std::string>;

    private:
        pla_file ( std::vector<pla_line>    lines
                 , std::vector<std::string> inputLabels
                 , std::vector<std::string> outputLabels );

    private:
        std::vector<pla_line>    lines_;
        std::vector<std::string> inputLabels_;
        std::vector<std::string> outputLabels_;
    };

    namespace detail
    {
        inline auto read_options (utils::file_reader& reader)
        {
            enum class line_type { empty, comment, option, other };
            auto options = std::unordered_map<std::string, std::string>();

            for (;;)
            {
                auto const lineView = reader.peek_line_except();
                auto const lineType = [=](auto const sv)
                {
                    auto const end = std::end(sv);
                    auto const fc  = std::find_if(std::begin(sv), end, [](auto const c){ return !std::isspace(c); });
                    return fc  == end ? line_type::empty   :
                           '#' == *fc ? line_type::comment :
                           '.' == *fc ? line_type::option  :
                                        line_type::other;
                }(lineView);

                if (lineType == line_type::empty || lineType == line_type::comment)
                {
                    reader.read_line_except();
                    continue;
                }

                if (lineType != line_type::option)
                {
                    break;
                }

                auto line = reader.read_line_except();
                utils::trim(line);
                utils::shrink_spaces(line);
                auto [headView, tailView] = utils::to_head_tail(line);
                options.insert_or_assign(std::string(headView), tailView);
            }

            return options;
        }

        inline auto read_labels
            ( std::unordered_map<std::string, std::string> const& options
            , std::size_t        count
            , std::string const& labelKey
            , std::string_view   defaultPrefix ) -> std::vector<std::string>
        {
            auto const labelsIt = options.find(labelKey);

            if (labelsIt != std::end(options))
            {
                return utils::to_words(std::move(labelsIt->second));
            }

            return utils::fmap(utils::range(0ul, count), [=](auto const i)
            {
                return utils::concat(defaultPrefix, i);
            });
        }

        inline auto read_data ( utils::file_reader& reader
                              , std::size_t const   varCount
                              , std::size_t const   fCount
                              , std::size_t const   lineCount )
        {
            auto const char_to_bool = [](auto const c)
            {
                auto constexpr U = log_val_traits<2>::undefined;
                switch (c)
                {
                    case '0': return 0u;
                    case '1': return 1u;
                    case '-':
                    case '~': return U;
                    default:
                        throw std::runtime_error("Invalid pla line. Unknown variable value.");
                }
            };

            auto lines = utils::vector<pla_file::pla_line>(lineCount);

            do
            {
                auto line = reader.read_line_except();
                utils::trim(line);
                utils::shrink_spaces(line);
                auto const [headView, tailView] = utils::to_head_tail(line);

                if (line.empty() || line[0] == '#')
                {
                    continue;
                }

                if (tailView.empty())
                {
                    throw std::runtime_error("Invalid pla line. Expected function definition.");
                }

                if (headView.size() != varCount || tailView.size() != fCount)
                {
                    throw std::runtime_error("Invalid pla line. Expected function definition.");
                }

                auto variables = bool_cube(varCount);
                for (auto i = 0u; i < varCount; ++i)
                {
                    variables.set(i, char_to_bool(headView[i]));
                }

                auto values = bool_cube(fCount);
                for (auto i = 0u; i < fCount; ++i)
                {
                    values.set(i, char_to_bool(tailView[i]));
                }

                lines.push_back({std::move(variables), std::move(values)});

            }  while ( reader.has_next_line()
                     && !utils::starts_with(reader.peek_line_except(), ".e")
                     && !utils::starts_with(reader.peek_line_except(), ".end") );

            return lines;
        }
    }

    inline auto pla_file::load_file
        (std::string path) -> pla_file
    {
        auto reader = utils::file_reader(std::move(path));
        reader.throw_if_cant_read();

        auto const options     = detail::read_options(reader);
        auto const varCountIt  = options.find(".i");
        auto const fCountIt    = options.find(".o");
        auto const lineCountIt = options.find(".p");
        if (varCountIt == std::end(options) || fCountIt == std::end(options))
        {
            throw std::runtime_error("Invalid pla header. '.i' and '.o' must be set.");
        }

        auto const varCount  = utils::parse_except<std::uint32_t>(varCountIt->second);
        auto const fCount    = utils::parse_except<std::uint32_t>(fCountIt->second);
        auto const lineCount = lineCountIt != std::end(options)
                                   ? utils::parse_except<std::uint32_t>(lineCountIt->second)
                                   : 4u;

        return pla_file ( detail::read_data(reader, varCount, fCount, lineCount)
                        , detail::read_labels(options, varCount, ".ilb", "x")
                        , detail::read_labels(options, fCount, ".ob", "y") );
    }

    inline auto pla_file::save_to_file
        (std::string const& path, pla_file const& file) -> void
    {
        auto ost = std::ofstream(path);

        auto constexpr bool_to_char = [](auto const b)
        {
            switch (b)
            {
                case 0:  return '0';
                case 1:  return '1';
                default: return '-';
            }
        };

        ost << ".i "   << file.variable_count()                              << '\n';
        ost << ".o "   << file.function_count()                              << '\n';
        ost << ".ilb " << utils::concat_range(file.get_input_labels(),  " ") << '\n';
        ost << ".ob "  << utils::concat_range(file.get_output_labels(), " ") << '\n';
        ost << ".p "   << file.line_count()                                  << '\n';

        for (auto const& line : file.lines_)
        {
            auto const varCount = file.variable_count();
            for (auto i = 0u; i < varCount; ++i)
            {
                ost << bool_to_char(line.cube.get(i));
            }

            ost << ' ';

            auto const fCount = file.function_count();
            for (auto i = 0u; i < fCount; ++i)
            {
                ost << bool_to_char(line.fVals.get(i));
            }

            ost << '\n';
        }

        ost << ".e\n";
    }

    inline pla_file::pla_file
        ( std::vector<pla_line>    lines
        , std::vector<std::string> inputLabels
        , std::vector<std::string> outputLabels ) :
        lines_        {std::move(lines)},
        inputLabels_  {std::move(inputLabels)},
        outputLabels_ {std::move(outputLabels)}
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
        () const && -> std::vector<pla_line>
    {
        return lines_;
    }

    inline auto pla_file::get_input_labels
        () const & -> std::vector<std::string> const&
    {
        return inputLabels_;
    }

    inline auto pla_file::get_input_labels
        () const && -> std::vector<std::string>
    {
        return inputLabels_;
    }

    inline auto pla_file::get_output_labels
        () const & -> std::vector<std::string> const&
    {
        return outputLabels_;
    }

    inline auto pla_file::get_output_labels
        () const && -> std::vector<std::string>
    {
        return outputLabels_;
    }
}

#endif