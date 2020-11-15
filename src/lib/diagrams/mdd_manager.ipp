#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "../utils/more_math.hpp"

#include <numeric>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    mdd_manager<VertexData, ArcData, P>::mdd_manager
        (std::size_t const varCount) :
        mdd_manager {varCount, {}}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    mdd_manager<VertexData, ArcData, P>::mdd_manager
        (std::size_t const varCount, log_v domains) :
        vertexManager_ {varCount},
        domains_       {std::move(domains)}
    {
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
}