#ifndef MIX_DD_TRUTH_VECTOR_HPP
#define MIX_DD_TRUTH_VECTOR_HPP

#include "typedefs.hpp"
#include "../utils/more_math.hpp"
#include "../utils/more_vector.hpp"
#include "../utils/bits.hpp"
#include "../utils/string_utils.hpp"

#include <vector>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <type_traits>
#include <functional>
#include <stdexcept>

namespace mix::dd
{
    template<class Lambda>
    class lambda_iterator
    {
    public:
        using value_type        = bool;
        using difference_type   = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;
        using pointer           = void;
        using reference         = void;

    public:
        lambda_iterator ( Lambda      const& lambda
                        , index_t     const  varCount
                        , bool_vals_t const  curr );

        auto operator*  () const -> value_type;
        auto operator++ ()       -> lambda_iterator&;
        auto operator-- ()       -> lambda_iterator&;
        auto operator++ (int)    -> lambda_iterator;
        auto operator-- (int)    -> lambda_iterator;
        auto operator!= (lambda_iterator const& other) -> bool;
        auto operator-  (lambda_iterator const& other) -> difference_type;

    private:
        using lambda_cref = std::reference_wrapper<Lambda const>;
        lambda_cref lambda_;
        index_t     varCount_;
        bool_vals_t curr_;
    };

    template<class Lambda>
    class lambda_holder
    {
    public:
        using lambda_t = std::decay_t<Lambda>;
        using iterator = lambda_iterator<lambda_t>;

        lambda_holder (Lambda&& lambda);

        auto begin ()       -> iterator;
        auto end   ()       -> iterator;
        auto begin () const -> iterator;
        auto end   () const -> iterator;

    private:
        index_t  varCount_;
        lambda_t lambda_;
    };

    struct truth_vector
    {
        template<class Lambda>
        static auto from_lambda      (Lambda&& function)    -> decltype(lambda_holder {std::forward<Lambda>(function)});
        static auto from_string      (std::string_view vec) -> std::vector<bool>;
        static auto from_text_file   (std::string_view path, std::size_t const varCount = 0) -> std::vector<bool>;
        static auto from_binary_file (std::string_view path, std::size_t const varCount = 0) -> std::vector<bool>;
    };

// definitions:

    namespace aux_impl
    {
        struct vcf {};

        struct var_count_finder
        {
            index_t maxIndex {0};

            auto operator() (index_t const i)
            {
                maxIndex = std::max(maxIndex, i);
                return vcf {};
            }

            auto operator[] (index_t const i)
            {
                return (*this)(i);
            }
        };

        // note: While deducing variable count from a lambda expression
        // it is important that all calls to the var_count_finder are evaluated.
        // However when using built-in && and || operators some calls might be skipped.
        // example: x(0) && x(1) would deduce variable count 1, since value returned
        // from operator() is false so maxIndex would be 0.
        // Overloading these two operators prevents built-in short circuit evaluation.

        inline auto operator!  (vcf const&)             { return vcf {}; }
        inline auto operator&& (vcf const&, vcf const&) { return vcf {}; }
        inline auto operator&& (vcf const&, bool const) { return vcf {}; }
        inline auto operator&& (bool const, vcf const&) { return vcf {}; }
        inline auto operator|| (vcf const&, vcf const&) { return vcf {}; }
        inline auto operator|| (vcf const&, bool const) { return vcf {}; }
        inline auto operator|| (bool const, vcf const&) { return vcf {}; }
        inline auto operator!= (vcf const&, vcf const&) { return vcf {}; }
        inline auto operator!= (vcf const&, bool const) { return vcf {}; }
        inline auto operator!= (bool const, vcf const&) { return vcf {}; }

        template<class Lambda>
        auto var_count (Lambda const& lambda) -> index_t
        {
            auto finder = var_count_finder {};
            lambda(finder);
            return 1 + finder.maxIndex;
        }

        inline auto val_check_except (char const c) -> void
        {
            if ('0' != c && '1' != c)
            {
                throw std::logic_error {utils::concat("Invalid value in a truth vector: ", std::string(1, c))};
            }
        }
    }

