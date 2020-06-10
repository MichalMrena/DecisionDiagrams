#ifndef MIX_UTILS_RANDOM_WRAP_
#define MIX_UTILS_RANDOM_WRAP_

#include <random>

namespace mix::utils
{
    namespace aux_impl
    {
        class random_base
        {
        public:
            explicit random_base(unsigned long seed);

        protected:
            std::mt19937 generator_;
        };  

        inline random_base::random_base(unsigned long seed) :
            generator_ {seed}
        {
        }
    }

    template<class IntType>
    class random_uniform_int : private aux_impl::random_base
    {
    public:
        random_uniform_int( IntType const min  = std::numeric_limits<IntType>::min()
                          , IntType const max  = std::numeric_limits<IntType>::max()
                          , unsigned long seed = std::random_device {} () );
        auto next_int () -> IntType;

    private:
        std::uniform_int_distribution<IntType> distribution_;
    };

    template<class IntType>
    random_uniform_int<IntType>::random_uniform_int ( const IntType min
                                                    , const IntType max
                                                    , unsigned long seed ) :
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