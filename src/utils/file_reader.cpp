#include "file_reader.hpp"

#include <stdexcept>

namespace mix::utils
{
    file_reader::file_reader(const std::string& pFilePath) :
        filePath_ {pFilePath}
      , istr_     {pFilePath}
      , needRead_ {true}
    {
    }

    auto file_reader::throw_if_cant_read 
        () -> void
    {
        if (! istr_.is_open())
        {
            this->throw_cant_read();
        }
    }

    auto file_reader::read_line_except 
        (std::string& out) -> void
    {
        if (needRead_)
        {
            this->cache_next_line_except();
        }
        
        out = std::move(cachedLine_);
        needRead_ = true;
    }

    auto file_reader::read_line_except 
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

    auto file_reader::peek_line_except 
        () -> const std::string&
    {
        if (! needRead_)
        {
            return cachedLine_;    
        }

        this->cache_next_line_except();
        needRead_ = false;

        return cachedLine_;
    }

    auto file_reader::has_next_line
        () -> bool
    {
        if (! needRead_)
        {
            return true;
        }

        const auto ret {this->cache_next_line()};
        needRead_ = !ret;
        return ret;
    }

    auto file_reader::throw_no_more_lines
        () -> void
    {
        throw std::runtime_error {"No more lines in: " + filePath_};
    }

    auto file_reader::throw_cant_read
        () -> void
    {
        throw std::runtime_error {"Cannot read file: " + filePath_};
    }

    auto file_reader::cache_next_line 
        () -> bool
    {
        return std::getline(istr_, cachedLine_).operator bool();
    }
    
    auto file_reader::cache_next_line_except 
        () -> void
    {
        if (! this->cache_next_line())
        {
            this->throw_no_more_lines();
        }
    }
}