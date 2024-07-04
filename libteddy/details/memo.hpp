#ifndef LIBTEDDY_DETAILS_MEMO_HPP
#define LIBTEDDY_DETAILS_MEMO_HPP

#include <libteddy/details/node.hpp>
#include <libteddy/details/node_manager.hpp>
#include <libteddy/details/tools.hpp>

#include <unordered_map>

namespace teddy::details
{
// TODO(michal): out-of-line definition

/**
 *  \brief TODO
 */
template<class ValueType, class Data, class Degree, class Domain>
class in_node_memo
{
public:
    using node_t = node<Data, Degree>;

public:
    in_node_memo(
        node_t* const root,
        node_manager<Data, Degree, Domain> const& manager
    ) :
        root_(root),
        manager_(&manager)
    {
    }

    in_node_memo(in_node_memo&& other) noexcept :
        root_(utils::exchange(other.root_, nullptr)),
        manager_(utils::exchange(other.manager_, nullptr))
    {
    }

    ~in_node_memo()
    {
        if (root_)
        {
            this->finalize_impl(root_);
        }
    }

    auto operator= (in_node_memo&& other) noexcept -> in_node_memo&
    {
        root_    = utils::exchange(other.root_, nullptr);
        manager_ = utils::exchange(other.manager_, nullptr);
    }

    in_node_memo(in_node_memo const&)                     = delete;
    auto operator= (in_node_memo const&) -> in_node_memo& = delete;

    auto find (node_t* const /*key*/) const -> ValueType*
    {
        // TODO(michal): if not marked then nullptr
        return nullptr;
    }

    /**
     *  \brief Puts (key, value) into the memo
     *  \param key key
     *  \param value value
     *  \return Pointer to the place where \p value is stored
     *          Guaranteed to be valid until next call to \c put
     */
    auto put (node_t* const /*key*/, ValueType const& /*value*/) -> void
    {
        // TODO(michal):
    }

private:
    auto finalize_impl (node_t* node) -> void
    {
        node->toggle_marked();
        if (node->is_terminal())
        {
            return;
        }

        int32 const domain = manager_->get_domain(node);
        for (int32 k = 0; k < domain; ++k)
        {
            node_t* const son = node->get_son(k);
            if (node->is_marked() != son->is_marked())
            {
                this->finalize_impl(son);
            }
        }
    }

private:
    node_t* root_;
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
    explicit map_memo(int64 const /*nodeCount*/)
    {
        // TODO(michal): this one could reserve buckets
    }

    auto find (node_t* const key) -> ValueType*
    {
        auto const it = map_.find(key);
        return it != map_.end() ? &(it->second) : nullptr;
    }

    auto find (node_t* const key) const -> ValueType const*
    {
        auto const it = map_.find(key);
        return it != map_.end() ? &(it->second) : nullptr;
    }

    /**
     *  \brief Puts (key, value) into the memo
     *  \param key key
     *  \param value value
     *  \return Pointer to the place where \p value is stored
     *          Guaranteed to be valid until next call to \c put
     */
    auto put (node_t* const key, ValueType const& value) -> ValueType*
    {
        auto const pair = map_.emplace(key, value);
        return &pair.first->second;
    }

private:
    std::unordered_map<node_t*, ValueType> map_;
};
} // namespace teddy::details

#endif