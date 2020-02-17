#ifndef _MIX_DD_PLA_FILE_
#define _MIX_DD_PLA_FILE_

#include <vector>
#include <string>
#include "../dd/typedefs.hpp"
#include "../data_structures/bit_vector.hpp"

namespace mix::dd
{
    struct pla_line
    {
        bit_vector<2, bool_t> varVals; 
        bit_vector<2, bool_t> fVals;

        pla_line(const pla_line&) = default;
        pla_line(pla_line&&) = default;
    };

    class pla_file
    {
    private:
        std::vector<pla_line>    lines;
        std::vector<std::string> inputLabels;
        std::vector<std::string> outputLabels;

    public:
        static auto load_from_file (const std::string& filePath) -> pla_file;
        static auto save_to_file   (const std::string& filePath, const pla_file& file) -> void;

    public:
        pla_file(const pla_file&) = default;
        pla_file(pla_file&&) = default;

        pla_file( std::vector<pla_line> pLines
                , std::vector<std::string> pInputLabels
                , std::vector<std::string> pOutputLabels );

        auto variable_count    () const -> int32_t;
        auto function_count    () const -> int32_t;
        auto line_count        () const -> int32_t;
        auto get_lines         () const -> const std::vector<pla_line>&;
        auto get_indices       () const -> std::vector<index_t>;
        auto get_input_labels  () const -> const std::vector<std::string>&;
        auto get_output_labels () const -> const std::vector<std::string>&;

        auto swap_vars (const size_t i1, const size_t i2) -> void;
    };

    auto swap (pla_line& lhs, pla_line& rhs) -> void;

    auto operator== (const pla_line& lhs, const pla_line& rhs) -> bool;
    auto operator!= (const pla_line& lhs, const pla_line& rhs) -> bool;

    auto operator== (const pla_file& lhs, const pla_file& rhs) -> bool;
    auto operator!= (const pla_file& lhs, const pla_file& rhs) -> bool;
}

#endif