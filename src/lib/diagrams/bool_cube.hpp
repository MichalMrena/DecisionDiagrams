#ifndef MIX_DD_BOOL_CUBE_HPP
#define MIX_DD_BOOL_CUBE_HPP

#include <vector>

namespace teddy
{
    class bool_cube
    {
    public:
        bool_cube (std::size_t size_);

        auto size () const                          -> std::size_t;
        auto get  (std::size_t i) const             -> unsigned int;
        auto set  (std::size_t i, unsigned int val) -> void;

    private:
        struct byte
        {
            unsigned int b0 : 2;
            unsigned int b1 : 2;
            unsigned int b2 : 2;
            unsigned int b3 : 2;
        };

    private:
        std::size_t       size_;
        std::vector<byte> values_;
    };

    inline bool_cube::bool_cube (std::size_t size) :
        size_   (size),
        values_ (size / 4 + 1, byte {0, 0, 0, 0})
    {
    }

    inline auto bool_cube::size
        () const -> std::size_t
    {
        return size_;
    }

    inline auto bool_cube::get
        (std::size_t const i) const -> unsigned int
    {
        switch (i % 4)
        {
            case 0: return values_[i / 4].b0;
            case 1: return values_[i / 4].b1;
            case 2: return values_[i / 4].b2;
            case 3: return values_[i / 4].b3;
        }
        return static_cast<unsigned int>(-1);
    }

    inline auto bool_cube::set
        (std::size_t const i, unsigned int const val) -> void
    {
        switch (i % 4)
        {
            case 0: values_[i / 4].b0 = val; break;
            case 1: values_[i / 4].b1 = val; break;
            case 2: values_[i / 4].b2 = val; break;
            case 3: values_[i / 4].b3 = val; break;
        }
    }
}

#endif