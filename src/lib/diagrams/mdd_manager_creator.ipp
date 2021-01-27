#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::constant
        (log_t const val) -> mdd_t
    {
        return mdd_t {vertexManager_.terminal_vertex(val)};
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::variable
        (index_t const i) -> mdd_t
    {
        auto const dom  = this->get_domain(i);
        auto const vals = utils::fill_array<P>(utils::identityv);
        return this->variable_impl(i, vals, dom);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::variables
        (index_v const& is) -> mdd_v
    {
        // Compiler couldn't infer this one. What am I missing?
        using f_t = mdd_t(mdd_manager::*)(index_t const);
        return utils::fmap(is, std::bind_front<f_t>(&mdd_manager::variable, this));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::operator()
        (index_t const i) -> mdd_t
    {
        return this->variable(i);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class LeafVals>
    auto mdd_manager<VertexData, ArcData, P>::variable_impl
        (index_t const i, LeafVals&& vals, std::size_t const domain) -> mdd_t
    {
        auto const first  = std::begin(vals);
        auto const last   = std::next(first, static_cast<std::ptrdiff_t>(domain));
        auto const leaves = utils::fmap_to_array<P>(first, last, [this](auto const lv)
        {
            return vertexManager_.terminal_vertex(static_cast<log_t>(lv));
        });
        return mdd_t {vertexManager_.internal_vertex(i, leaves)};
    }
}