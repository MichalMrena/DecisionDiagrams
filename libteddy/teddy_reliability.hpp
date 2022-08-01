#ifndef LIBTEDDY_TEDDY_RELIABILITY_HPP
#define LIBTEDDY_TEDDY_RELIABILITY_HPP

#include <libteddy/details/reliability_manager.hpp>

namespace teddy
{
    using default_oder = std::vector<index_t>;

    /**
     *  \class bss_manager
     *  \brief Manager for creation of Binary Decision Diagrams and analysis
     *  of Binary State System.
     */
    struct bss_manager : public reliability_manager< degrees::fixed<2>
                                                   , domains::fixed<2> >
    {
        /**
         *  \brief Initializes BSS manager.
         *  \param componentCount Number of components.
         *  \param nodePoolSize Size of the main node pool.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        bss_manager ( std::size_t componentCount
                    , std::size_t nodePoolSize
                    , std::vector<index_t> order = default_oder() );

        /**
         *  \brief Initializes BSS manager.
         *  \param componentCount Number of components.
         *  \param nodePoolSize Size of the main node pool.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        bss_manager ( std::size_t componentCount
                    , std::size_t nodePoolSize
                    , std::size_t overflowNodePoolSize
                    , std::vector<index_t> order = default_oder() );
    };

    /**
     *  \class mss_manager
     *  \brief Manager for creation of Multi-valued Decision diagrams
     *  and analysis of homogenous Multi-state Systems.
     *
     *  \tparam P number of component and system states.
     */
    template<uint_t P>
    struct mss_manager : public reliability_manager< degrees::fixed<P>
                                                   , domains::fixed<P> >
    {
        /**
         *  \brief Initializes MSS manager.
         *
         *  \param componentCount Number of components.
         *  \param nodePoolSize Size of the main node pool.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        mss_manager ( std::size_t componentCount
                    , std::size_t nodePoolSize
                    , std::vector<index_t> order = default_oder() );

        /**
         *  \brief Initializes MSS manager.
         *
         *  \param componentCount Number of components.
         *  \param nodePoolSize Size of the main node pool.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        mss_manager ( std::size_t componentCount
                    , std::size_t nodePoolSize
                    , std::size_t overflowNodePoolSize
                    , std::vector<index_t> order = default_oder() );
    };

    /**
     *  \class imss_manager
     *  \brief Manager for creation of (integer) Muti-valued Decision diagrams
     *  and analysis of non-homogenous Multi-state Systems.
     */
    struct imss_manager : public reliability_manager< degrees::mixed
                                                    , domains::mixed >
    {
        /**
         *  \brief Initializes iMSS manager.
         *
         *  \param componentCount Number of components.
         *  \param nodePoolSize Size of the main node pool.
         *  \param domains Domains of variables.
         *  Number at index i is the domain of i-th variable.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        imss_manager ( std::size_t componentCount
                     , std::size_t nodePoolSize
                     , std::vector<uint_t> domains
                     , std::vector<index_t> order = default_oder() );

        /**
         *  \brief Initializes iMSS manager.
         *
         *  \param componentCount Number of components.
         *  \param nodePoolSize Size of the main node pool.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param domains Domains of variables.
         *  Number at index i is the domain of i-th variable.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        imss_manager ( std::size_t componentCount
                     , std::size_t nodePoolSize
                     , std::size_t overflowNodePoolSize
                     , std::vector<uint_t> domains
                     , std::vector<index_t> order = default_oder() );
    };

    /**
     *  \class ifmss_manager
     *  \brief Manager for creation of (integer) Muti-valued Decision diagrams
     *  and analysis of non-homogenous Multi-state Systems.
     *
     *  \tparam PMax maximal number of system and component states.
     */
    template<uint_t PMax>
    struct ifmss_manager : public reliability_manager< degrees::fixed<PMax>
                                                     , domains::mixed >
    {
        /**
         *  \brief Initializes ifMSS manager.
         *
         *  \param componentCount Number of components.
         *  \param nodePoolSize Size of the main node pool.
         *  \param domains Domains of variables.
         *  Number at index i is the domain of i-th variable.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        ifmss_manager ( std::size_t componentCount
                      , std::size_t nodePoolSize
                      , std::vector<uint_t> domains
                      , std::vector<index_t> order = default_oder() );

        /**
         *  \brief Initializes ifMSS manager.
         *
         *  \param componentCount Number of components.
         *  \param nodePoolSize Size of the main node pool.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param domains Domains of variables.
         *  Number at index i is the domain of i-th variable.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        ifmss_manager ( std::size_t componentCount
                      , std::size_t nodePoolSize
                      , std::size_t overflowNodePoolSize
                      , std::vector<uint_t> domains
                      , std::vector<index_t> order = default_oder() );
    };




    inline bss_manager::bss_manager
        ( std::size_t const componentCount
        , std::size_t const nodePoolSize
        , std::vector<index_t> order ) :
        bss_manager
            ( componentCount
            , nodePoolSize
            , nodePoolSize / 2
            , std::move(order) )
    {
    }

    inline bss_manager::bss_manager
        ( std::size_t const componentCount
        , std::size_t const nodePoolSize
        , std::size_t const overflowNodePoolSize
        , std::vector<index_t> order ) :
        reliability_manager<degrees::fixed<2>, domains::fixed<2>>
            ( componentCount
            , nodePoolSize
            , overflowNodePoolSize
            , std::move(order) )
    {
    }

    template<uint_t P>
    mss_manager<P>::mss_manager
        ( std::size_t const componentCount
        , std::size_t const nodePoolSize
        , std::vector<index_t> order ) :
        mss_manager
            ( componentCount
            , nodePoolSize
            , nodePoolSize / 2
            , std::move(order) )
    {
    }

    template<uint_t P>
    mss_manager<P>::mss_manager
        ( std::size_t const componentCount
        , std::size_t const nodePoolSize
        , std::size_t const overflowNodePoolSize
        , std::vector<index_t> order ) :
        reliability_manager<degrees::fixed<P>, domains::fixed<P>>
            ( componentCount
            , nodePoolSize
            , overflowNodePoolSize
            , std::move(order) )
    {
    }

    inline imss_manager::imss_manager
        ( std::size_t const componentCount
        , std::size_t const nodePoolSize
        , std::vector<uint_t> domains
        , std::vector<index_t> order ) :
        imss_manager
            ( componentCount
            , nodePoolSize
            , nodePoolSize / 2
            , std::move(domains)
            , std::move(order) )
    {
    }

    inline imss_manager::imss_manager
        ( std::size_t const componentCount
        , std::size_t const nodePoolSize
        , std::size_t const overflowNodePoolSize
        , std::vector<uint_t> domains
        , std::vector<index_t> order ) :
        reliability_manager<degrees::mixed, domains::mixed>
            ( componentCount
            , nodePoolSize
            , overflowNodePoolSize
            , std::move(domains)
            , std::move(order) )
    {
    }

    template<uint_t PMax>
    ifmss_manager<PMax>::ifmss_manager
        ( std::size_t const componentCount
        , std::size_t const nodePoolSize
        , std::vector<uint_t> domains
        , std::vector<index_t> order ) :
        ifmss_manager
            ( componentCount
            , nodePoolSize
            , nodePoolSize / 2
            , std::move(domains)
            , std::move(order) )
    {
    }

    template<uint_t PMax>
    ifmss_manager<PMax>::ifmss_manager
        ( std::size_t const componentCount
        , std::size_t const nodePoolSize
        , std::size_t const overflowNodePoolSize
        , std::vector<uint_t> domains
        , std::vector<index_t> order ) :
        reliability_manager<degrees::fixed<PMax>, domains::mixed>
            ( componentCount
            , nodePoolSize
            , overflowNodePoolSize
            , std::move(domains)
            , std::move(order) )
    {
    }
}

#endif