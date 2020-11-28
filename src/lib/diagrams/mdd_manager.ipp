#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "../utils/more_math.hpp"
#include "../utils/more_assert.hpp"

#include <numeric>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    mdd_manager<VertexData, ArcData, P>::mdd_manager
        (std::size_t const varCount) :
        vertexManager_ {varCount}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::set_domains
        (log_v domains) -> void
    {
        utils::runtime_assert( this->get_var_count() == domains.size()
                             , "mdd_manager::set_domains: Domains vector size must match var count." );
        domains_ = std::move(domains);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::set_order
        (index_v levels) -> void
    {
        utils::runtime_assert( applyMemo_.empty()
                             , "mdd_manager::set_order: Cache must be empty." );
        vertexManager_.set_order(std::move(levels));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_var_count
        () const -> std::size_t
    {
        return vertexManager_.get_var_count();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_domain
        (index_t const i) const -> log_t
    {
        return domains_.size() ? domains_[i] : P;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_domain_product
        () const -> std::size_t
    {
        return domains_.size() ? std::reduce( std::begin(domains_), std::end(domains_)
                                            , std::size_t {1}, std::multiplies() )
                               : utils::int_pow(P, this->get_var_count());
    }

    template<std::size_t P>
    auto make_mdd_manager(std::size_t const varCount)
    {
        return mdd_manager<double, void, P>(varCount);
    }
}