#ifndef MIX_DD_TEDDY_HPP
#define MIX_DD_TEDDY_HPP

#include "impl/diagram_manager.hpp"

namespace teddy
{
    using default_oder = std::vector<index_t>;

    struct bdd_manager : public diagram_manager< void
                                               , degrees::fixed<2>
                                               , domains::fixed<2> >
    {
        bdd_manager ( std::size_t varCount
                    , std::size_t initNodeCount
                    , std::vector<index_t> order = default_oder() );
    };

    template<uint_t P>
    struct mdd_manager : public diagram_manager< void
                                               , degrees::fixed<P>
                                               , domains::fixed<P> >
    {
        mdd_manager ( std::size_t varCount
                    , std::size_t initNodeCount
                    , std::vector<index_t> order = default_oder() );
    };

    struct imdd_manager : public diagram_manager< void
                                                , degrees::mixed
                                                , domains::mixed >
    {
        imdd_manager ( std::size_t varCount
                     , std::size_t initNodeCount
                     , std::vector<uint_t> domains
                     , std::vector<index_t> order = default_oder() );
    };

    template<uint_t PMax>
    struct ifmdd_manager : public diagram_manager< void
                                                 , degrees::fixed<PMax>
                                                 , domains::mixed >
    {
        ifmdd_manager ( std::size_t varCount
                      , std::size_t initNodeCount
                      , std::vector<uint_t> domains
                      , std::vector<index_t> order = default_oder() );
    };




    inline bdd_manager::bdd_manager
        ( std::size_t const varCount
        , std::size_t const initNodeCount
        , std::vector<index_t> order ) :
        diagram_manager(varCount, initNodeCount, std::move(order))
    {
    }
}

#endif