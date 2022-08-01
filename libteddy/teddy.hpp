#ifndef MIX_DD_TEDDY_HPP
#define MIX_DD_TEDDY_HPP

#include "impl/diagram_manager.hpp"
#include "impl/pla_file.hpp"

namespace teddy
{
    using default_oder = std::vector<index_t>;

    /**
     *  \class bdd_manager
     *  \brief Diagram manager for creation and manipulation
     *  of Binary Decision Diagrams.
     */
    struct bdd_manager : public diagram_manager< void
                                               , degrees::fixed<2>
                                               , domains::fixed<2> >
    {
        /**
         *  \brief Initializes BDD manager.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Size of the main node pool.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        bdd_manager ( std::size_t varCount
                    , std::size_t nodePoolSize
                    , std::vector<index_t> order = default_oder() );

        /**
         *  \brief Initializes BDD manager.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Size of the main node pool.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        bdd_manager ( std::size_t varCount
                    , std::size_t nodePoolSize
                    , std::size_t overflowNodePoolSize
                    , std::vector<index_t> order = default_oder() );
    };

    /**
     *  \class mdd_manager
     *  \brief Diagram manager for creation and manipulation
     *  of Multi-valued Decision Diagrams.
     *
     *  \tparam P domain of variables.
     */
    template<uint_t P>
    struct mdd_manager : public diagram_manager< void
                                               , degrees::fixed<P>
                                               , domains::fixed<P> >
    {
        /**
         *  \brief Initializes MDD manager.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Size of the main node pool.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        mdd_manager ( std::size_t varCount
                    , std::size_t nodePoolSize
                    , std::vector<index_t> order = default_oder() );

        /**
         *  \brief Initializes MDD manager.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Size of the main node pool.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        mdd_manager ( std::size_t varCount
                    , std::size_t nodePoolSize
                    , std::size_t overflowNodePoolSize
                    , std::vector<index_t> order = default_oder() );
    };

    /**
     *  \class imdd_manager
     *  \brief Diagram manager for creation and manipulation
     *  of (integer) Multi-valued Decision Diagrams (iMDDs).
     *
     *  Unlike \c mdd_manager variables in iMDDs can have
     *  different domains. Node representation is less compact in
     *  this case since the number of sons of a node is not known
     *  at compile time.
     */
    struct imdd_manager : public diagram_manager< void
                                                , degrees::mixed
                                                , domains::mixed >
    {
        /**
         *  \brief Initializes iMDD manager.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Size of the main node pool.
         *  \param domains Domains of variables.
         *  Number at index i is the domain of i-th variable.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        imdd_manager ( std::size_t varCount
                     , std::size_t nodePoolSize
                     , std::vector<uint_t> domains
                     , std::vector<index_t> order = default_oder() );

        /**
         *  \brief Initializes iMDD manager.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Size of the main node pool.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param domains Domains of variables.
         *  Number at index i is the domain of i-th variable.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        imdd_manager ( std::size_t varCount
                     , std::size_t nodePoolSize
                     , std::size_t overflowNodePoolSize
                     , std::vector<uint_t> domains
                     , std::vector<index_t> order = default_oder() );
    };

    /**
     *  \class ifmdd_manager
     *  \brief Diagram manager for creation and manipulation
     *  of (integer) Multi-valued Decision Diagrams (iMDDs).
     *
     *  Unlike \c mdd_manager variables in ifMDDs can have
     *  different domains. However, node representation is the same
     *  since the maximal number of sons is known at compile time.
     *  Note that some memory might be allocated but unused because
     *  each node allocates space for \p PMax sons regardles of its domain.
     *
     *  \tparam PMax maximum from the sizes of domains of variables.
     */
    template<uint_t PMax>
    struct ifmdd_manager : public diagram_manager< void
                                                 , degrees::fixed<PMax>
                                                 , domains::mixed >
    {
        /**
         *  \brief Initializes ifMDD manager.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Size of the main node pool.
         *  \param domains Domains of variables.
         *  Number at index i is the domain of i-th variable.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        ifmdd_manager ( std::size_t varCount
                      , std::size_t nodePoolSize
                      , std::vector<uint_t> domains
                      , std::vector<index_t> order = default_oder() );

        /**
         *  \brief Initializes ifMDD manager.
         *
         *  \param varCount Number of variables.
         *  \param nodePoolSize Size of the main node pool.
         *  \param overflowNodePoolSize Size of the additional node pools.
         *  \param domains Domains of variables.
         *  Number at index i is the domain of i-th variable.
         *  \param order Order of variables. Variables are ordered
         *  by their indices by default.
         */
        ifmdd_manager ( std::size_t varCount
                      , std::size_t nodePoolSize
                      , std::size_t overflowNodePoolSize
                      , std::vector<uint_t> domains
                      , std::vector<index_t> order = default_oder() );
    };




    inline bdd_manager::bdd_manager
        ( std::size_t const varCount
        , std::size_t const nodePoolSize
        , std::vector<index_t> order ) :
        bdd_manager (varCount, nodePoolSize, nodePoolSize / 2, std::move(order))
    {
    }

    inline bdd_manager::bdd_manager
        ( std::size_t const varCount
        , std::size_t const nodePoolSize
        , std::size_t const overflowNodePoolSize
        , std::vector<index_t> order ) :
        diagram_manager<void, degrees::fixed<2>, domains::fixed<2>>
            (varCount, nodePoolSize, overflowNodePoolSize, std::move(order))
    {
    }

    template<uint_t P>
    mdd_manager<P>::mdd_manager
        ( std::size_t const varCount
        , std::size_t const nodePoolSize
        , std::vector<index_t> order ) :
        mdd_manager (varCount, nodePoolSize, nodePoolSize / 2, std::move(order))
    {
    }

    template<uint_t P>
    mdd_manager<P>::mdd_manager
        ( std::size_t const varCount
        , std::size_t const nodePoolSize
        , std::size_t const overflowNodePoolSize
        , std::vector<index_t> order ) :
        diagram_manager<void, degrees::fixed<P>, domains::fixed<P>>
            (varCount, nodePoolSize, overflowNodePoolSize, std::move(order))
    {
    }

    inline imdd_manager::imdd_manager
        ( std::size_t const    varCount
        , std::size_t const    nodePoolSize
        , std::vector<uint_t>  domains
        , std::vector<index_t> order ) :
        imdd_manager ( varCount
                     , nodePoolSize
                     , nodePoolSize / 2
                     , std::move(domains)
                     , std::move(order) )
    {
    }

    inline imdd_manager::imdd_manager
        ( std::size_t const    varCount
        , std::size_t const    nodePoolSize
        , std::size_t const    overflowNodePoolSize
        , std::vector<uint_t>  domains
        , std::vector<index_t> order ) :
        diagram_manager<void, degrees::mixed, domains::mixed>
            ( varCount
            , nodePoolSize
            , overflowNodePoolSize
            , std::move(domains)
            , std::move(order) )
    {
    }

    template<uint_t PMax>
    ifmdd_manager<PMax>::ifmdd_manager
        ( std::size_t const    varCount
        , std::size_t const    nodePoolSize
        , std::vector<uint_t>  domains
        , std::vector<index_t> order ) :
        ifmdd_manager
            ( varCount
            , nodePoolSize
            , nodePoolSize / 2
            , std::move(domains)
            , std::move(order) )
    {
    }

    template<uint_t PMax>
    ifmdd_manager<PMax>::ifmdd_manager
        ( std::size_t const    varCount
        , std::size_t const    nodePoolSize
        , std::size_t const    overflowNodePoolSize
        , std::vector<uint_t>  domains
        , std::vector<index_t> order ) :
        diagram_manager<void, degrees::fixed<PMax>, domains::mixed>
            ( varCount
            , nodePoolSize
            , overflowNodePoolSize
            , std::move(domains)
            , std::move(order) )
    {
    }
}

#endif