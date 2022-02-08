#ifndef TEDDY_VECTOR_FUNCTION_HPP
#define TEDDY_VECTOR_FUNCTION_HPP

#include <algorithm>
#include <cassert>
#include <concepts>
#include <functional>
#include <limits>
#include <numeric>
#include <vector>

namespace teddy::vector
{
    using uint_t = unsigned int;

    inline auto constexpr U = std::numeric_limits<uint_t>::max();

    struct var_val_change
    {
        uint_t index;
        uint_t from;
        uint_t to;
    };

    template<class F>
    concept f_val_change = requires(F f)
    {
        { f(uint_t(), uint_t()) } -> std::convertible_to<bool>;
    };

    template<class F>
    concept domain_consumer = requires(F f)
    {
        f(uint_t(), std::vector<uint_t>());
    };

    template<class F>
    concept uint_bin_op = requires(F f)
    {
        { f(uint_t(), uint_t()) } -> std::convertible_to<uint_t>;
    };

    /**
     *  Integer function represented by truth vector.
     */
    class vector_function
    {
    public:
        /**
         *  Initializes the function using @p vector and @p domains .
         */
        vector_function
            ( std::vector<uint_t> vector
            , std::vector<uint_t> domains ) :
            vector_   (std::move(vector)),
            domains_  (std::move(domains)),
            offset_   (this->var_count()),
            maxValue_ (std::ranges::max(vector_))
        {
            assert(vector_.size() == std::reduce(
                begin(domains_), end(domains_), 1u, std::multiplies<>()));

            offset_[this->var_count() - 1] = 1;
            if (this->var_count() > 1)
            {
                auto i = this->var_count() - 1;
                while (i > 0)
                {
                    --i;
                    offset_[i] = domains_[i + 1] * offset_[i + 1];
                }
            }
        }

        /**
         *  Evaluates the function for given values of variables @p vars .
         */
        auto evaluate (std::vector<uint_t> const& vars) const -> uint_t
        {
            return vector_[this->to_index(vars)];
        }

        /**
         *  Calculates DPBD where @p var describes variable and its change and
         *  @p d is a function that checks whether change in value of the 
         *  function is right for the derivative type.
         */
        template<f_val_change F>
        auto dpbd (var_val_change const var, F const d) const -> vector_function
        {
            auto dpbdVector = std::vector<uint_t>(vector_.size());
            auto tmpElem = std::vector<uint_t>();
            auto k = 0u;

            this->domain_for_each([&](auto const val, auto const& elem)
            {
                if (val != var.from)
                {
                    dpbdVector[k] = U;
                }
                else
                {
                    tmpElem = elem;
                    tmpElem[var.index] = var.to;
                    auto const valTo = this->evaluate(tmpElem);
                    dpbdVector[k] = d(elem[var.index], valTo) ? 1 : 0;
                }
                ++k;
            });

            return vector_function(std::move(dpbdVector), domains_);
        }

        /**
         *  Returns domain elements for which the function evaluates to 1.
         */
        auto satisfy_all () const -> std::vector<std::vector<uint_t>>
        {
            auto elems = std::vector<std::vector<uint_t>>();
            this->domain_for_each([this, &elems](auto const val, auto elem)
            {
                if (val == 1)
                {
                    elems.emplace_back(std::move(elem));
                }
            });
            return elems;
        }

        /**
         *  Compares vector of this function with @p vector .
         *  Does not consider domains!
         */
        auto raw_compare (std::vector<uint_t> const& vector) const -> bool
        {
            return vector_ == vector;
        }

        /**
         *  Returns new function as a result of applying @p f on @p l and @p l .
         */
        template<uint_bin_op F>
        friend auto vector_op
            ( F const                f
            , vector_function const& l
            , vector_function const& r ) -> vector_function
        {
            assert(l.domains_ == r.domains_);

            auto newVector = std::vector<uint_t>(l.domain_size());
            for (auto k = 0u; k < l.domain_size(); ++k)
            {
                newVector[k] = f(l.vector_, r.vector_);
            }
            return vector_function(std::move(newVector), l.domains_);
        }

        /**
         *  Invokes @p f with each element of the domain.
         */
        template<domain_consumer F>
        auto domain_for_each (F f) const -> void
        {
            auto element = std::vector<uint_t>(this->var_count(), 0);
            auto wasLast = false;
            auto k = 0u;
            do
            {
                // Invoke f.
                f(k, element);

                // Move to the next domain element.
                auto overflow = true;
                auto i = 0u;
                while (i < this->var_count() && overflow)
                {
                    ++element[i];
                    overflow = element[i] == domains_[i];
                    if (overflow)
                    {
                        element[i] = 0;
                    }
                    ++i;
                }

                wasLast = overflow;
                ++k;
            } while (wasLast);
        }

        /**
         *  Returns the number of variables this function depends on.
         */
        auto var_count () const -> uint_t
        {
            return static_cast<uint_t>(domains_.size());
        }

        /**
         *  Returns the number of elements in the functions domain.
         */
        auto domain_size () const -> uint_t
        {
            return static_cast<uint_t>(vector_.size());
        }

        /**
         *  Returns maximal value of the function.
         */
        auto max_value () const -> uint_t
        {
            return maxValue_;
        }

    private:
        /**
         *  Maps values of variables to index.
         */
        auto to_index (std::vector<uint_t> const& vars) const -> uint_t
        {
            assert(vars.size() == this->var_count());
            auto index = 0u;
            for (auto i = 0u; i < this->var_count(); ++i)
            {
                index += vars[i] * offset_[i];
            }
            return index;
        }

    private:
        std::vector<uint_t> vector_;
        std::vector<uint_t> domains_;
        std::vector<uint_t> offset_;
        uint_t              maxValue_;
    };


    class vector_reliability
    {
    public:
        vector_reliability
            ( vector_function const&           sf
            , std::vector<std::vector<double>> ps ) :
            sf_ (&sf),
            ps_ (std::move(ps))
        {
        }

        auto probability (uint_t const j) const -> double
        {
            auto result = 0.0;
            sf().domain_for_each([&](auto const val, auto const& elem)
            {
                if (val == j)
                {
                    result += this->probability(elem);
                }
            });
            return result;
        }

        auto availability (uint_t const j) const -> double
        {
            auto result = 0.0;
            sf().domain_for_each([&](auto const val, auto const& elem)
            {
                if (val >= j)
                {
                    result += this->probability(elem);
                }
            });
            return result;
        }

        auto unavailability (uint_t const j) const -> double
        {
            auto result = 0.0;
            sf().domain_for_each([&](auto const val, auto const& elem)
            {
                if (val < j)
                {
                    result += this->probability(elem);
                }
            });
            return result;
        }

    private:
        auto probability (std::vector<uint_t> const& vars) const -> double
        {
            auto result = 1.0;
            for (auto i = 0u; i < sf().var_count(); ++i)
            {
                result *= ps_[i][vars[i]];
            }
            return result;
        }

        auto sf () const -> vector_function const&
        {
            return *sf_;
        }

    private:
        vector_function const*           sf_;
        std::vector<std::vector<double>> ps_;
    };
}

#endif