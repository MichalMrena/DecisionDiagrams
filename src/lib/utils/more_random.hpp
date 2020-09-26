#ifndef MIX_UTILS_MORE_RANDOM_HPP
#define MIX_UTILS_MORE_RANDOM_HPP

#include <random>

namespace mix::utils
{
    namespace aux_impl
    {
        class random_base
        {
        public:
            using seed_t = unsigned long;

            explicit random_base(seed_t const seed);

        protected:
            std::mt19937 generator_;
        };

        inline random_base::random_base(seed_t const seed) :
            generator_ {seed}
        {
        }
    }

    template<class IntType>
    class random_uniform_int : private aux_impl::random_base
    {
    public:
        using base_t = aux_impl::random_base;
        using seed_t = typename base_t::seed_t;
        using dist_t = std::uniform_int_distribution<IntType>;

        random_uniform_int( IntType const min  = std::numeric_limits<IntType>::min()
                          , IntType const max  = std::numeric_limits<IntType>::max()
                          , seed_t  const seed = std::random_device {} () );

        auto next_int () -> IntType;

    private:
        dist_t distribution_;
    };

    template<class IntType>
    random_uniform_int<IntType>::random_uniform_int ( IntType const min
                                                    , IntType const max
                                                    , seed_t  const seed ) :
        random_base   {seed}
      , distribution_ {min, max}
    {
    }

    template<class IntType>
    auto random_uniform_int<IntType>::next_int
        () -> IntType
    {
        return distribution_(random_base::generator_);
    }
}

#endif