#ifndef LIBTEDDY_CORE_HPP
#define LIBTEDDY_CORE_HPP

#include <libteddy/details/diagram_manager.hpp>
#include <libteddy/details/pla_file.hpp>

namespace teddy
{
using default_oder = std::vector<int32>;

/**
 *  \class bdd_manager
 *  \brief Diagram manager for Binary Decision Diagrams
 */
struct bdd_manager :
    public diagram_manager<void, degrees::fixed<2>, domains::fixed<2>>
{
    /**
     *  \brief Initializes BDD manager.
     *
     *  \param varCount Number of variables.
     *  \param nodePoolSize Size of the main node pool.
     *  \param order Order of variables. Variables are ordered
     *  by their indices by default.
     */
    bdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        std::vector<int32> order = default_oder()
    );

    /**
     *  \brief Initializes BDD manager.
     *
     *  \param varCount Number of variables.
     *  \param nodePoolSize Size of the main node pool.
     *  \param overflowNodePoolSize Size of the additional node pools.
     *  \param order Order of variables. Variables are ordered
     *  by their indices by default.
     */
    bdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order = default_oder()
    );
};

/**
 *  \class mdd_manager
 *  \brief Diagram manager for Multi-valued Decision Diagrams
 *  \tparam M domain of variables
 */
template<int32 M>
struct mdd_manager :
    public diagram_manager<void, degrees::fixed<M>, domains::fixed<M>>
{
    /**
     *  \brief Initializes MDD manager
     *  \param varCount Number of variables
     *  \param nodePoolSize Size of the main node pool
     *  \param order Order of variables. Variables are ordered
     *  by their indices by default
     */
    mdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        std::vector<int32> order = default_oder()
    );

    /**
     *  \brief Initializes MDD manager
     *  \param varCount Number of variables
     *  \param nodePoolSize Size of the main node pool
     *  \param overflowNodePoolSize Size of the additional node pools
     *  \param order Order of variables. Variables are ordered
     *  by their indices by default.
     */
    mdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> order = default_oder()
    );
};

/**
 *  \class imdd_manager
 *  \brief Diagram manager for (integer) Multi-valued Decision Diagrams (iMDDs)
 *
 *  Unlike \c mdd_manager variables in iMDDs can have
 *  different domains. Node representation is less compact in
 *  this case since the number of sons of a node is not known
 *  at compile time.
 */
struct imdd_manager :
    public diagram_manager<void, degrees::mixed, domains::mixed>
{
    /**
     *  \brief Initializes iMDD manager
     *  \param varCount Number of variables
     *  \param nodePoolSize Size of the main node pool
     *  \param domains Domains of variables
     *  Number at index i is the domain of i-th variable.
     *  \param order Order of variables. Variables are ordered
     *  by their indices by default
     */
    imdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        std::vector<int32> domains,
        std::vector<int32> order = default_oder()
    );

    /**
     *  \brief Initializes iMDD manager
     *  \param varCount Number of variables
     *  \param nodePoolSize Size of the main node pool
     *  \param overflowNodePoolSize Size of the additional node pools
     *  \param domains Domains of variables
     *  Number at index i is the domain of i-th variable.
     *  \param order Order of variables. Variables are ordered
     *  by their indices by default
     */
    imdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> domains,
        std::vector<int32> order = default_oder()
    );
};

/**
 *  \class ifmdd_manager
 *  \brief Diagram manager (integer) Multi-valued Decision Diagrams (iMDDs)
 *
 *  Unlike \c mdd_manager variables in ifMDDs can have
 *  different domains. However, node representation is the same
 *  since the maximal number of sons is known at compile time.
 *  Note that some memory might be allocated but unused because
 *  each node allocates space for \p PMax sons regardles of its domain.
 *
 *  \tparam M maximum of the sizes of domains of variables
 */
template<int32 M>
struct ifmdd_manager :
    public diagram_manager<void, degrees::fixed<M>, domains::mixed>
{
    /**
     *  \brief Initializes ifMDD manager
     *  \param varCount Number of variables
     *  \param nodePoolSize Size of the main node pool
     *  \param domains Domains of variables
     *  Number at index i is the domain of i-th variable.
     *  \param order Order of variables. Variables are ordered
     *  by their indices by default
     */
    ifmdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        std::vector<int32> domains,
        std::vector<int32> order = default_oder()
    );

    /**
     *  \brief Initializes ifMDD manager.
     *  \param varCount Number of variables
     *  \param nodePoolSize Size of the main node pool
     *  \param overflowNodePoolSize Size of the additional node pools
     *  \param domains Domains of variables
     *  Number at index i is the domain of i-th variable.
     *  \param order Order of variables. Variables are ordered
     *  by their indices by default
     */
    ifmdd_manager(
        int32 varCount,
        int64 nodePoolSize,
        int64 overflowNodePoolSize,
        std::vector<int32> domains,
        std::vector<int32> order = default_oder()
    );
};

inline bdd_manager::bdd_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    std::vector<int32> order
) :
    bdd_manager(varCount, nodePoolSize, nodePoolSize / 2, std::move(order))
{
}

inline bdd_manager::bdd_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order
) :
    diagram_manager<void, degrees::fixed<2>, domains::fixed<2>>(
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        std::move(order)
    )
{
}

template<int32 M>
mdd_manager<M>::mdd_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    std::vector<int32> order
) :
    mdd_manager(varCount, nodePoolSize, nodePoolSize / 2, std::move(order))
{
}

template<int32 M>
mdd_manager<M>::mdd_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> order
) :
    diagram_manager<void, degrees::fixed<M>, domains::fixed<M>>(
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        std::move(order)
    )
{
}

inline imdd_manager::imdd_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    std::vector<int32> domains,
    std::vector<int32> order
) :
    imdd_manager(
        varCount,
        nodePoolSize,
        nodePoolSize / 2,
        std::move(domains),
        std::move(order)
    )
{
}

inline imdd_manager::imdd_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> domains,
    std::vector<int32> order
) :
    diagram_manager<void, degrees::mixed, domains::mixed>(
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        std::move(domains),
        std::move(order)
    )
{
}

template<int32 M>
ifmdd_manager<M>::ifmdd_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    std::vector<int32> domains,
    std::vector<int32> order
) :
    ifmdd_manager(
        varCount,
        nodePoolSize,
        nodePoolSize / 2,
        std::move(domains),
        std::move(order)
    )
{
}

template<int32 PMax>
ifmdd_manager<PMax>::ifmdd_manager(
    int32 const varCount,
    int64 const nodePoolSize,
    int64 const overflowNodePoolSize,
    std::vector<int32> domains,
    std::vector<int32> order
) :
    diagram_manager<void, degrees::fixed<PMax>, domains::mixed>(
        varCount,
        nodePoolSize,
        overflowNodePoolSize,
        std::move(domains),
        std::move(order)
    )
{
}
} // namespace teddy

#endif