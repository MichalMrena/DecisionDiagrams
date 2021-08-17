#ifndef MIX_DD_TEDDY_HPP
#define MIX_DD_TEDDY_HPP

// #include diagram_manager.hpp
    #include "impl/node_manager.hpp"

namespace teddy
{
    // template<bool B, class T, class U>
    // using t_if = std::conditional_t<B, T, U>;

    // using bdd_manager = bin_diagram_manager<void>;

    // template<uint_t P>
    // using mdd_manager = t_if< P == 2
    //                         , bdd_manager
    //                         , diagram_manager< void
    //                                         , degrees::fixed<P>
    //                                         , domains::fixed<P>> >;

    // using imdd_manager = diagram_manager< void
    //                                     , degrees::fixed<P>
    //                                     , domains::fixed<P> >;

    // template<uint_t PMax>
    // using ifmdd_manager = t_if< P == 2
    //                           , bdd_manager
    //                           , diagram_manager< void
    //                                            , degrees::fixed<PMax>
    //                                            , domains::mixed >;
}

#endif