    template<class Lambda>
    lambda_iterator<Lambda>::lambda_iterator
        (Lambda const& lambda, index_t const varCount, bool_vals_t const curr) :
        lambda_   {std::cref(lambda)},
        varCount_ {varCount},
        curr_     {curr}
    {
    }

    template<class Lambda>
    auto lambda_iterator<Lambda>::operator++
        () -> lambda_iterator&
    {
        ++curr_;
        return *this;
    }

    template<class Lambda>
    auto lambda_iterator<Lambda>::operator--
        () -> lambda_iterator&
    {
        --curr_;
        return *this;
    }

    template<class Lambda>
    auto lambda_iterator<Lambda>::operator++
        (int) -> lambda_iterator
    {
        auto const copy = *this;
        ++(*this);
        return copy;
    }

    template<class Lambda>
    auto lambda_iterator<Lambda>::operator--
        (int) -> lambda_iterator
    {
        auto const copy = *this;
        --(*this);
        return copy;
    }

    template<class Lambda>
    auto lambda_iterator<Lambda>::operator!=
        (lambda_iterator const& other) -> bool
    {
        return curr_ != other.curr_;
    }

    template<class Lambda>
    auto lambda_iterator<Lambda>::operator-
        (lambda_iterator const& other) -> difference_type
    {
        return curr_ - other.curr_;
    }

    template<class Lambda>
    auto lambda_iterator<Lambda>::operator*
        () const -> bool
    {
        auto const shift = 8 * sizeof(bool_vals_t) - varCount_;
        return lambda_(utils::bit_accesser {utils::reverse_bits(curr_) >> shift});
    }

    template<class Lambda>
    lambda_holder<Lambda>::lambda_holder(Lambda&& lambda) :
        varCount_ {aux_impl::var_count(lambda)},
        lambda_   {std::forward<Lambda>(lambda)}
    {
    }

    template<class Lambda>
    auto lambda_holder<Lambda>::begin
        () -> iterator
    {
        return iterator {lambda_,  varCount_, 0};
    }

    template<class Lambda>
    auto lambda_holder<Lambda>::end
        () -> iterator
    {
        return iterator {lambda_, varCount_, utils::two_pow(varCount_)};
    }

    template<class Lambda>
    auto lambda_holder<Lambda>::begin
        () const -> iterator
    {
        return iterator {lambda_,  varCount_, 0};
    }

    template<class Lambda>
    auto lambda_holder<Lambda>::end
        () const -> iterator
    {
        return iterator {lambda_, varCount_, utils::two_pow(varCount_)};
    }

    template<class Lambda>
    auto truth_vector::from_lambda
        (Lambda&& function) -> decltype(lambda_holder {std::forward<Lambda>(function)})
    {
        return lambda_holder {std::forward<Lambda>(function)};
    }

    inline auto truth_vector::from_string
        (std::string_view vec) -> std::vector<bool>
    {
        if (!utils::is_power_of_two(vec.size()))
        {
            throw std::logic_error {"Size of the vector must be power of two."};
        }

        auto vals = utils::vector<bool>(static_cast<std::size_t>(std::log2(vec.size())));
        auto it   = std::begin(vec);
        auto end  = std::end(vec);

        while (it != end)
        {
            auto const val = *it++;
            aux_impl::val_check_except(val);
            vals.push_back(val == '1');
        }

        return vals;
    }

    inline auto truth_vector::from_text_file
        (std::string_view path, std::size_t const varCount) -> std::vector<bool>
    {
        auto c    = '_';
        auto istr = std::fstream {std::filesystem::path {path}};
        auto vals = std::vector<bool>();

        if (!istr.is_open())
        {
            throw std::runtime_error {utils::concat("Failed to open file " , path)};
        }

        if (0 != varCount)
        {
            vals.reserve(utils::two_pow(varCount));
        }

        while (istr >> c)
        {
            aux_impl::val_check_except(c);
            vals.push_back(c == '1');
        }

        return vals;
    }

    inline auto truth_vector::from_binary_file
        (std::string_view, std::size_t const) -> std::vector<bool>
    {
        throw "not supported yet";
    }
}

#endif