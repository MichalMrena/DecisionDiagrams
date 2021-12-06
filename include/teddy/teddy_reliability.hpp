#ifndef TEDDY_RELIABILITY_HPP
#define TEDDY_RELIABILITY_HPP

#include "impl/reliability_manager.hpp"

namespace teddy
{
    using default_oder = std::vector<index_t>;

    struct bss_manager : public reliability_manager< degrees::fixed<2>
                                                   , domains::fixed<2> >
    {
        bss_manager ( std::size_t varCount
                    , std::size_t initNodeCount
                    , std::vector<index_t> order = default_oder() );
    };

    template<uint_t P>
    struct mss_manager : public reliability_manager< degrees::fixed<P>
                                                   , domains::fixed<P> >
    {
        mss_manager ( std::size_t varCount
                    , std::size_t initNodeCount
                    , std::vector<index_t> order = default_oder() );
    };

    struct imss_manager : public reliability_manager< degrees::mixed
                                                    , domains::mixed >
    {
        imss_manager ( std::size_t varCount
                     , std::size_t initNodeCount
                     , std::vector<uint_t> domains
                     , std::vector<index_t> order = default_oder() );
    };

    template<uint_t PMax>
    struct ifmss_manager : public reliability_manager< degrees::fixed<PMax>
                                                     , domains::mixed >
    {
        ifmss_manager ( std::size_t varCount
                      , std::size_t initNodeCount
                      , std::vector<uint_t> domains
                      , std::vector<index_t> order = default_oder() );
    };




    inline bss_manager::bss_manager
        ( std::size_t const varCount
        , std::size_t const initNodeCount
        , std::vector<index_t> order ) :
        reliability_manager<degrees::fixed<2>, domains::fixed<2>>
            (varCount, initNodeCount, std::move(order))
    {
    }

    template<uint_t P>
    mss_manager<P>::mss_manager
        ( std::size_t const varCount
        , std::size_t const initNodeCount
        , std::vector<index_t> order ) :
        reliability_manager<degrees::fixed<P>, domains::fixed<P>>
            (varCount, initNodeCount, std::move(order))
    {
    }

    inline imss_manager::imss_manager
        ( std::size_t const varCount
        , std::size_t const initNodeCount
        , std::vector<uint_t> domains
        , std::vector<index_t> order ) :
        reliability_manager<degrees::mixed, domains::mixed>
            (varCount, initNodeCount, std::move(domains), std::move(order))
    {
    }

    template<uint_t PMax>
    ifmss_manager<PMax>::ifmss_manager
        ( std::size_t const varCount
        , std::size_t const initNodeCount
        , std::vector<uint_t> domains
        , std::vector<index_t> order ) :
        reliability_manager<degrees::fixed<PMax>, domains::mixed>
            (varCount, initNodeCount, std::move(domains), std::move(order))
    {
    }
}

#endif