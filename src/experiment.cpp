// #define NDEBUG
#include "teddy/teddy.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>

using uint_t = unsigned int;
using word_t = std::uint64_t;

auto constexpr calculate_array_size (uint_t const N) -> uint_t
{
    return (1u << N) / (8 * sizeof(std::uint64_t));
}

template<uint_t NumVar>
using bitarray = std::array<word_t, calculate_array_size(NumVar)>;

using bitrng = std::independent_bits_engine< std::mt19937_64
                                           , 8 * sizeof(word_t)
                                           , std::uint64_t >;

struct bit_iterator_sentinel {};

template<uint_t NumVar>
class bit_iterator
{
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = uint_t;
    using pointer           = value_type*;
    using reference         = value_type&;
    using iterator_category = std::input_iterator_tag;

private:
    inline static auto constexpr WordMask = word_t(1);

public:
    bit_iterator() :
        src_     (nullptr),
        srcPos_  (0),
        wordPos_ (0)
    {
    }

    bit_iterator(bitarray<NumVar> const& src) :
        src_     (&src),
        srcPos_  (0),
        wordPos_ (0)
    {
    }

    auto operator* () const -> uint_t
    {
        auto const word = src_->operator[](srcPos_);
        return (word >> wordPos_) & WordMask;
    }

    auto operator++ () -> bit_iterator&
    {
        ++wordPos_;
        if (wordPos_ == 8 * sizeof(word_t))
        {
            wordPos_ = 0;
            ++srcPos_;
        }
        return *this;
    }

    auto operator++ (int) -> bit_iterator
    {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    auto operator== (bit_iterator_sentinel) const -> bool
    {
        return srcPos_ == src_->size();
    }

    auto operator!= (bit_iterator_sentinel) const -> bool
    {
        return srcPos_ != src_->size();
    }

private:
    bitarray<NumVar> const* src_;
    uint_t srcPos_;
    uint_t wordPos_;
};

template<uint_t NumVar>
auto generate_random_vector ( bitrng&           rng
                            , bitarray<NumVar>& out )
{
    std::generate(std::begin(out), std::end(out), std::ref(rng));
}

template<uint_t NumVar>
auto generate_monotonous_vector ( std::mt19937_64&  rng
                                , bitarray<NumVar>& out )
{

}

template<uint_t NumVar>
auto print_sample_counts (uint_t sampleSize)
{
    auto rng = bitrng(std::random_device()());
    auto vector = bitarray<NumVar>();
    auto manager = teddy::bdd_manager(NumVar, 10'000);
    for (auto k = 0u; k < sampleSize; ++k)
    {
        generate_random_vector<NumVar>(rng, vector);
        auto iterator = bit_iterator<NumVar>(vector);
        auto sentinel = bit_iterator_sentinel();
        auto const diagram = manager.from_vector(iterator, sentinel);
        std::cout << manager.node_count(diagram) << '\n';
    }
    std::cout << "unique nodes = " << manager.node_count() << '\n';
}

auto main (int, char** argv) -> int
{
    // TODO naostro vypnut asserty!
    auto const sampleSize = static_cast<uint_t>(std::stoul(argv[1]));
    print_sample_counts<10>(sampleSize);
}