#ifndef MIX_DD_MDD_CREATOR_HPP
#define MIX_DD_MDD_CREATOR_HPP

#include "bdd.hpp"
#include "../utils/more_type_traits.hpp"
#include "../utils/alloc_manager.hpp"
#include "../utils/more_iterator.hpp"
#include "../utils/more_algorithm.hpp"

#include <stdexcept>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    class mdd_creator
    {
    public:
        using mdd_t    = std::conditional_t< 2 == P
                                           , bdd<VertexData, ArcData, Allocator>
                                           , mdd<VertexData, ArcData, P, Allocator> >;
        using vertex_t = typename mdd_t::vertex_t;
        using log_t    = typename mdd_t::log_t;

    public:
        mdd_creator (Allocator const& alloc = Allocator {});

        auto just_val   (log_t   const val)   -> mdd_t;
        auto just_var   (index_t const index) -> mdd_t;
        auto operator() (index_t const index) -> mdd_t;
        auto just_var   (index_t const index, std::size_t const domain) -> mdd_t;
        auto operator() (index_t const index, std::size_t const domain) -> mdd_t;
        auto just_vars  (std::vector<log_t> const& domains)             -> std::vector<mdd_t>;

    protected:
        using leaf_val_map = typename mdd_t::leaf_val_map;
        using manager_t    = utils::alloc_manager<Allocator>;

    protected:
        manager_t manager_;
    };

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd_creator<VertexData, ArcData, P, Allocator>::mdd_creator
        (Allocator const& alloc) :
        manager_ {alloc}
    {
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_creator<VertexData, ArcData, P, Allocator>::just_var
        (index_t const index) -> mdd_t
    {
        return this->just_var(index, P);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_creator<VertexData, ArcData, P, Allocator>::operator()
        (index_t const index) -> mdd_t
    {
        return (*this)(index, P);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_creator<VertexData, ArcData, P, Allocator>::just_var
        (index_t const index, std::size_t const domain) -> mdd_t
    {
        if (domain > P)
        {
            throw std::logic_error("Invalid domain.");
        }

        auto const root = manager_.create(id_t {0}, index);
        auto id         = 1;
        auto leafToVal  = leaf_val_map {};

        for (auto val = 0u; val < domain; ++val)
        {
            auto const leaf = manager_.create(id++, index + 1);
            leafToVal.emplace(leaf, val);
            root->son(val) = leaf;
        }

        if (domain < P)
        {
            auto constexpr ND = log_val_traits<P>::nodomain;
            auto const ndleaf = manager_.create(id++, index + 1);
            leafToVal.emplace(ndleaf, ND);
            for (auto val = domain; val < P; ++val)
            {
                root->son(val) = ndleaf;
            }
        }

        return mdd_t { root
                     , std::move(leafToVal)
                     , manager_.get_alloc() };
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_creator<VertexData, ArcData, P, Allocator>::operator()
        (index_t const index, std::size_t const domain) -> mdd_t
    {
        return this->just_var(index, domain);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_creator<VertexData, ArcData, P, Allocator>::just_val
        (log_t const val) -> mdd_t
    {
        auto const root = manager_.create(id_t {0}, index_t {0});
        auto leafToVal  = leaf_val_map
        {
            {root, val}
        };

        return mdd_t { root
                     , std::move(leafToVal)
                     , manager_.get_alloc() };
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_creator<VertexData, ArcData, P, Allocator>::just_vars
        (std::vector<log_t> const& domains) -> std::vector<mdd_t>
    {
        auto const zs = utils::zip(utils::range(0u, domains.size()), domains);
        return utils::map(zs, domains.size(), [this](auto&& pair)
        {
            auto const [i, d] = pair;
            return this->just_var(i, d);
        });
    }
}

#endif