#ifndef LIBTEDDY_DETAILS_NODE_POOL_HPP
#define LIBTEDDY_DETAILS_NODE_POOL_HPP

#include <libteddy/impl/config.hpp>
#include <libteddy/impl/debug.hpp>
#include <libteddy/impl/node.hpp>
#include <libteddy/impl/tools.hpp>

#include <cassert>
#include <cstdlib>

namespace teddy
{
template<class Degree>
class node_pool
{
public:
  using node_t = node<Degree>;

public:
  node_pool(int64 mainPoolSize, int64 overflowPoolSize);
  node_pool(node_pool&& other) noexcept;
  ~node_pool();

  node_pool(node_pool const&)       = delete;
  auto operator= (node_pool const&) = delete;
  auto operator= (node_pool&&)      = delete;

  [[nodiscard]]
  auto get_available_node_count () const -> int64;

  [[nodiscard]]
  auto get_main_pool_size () const -> int64;

  template<class... Args>
  [[nodiscard]]
  auto create (Args&&... args) -> node_t*;

  auto destroy (node_t* node) -> void;

  auto grow () -> void;

private:
  struct pool_item
  {
    node_t* pool_;
    pool_item* next_;
  };

private:
  /**
   *  \brief Allocates new pool of size \p size
   *  \param size Size of the new pool
   *  \param next Next pool in the linked list
   *  \return New pool
   */
  [[nodiscard]]
  static auto allocate_pool (int64 size, pool_item* next) -> pool_item*;

  /**
   *  \brief Destroys all nodes up to last node and deallocates the pool
   *  \return Pointer to the next pool
   */
  static auto deallocate_pool (pool_item* poolPtr, node_t* lastNode)
    -> pool_item*;

private:
  pool_item* pools_;
  node_t* nextPoolNode_;
  node_t* freeNodes_;
  int64 mainPoolSize_;
  int64 extraPoolSize_;
  int64 availableNodeCount_;
};

template<class Degree>
node_pool<Degree>::node_pool(
  int64 const mainPoolSize,
  int64 const overflowPoolSize
) :
  pools_(allocate_pool(mainPoolSize, nullptr)),
  nextPoolNode_(pools_->pool_),
  freeNodes_(nullptr),
  mainPoolSize_(mainPoolSize),
  extraPoolSize_(overflowPoolSize),
  availableNodeCount_(mainPoolSize)
{
#ifdef LIBTEDDY_VERBOSE
  debug::out(
    "node_pool::node_pool\tAllocating initial pool with size ",
    mainPoolSize_,
    "\n"
  );
#endif
}

template<class Degree>
node_pool<Degree>::node_pool(node_pool&& other) noexcept :
  pools_(utils::exchange(other.mainPool_, nullptr)),
  nextPoolNode_(utils::exchange(other.nextPoolNode_, nullptr)),
  freeNodes_(utils::exchange(other.freeNodes_, nullptr)),
  mainPoolSize_(utils::exchange(other.mainPoolSize_, -1)),
  extraPoolSize_(utils::exchange(other.extraPoolSize_, -1)),
  availableNodeCount_(utils::exchange(other.availableNodeCount_, -1))
{
}

template<class Degree>
node_pool<Degree>::~node_pool()
{
  /*
   *  This is the currently used pool.
   */
  pools_ = deallocate_pool(pools_, nextPoolNode_);

  /*
   *  If there are more pools with next pool they are extra pools.
   */
  while (pools_ && pools_->next_)
  {
    node_t* const lastNode = pools_->pool_ + extraPoolSize_;
    pools_                 = deallocate_pool(pools_, lastNode);
  }

  /**
   *  This must be the main pool.
   */
  if (pools_)
  {
    node_t* const lastNode = pools_->pool_ + mainPoolSize_;
    pools_                 = deallocate_pool(pools_, lastNode);
  }
}

template<class Degree>
auto node_pool<Degree>::get_available_node_count() const -> int64
{
  return availableNodeCount_;
}

template<class Degree>
auto node_pool<Degree>::get_main_pool_size() const -> int64
{
  return mainPoolSize_;
}

template<class Degree>
template<class... Args>
auto node_pool<Degree>::create(Args&&... args) -> node_t* // NOLINT
{
  assert(availableNodeCount_ > 0);
  --availableNodeCount_;

  node_t* nodePtr = nullptr;
  if (freeNodes_)
  {
    nodePtr    = freeNodes_;
    freeNodes_ = freeNodes_->get_next();
    nodePtr->~node_t();
  }
  else
  {
    nodePtr = nextPoolNode_;
    ++nextPoolNode_;
  }

  return static_cast<node_t*>(::new (nodePtr) node_t(TEDDY_FORWARD(args)...));
}

template<class Degree>
auto node_pool<Degree>::destroy(node_t* const node) -> void
{
  ++availableNodeCount_;
  node->set_next(freeNodes_);
  freeNodes_ = node;
}

template<class Degree>
auto node_pool<Degree>::grow() -> void
{
#ifdef LIBTEDDY_VERBOSE
  debug::out(
    "node_pool::grow\tallocating overflow pool with size ",
    extraPoolSize_,
    "\n"
  );
#endif

  pools_        = allocate_pool(extraPoolSize_, pools_);
  nextPoolNode_ = pools_->pool_;
  availableNodeCount_ += extraPoolSize_;
}

template<class Degree>
auto node_pool<Degree>::allocate_pool(int64 const size, pool_item* const next)
  -> pool_item*
{
  return new pool_item {
    static_cast<node_t*>(std::malloc(as_usize(size) * sizeof(node_t))),
    next
  };
}

template<class Degree>
auto node_pool<Degree>::deallocate_pool(
  pool_item* const pool,
  node_t* const lastNode
) -> pool_item*
{
  node_t* node = pool->pool_;
  while (node < lastNode)
  {
    node->~node_t();
    ++node;
  }
  pool_item* next = pool->next_;
  std::free(pool->pool_);
  delete pool;
  return next;
}
} // namespace teddy

#endif