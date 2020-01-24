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

        pla_line(const pla_line&) = delete;
        pla_line(pla_line&&) = default;
    };

    auto swap (pla_line& lhs, pla_line& rhs) -> void;

    class pla_file
    {
    private:
        std::vector<pla_line> lines;

    public:
        static auto load_from_file (const std::string& filePath) -> pla_file;

    public:
        pla_file(const pla_file&) = delete;
        pla_file(pla_file&&) = default;

        pla_file(std::vector<pla_line> pLines);

        auto variable_count () const -> int32_t;
        auto function_count () const -> int32_t;
        auto line_count     () const -> int32_t;
        auto get_lines      () const -> const std::vector<pla_line>&;
    };
}

#endif