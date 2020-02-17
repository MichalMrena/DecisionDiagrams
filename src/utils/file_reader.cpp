#include "file_reader.hpp"

#include <stdexcept>

namespace mix::utils
{
    file_reader::file_reader(const std::string& pFilePath) :
        filePath {pFilePath}
      , istr     {pFilePath}
      , needRead {true}
    {
    }

    auto file_reader::throw_if_cant_read 
        () -> void
    {
        if (! this->istr.is_open())
        {
            this->throw_cant_read();
        }
    }

    auto file_reader::read_line_except 
        (std::string& out) -> void
    {
        if (! this->needRead)
        {
            this->needRead = true;
            out = this->cachedLine;
            return;
        }

        if (! std::getline(this->istr, out))
        {
            this->throw_no_more_lines();
        }

        this->needRead = true;
    }

    auto file_reader::read_line_except 
        () -> std::string&&
    {

        if (! this->needRead)
        {
            this->needRead = true;
            return std::move(this->cachedLine);
        }
        
        if (! this->cache_next_line())
        {
            this->throw_no_more_lines();
        }

        this->needRead = true;
        return std::move(this->cachedLine);
    }

    auto file_reader::peek_line_except 
        () -> const std::string&
    {
        if (! this->needRead)
        {
            return this->cachedLine;    
        }

        if (! this->cache_next_line())
        {
            this->throw_no_more_lines();
        }

        this->needRead = false;

        return this->cachedLine;
    }

    auto file_reader::has_next_line
        () -> bool
    {
        if (! this->needRead)
        {
            return true;
        }

        const auto ret {this->cache_next_line()};
        this->needRead = !ret;
        return ret;
    }

    auto file_reader::throw_no_more_lines
        () -> void
    {
        throw std::runtime_error {"No more lines in: " + this->filePath};
    }

    auto file_reader::throw_cant_read
        () -> void
    {
        throw std::runtime_error {"Cannot read file: " + this->filePath};
    }

    auto file_reader::cache_next_line 
        () -> bool
    {
        return std::getline(this->istr, this->cachedLine).operator bool();
    }
}