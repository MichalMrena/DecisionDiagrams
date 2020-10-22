#ifndef MIX_UTILS_DEBUG_TOOLS_HPP
#define MIX_UTILS_DEBUG_TOOLS_HPP

#include <vector>
#include <cstddef>

namespace mix::utils
{
    template<class T>
    class average_keeper
    {
    public:
        auto add   (T const t) -> average_keeper&;
        auto avg   () const    -> double;
        auto count () const    -> std::size_t;

    private:
        double      avg_   {0};
        std::size_t count_ {0};
    };

    template<class T>
    auto average_keeper<T>::add
        (T const t) -> average_keeper&
    {
        avg_ = (avg_ * count_) + t;
        avg_ /= ++count_;

        return *this;
    }

    template<class T>
    auto average_keeper<T>::avg
        () const -> double
    {
        return avg_;
    }

    template<class T>
    auto average_keeper<T>::count
        () const -> std::size_t
    {
        return count_;
    }
}

#endif