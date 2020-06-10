#ifndef _MIX_DD_FILE_READER_
#define _MIX_DD_FILE_READER_

#include <string>
#include <string_view>
#include <fstream>

namespace mix::utils
{
    class file_reader
    {
    public:
        explicit file_reader(std::string filePath);

        auto throw_if_cant_read ()                 -> void;
        auto read_line_except   (std::string& out) -> void;
        auto read_line_except   ()                 -> std::string;
        auto peek_line_except   ()                 -> std::string_view;
        auto has_next_line      ()                 -> bool;

    private:
        auto cache_next_line        () -> bool;
        auto cache_next_line_except () -> void;
        auto throw_no_more_lines    () -> void;
        auto throw_cant_read        () -> void;

    private:
        std::ifstream istr_;
        std::string   filePath_;
        std::string   cachedLine_;
        bool          needRead_;
    };
}

#endif