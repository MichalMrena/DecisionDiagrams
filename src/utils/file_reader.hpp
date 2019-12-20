#ifndef _MIX_DD_FILE_READER_
#define _MIX_DD_FILE_READER_

#include <string>
#include <fstream>

namespace mix::utils
{
    class file_reader
    {
    private:
        std::ifstream istr;

    public:
        file_reader(const std::string& filePath);
        ~file_reader() = default;

        auto throw_if_cant_read () -> void;
        auto next_line_except   (std::string& out) -> void;
        auto next_line_except   () -> std::string;

        auto peek_line () -> const std::string&;
    };
}

#endif