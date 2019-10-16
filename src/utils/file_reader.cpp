#include "file_reader.hpp"

#include "io_exception.hpp"

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
            throw io_exception {"File can't be read."};
        }
    }

    auto file_reader::next_line_except (std::string& out) -> void
    {
        if (not std::getline(this->istr, out))
        {
            throw io_exception {"Unexpected end of file."};
        }
    }
}