#ifndef MIX_UTILS_MORE_RANDOM_HPP
#define MIX_UTILS_MORE_RANDOM_HPP

#include <random>

namespace teddy::utils
{
    /**
        @brief Base class for pseudo random generators.
     */
    class random_base
    {
    public:
        using seed_t   = unsigned int;
        using engine_t = std::mt19937;

    public:
        inline static auto constexpr Max = engine_t::max();

    protected:
        explicit random_base (seed_t const seed);

    protected:
        engine_t generator_;
    };

    /**
        @brief Generates pseudo random integer of given type from closed interval [a, b].
     */
    template<class IntType>
    class random_uniform_int : private random_base
    {
    public:
        using base   = random_base;
        using seed_t = typename base::seed_t;
        using dist_t = std::uniform_int_distribution<IntType>;

    public:
        inline static auto constexpr Min = std::numeric_limits<IntType>::min();
        inline static auto constexpr Max = std::numeric_limits<IntType>::max();

    public:
        random_uniform_int( IntType const a    = Min
                          , IntType const b    = Max
                          , seed_t  const seed = std::random_device {} () );

        auto next_int () -> IntType;

    private:
        dist_t distribution_;
    };

    /**
     *  @brief Generates true with probability p and false with probability (1 - p).
     */
    class random_bool : private random_base
    {
    public:
        using base   = random_base;
        using seed_t = typename base::seed_t;

    public:
        random_bool ( seed_t const seed );
        random_bool ( double const p    = 0.5
                    , seed_t const seed = std::random_device {} () );

        auto next_bool () -> bool;

    private:
        double p_;
    };

// random_base definitions:

    inline random_base::random_base
        (seed_t const seed) :
        generator_ {seed}
    {
    }

// random_uniform_int definitions:

    template<class IntType>
    random_uniform_int<IntType>::random_uniform_int
        ( IntType const a
        , IntType const b
        , seed_t  const seed ) :
        base          {seed},
        distribution_ {a, b}
    {
    }

    template<class IntType>
    auto random_uniform_int<IntType>::next_int
        () -> IntType
    {
        return distribution_(base::generator_);
    }

// random_bool definitions:

    inline random_bool::random_bool
        (seed_t const seed) :
        base {seed},
        p_   {0.5}
    {
    }

    inline random_bool::random_bool
        ( double const p
        , seed_t const seed ) :
        base {seed},
        p_   {p}
    {
    }

    inline auto random_bool::next_bool
        () -> bool
    {
        return static_cast<double>(base::generator_()) / base::Max < p_;
    }
}

#endif