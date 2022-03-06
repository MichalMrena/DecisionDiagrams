// #define NDEBUG
#include "teddy/teddy.hpp"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <random>

using uint_t = unsigned int;
using word_t = std::uint64_t;

template<class Word>
auto constexpr word_bit_count () -> uint_t
{
    return 8 * sizeof(Word);
}

auto constexpr calculate_array_size (uint_t const numVar) -> uint_t
{
    return (1u << numVar) / word_bit_count<word_t>();
}

auto constexpr calculate_bitset_size (uint_t const numVar) -> uint_t
{
    return 1 << numVar;
}

template<uint_t NumVar>
using bitarray = std::array<word_t, calculate_array_size(NumVar)>;

template<uint_t NumVar>
using bitset = std::bitset<calculate_bitset_size(NumVar)>;

using bitrng = std::independent_bits_engine< std::mt19937_64
                                           , word_bit_count<word_t>()
                                           , word_t >;

template<uint_t NumVar>
auto set_bit (bitarray<NumVar>& words, std::size_t bitIndex) -> void
{
    auto const wordIndex = bitIndex / word_bit_count<word_t>();
    auto const wordBitIndex = bitIndex % word_bit_count<word_t>();
    auto& word = words[wordIndex];
    word |= word_t(1) << wordBitIndex;
}

template<uint_t NumVar>
auto test_bit (bitarray<NumVar> const& words, std::size_t bitIndex) -> bool
{
    auto const wordIndex = bitIndex / word_bit_count<word_t>();
    auto const wordBitIndex = bitIndex % word_bit_count<word_t>();
    auto const& word = words[wordIndex];
    return word & ~(word_t(1) << wordBitIndex);
}

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
                                , bitarray<NumVar>& out
                                , bitset<NumVar>&   memo )
{
    generate_random_vector(rng, out);
    memo.reset();
    for (auto k = memo.size(); k > 0;)
    {
        --k;
        if (not memo.test(k))
        {
            if (test_bit(out, k))
            {
                auto vars = std::bitset<8 * NumVar>(k);
            }
        }
    }
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