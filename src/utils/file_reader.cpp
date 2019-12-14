#include "file_reader.hpp"

#include <stdexcept>

namespace mix::utils
{
    file_reader::file_reader(const std::string& filePath) :
        istr {filePath}
    {
    }

    auto file_reader::throw_if_cant_read () -> void
    {
        if (not this->istr.is_open())
        {
            throw std::runtime_error {"File can't be read."};
        }
    }

    auto file_reader::next_line_except (std::string& out) -> void
    {
        if (not std::getline(this->istr, out))
        {
            throw std::runtime_error {"Unexpected end of file."};
        }
    }

    auto file_reader::next_line_except () -> std::string
    {
        std::string line;
        this->next_line_except(line);
        return line;
    }
}