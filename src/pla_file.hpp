#ifndef _MIX_DD_PLA_FILE_
#define _MIX_DD_PLA_FILE_

#include <vector>
#include <string>
#include "typedefs.hpp"

namespace mix::dd
{
    struct pla_line
    {
        // TODO použiť 2-bit set aby to bolo viac memmory friendly
        std::vector<log_val_t> varVals; 
        // TODO použiť 1-bit set aby to bolo viac memmory friendly
        std::vector<log_val_t> fVals;

        pla_line(const pla_line&) = delete;
        pla_line(pla_line&&) = default;
    };

    class pla_file
    {
    private:
        std::vector<pla_line> lines;

    public:
        pla_file(const pla_file&) = delete;
        pla_file(pla_file&&) = default;

        static auto read (const std::string& filePath) -> pla_file;

        auto variable_count () const -> int32_t;
        auto function_count () const -> int32_t;
        auto line_count     () const -> int32_t;
        auto get_lines      () const -> const std::vector<pla_line>&;

    private:
        pla_file(std::vector<pla_line> pLines);
    };
}

#endif