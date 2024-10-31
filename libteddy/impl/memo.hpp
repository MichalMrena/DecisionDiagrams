#ifndef LIBTEDDY_DETAILS_MEMO_HPP
#define LIBTEDDY_DETAILS_MEMO_HPP

#include <libteddy/impl/node.hpp>
#include <libteddy/impl/node_manager.hpp>
#include <libteddy/impl/tools.hpp>

#include <unordered_map>

namespace teddy::details
{
// TODO(michal): out-of-line definition

/**
 *  \brief TODO
 */
template<class ValueType, class Degree, class Domain>
class in_node_memo
{
public:
  using node_t = node<Degree>;

public:
  in_node_memo(
    node_t* const root,
    node_manager<Degree, Domain> const& manager
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

  auto find (node_t* const key) -> ValueType*
  {
    return key->is_marked() ? &key->template get_data<ValueType>() : nullptr;
  }

  auto find (node_t* const key) const -> ValueType const*
  {
    return key->is_marked() ? &key->template get_data<ValueType>() : nullptr;
  }

  /**
   *  \brief Puts (key, value) into the memo
   *  \param key key
   *  \param value value
   *  \return Pointer to the place where \p value is stored
   *          Guaranteed to be valid until next call to \c put
   */
  auto put (node_t* const key, ValueType const& value) -> void
  {
    key->set_marked();
    key->template get_data<ValueType>() = value;
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
  node_manager<Degree, Domain> const* manager_;
};

/**
 *  \brief TODO
 */
template<class ValueType, class Degree>
class map_memo
{
public:
  using node_t = node<Degree>;

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

  // TODO(michal): doc
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
   */
  auto put (node_t* const key, ValueType const& value) -> ValueType*
  {
    auto const pair = map_.emplace(key, value);
    return &pair.first->second;
  }

private:
  /**
   *  No that this has to be node-based map implementation.
   *  Ponters to data pairs must stay valid even after rehashing.
   */
  std::unordered_map<node_t*, ValueType> map_;
};
} // namespace teddy::details

#endif