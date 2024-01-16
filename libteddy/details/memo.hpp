#ifndef LIBTEDDY_DETAILS_MEMO_HPP
#define LIBTEDDY_DETAILS_MEMO_HPP

#include <libteddy/details/node.hpp>
#include <libteddy/details/node_manager.hpp>

#include <unordered_map>

namespace teddy::details
{
/**
 *  \brief TODO
 */
template<class ValueType, class Data, class Degree, class Domain>
class node_memo
{
public:
    using node_t = node<Data, Degree>;

public:
    node_memo (node_manager<Data, Degree, Domain> const& manager) :
        manager_(&manager)
    {
    }

    auto init (node_t* const, int64 const) -> void
    {
    }

    auto finalize (node_t* const root) -> void
    {
        this->finalize_impl(root);
    }

    auto find (node_t* const key) -> ValueType*
    {
        // TODO
        return nullptr;
    }

    auto put (node_t* const key, ValueType const& value) -> void
    {
        // TODO
    }

private:
    auto finalize_impl (node_t* node) -> void
    {
        node->toggle_marked();
        if (node->is_terminal())
        {
            return;
        }

        int32 const nodeDomain = manager_->get_domain(node);
        for (int32 k = 0; k < nodeDomain; ++k)
        {
            node_t* const son = node->get_son(k);
            if (node->is_marked() != son->is_marked())
            {
                this->finalize_impl(son);
            }
        }
    }

private:
    node_manager<Data, Degree, Domain> const* manager_;
};

/**
 *  \brief TODO
 */
template<class ValueType, class Data, class Degree>
class map_memo
{
public:
    using node_t = node<Data, Degree>;

public:
    auto init (node_t* const, int64 const /*nodeCount*/) -> void
    {
        // TODO this one could reserve buckets
    }

    auto finalize (node_t* const) -> void
    {
    }

    auto find (node_t* const key) -> ValueType*
    {
        auto const it = map_.find(key);
        return it != map_.end() ? &(it->second) : nullptr;
    }

    auto put (node_t* const key, ValueType const& value) -> void
    {
        map_.emplace(key, value);
    }

private:
    std::unordered_map<node_t*, ValueType> map_;
};
}

#endif