#ifndef MIX_DD_FILE_READER_HPP
#define MIX_DD_FILE_READER_HPP

#include <string>
#include <string_view>
#include <fstream>
#include <stdexcept>

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

// definitions:
    inline file_reader::file_reader(std::string filePath) :
        istr_     {filePath},
        filePath_ {std::move(filePath)},
        needRead_ {true}
    {
    }

    inline auto file_reader::throw_if_cant_read 
        () -> void
    {
        if (! istr_.is_open())
        {
            this->throw_cant_read();
        }
    }

    inline auto file_reader::read_line_except 
        (std::string& out) -> void
    {
        if (needRead_)
        {
            this->cache_next_line_except();
        }

        out = std::move(cachedLine_);
        needRead_ = true;
    }

    inline auto file_reader::read_line_except 
        () -> std::string
    {
        if (needRead_)
        {
            this->cache_next_line_except();
        }
        else
        {
            needRead_ = true;
        }

        return std::string {std::move(cachedLine_)};
    }

    inline auto file_reader::peek_line_except 
        () -> std::string_view
    {
        if (! needRead_)
        {
            return cachedLine_;    
        }

        this->cache_next_line_except();
        needRead_ = false;

        return cachedLine_;
    }

    inline auto file_reader::has_next_line
        () -> bool
    {
        if (! needRead_)
        {
            return true;
        }

        auto const ret = this->cache_next_line();
        needRead_ = !ret;
        return ret;
    }

    inline auto file_reader::throw_no_more_lines
        () -> void
    {
        throw std::runtime_error {"No more lines in: " + filePath_};
    }

    inline auto file_reader::throw_cant_read
        () -> void
    {
        throw std::runtime_error {"Cannot read file: " + filePath_};
    }

    inline auto file_reader::cache_next_line 
        () -> bool
    {
        return std::getline(istr_, cachedLine_).operator bool();
    }

    inline auto file_reader::cache_next_line_except 
        () -> void
    {
        if (! this->cache_next_line())
        {
            this->throw_no_more_lines();
        }
    }
}

#endif