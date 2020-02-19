#ifndef _MIX_DD_FILE_READER_
#define _MIX_DD_FILE_READER_

#include <string>
#include <fstream>

namespace mix::utils
{
    class file_reader
    {
    private:
        const std::string filePath;

        std::ifstream istr;
        
        bool needRead;
        std::string cachedLine;

    public:
        explicit file_reader(const std::string& filePath);
        ~file_reader() = default;

        auto throw_if_cant_read () -> void;
        auto read_line_except (std::string& out) -> void;
        
        auto read_line_except () -> std::string&&;
        auto peek_line_except () -> const std::string&;
        auto has_next_line    () -> bool;

    private:
        auto cache_next_line        () -> bool;
        auto cache_next_line_except () -> void;
        auto throw_no_more_lines    () -> void;
        auto throw_cant_read        () -> void;
    };
}

#endif