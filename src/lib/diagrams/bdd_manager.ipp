#ifndef MIX_DD_BDD_MANAGER_HPP
#include "../bdd_manager.hpp"
#endif

#include "operators.hpp"

namespace mix::dd
{
    template<class VertexData, class ArcData>
    bdd_manager<VertexData, ArcData>::bdd_manager
        (std::size_t const varCount) :
        base {varCount}
    {
    }

    auto make_bdd_manager(std::size_t const varCount)
    {
        return bdd_manager<double, void>(varCount);
    }

    namespace impl
    {
        template<class VertexData, class ArcData>
        inline auto bddManager_ = static_cast<bdd_manager<VertexData, ArcData>*>(nullptr);

        template<class VertexData, class ArcData>
        auto m_ref () -> bdd_manager<VertexData, ArcData>&
        {
            return *bddManager_<VertexData, ArcData>;
        }
    }

    template<class VertexData, class ArcData>
    auto register_manager(bdd_manager<VertexData, ArcData>& manager)
    {
        impl::bddManager_<VertexData, ArcData> = &manager;
    }

    template<class VertexData, class ArcData>
    auto operator&& ( mdd<VertexData, ArcData, 2> const& lhs
                    , mdd<VertexData, ArcData, 2> const& rhs ) -> mdd<VertexData, ArcData, 2>
    {
        return impl::m_ref<VertexData, ArcData>().template apply<AND>(lhs, rhs);
    }

    template<class VertexData, class ArcData>
    auto operator* ( mdd<VertexData, ArcData, 2> const& lhs
                   , mdd<VertexData, ArcData, 2> const& rhs ) -> mdd<VertexData, ArcData, 2>
    {
        return impl::m_ref<VertexData, ArcData>().template apply<AND>(lhs, rhs);
    }

    template<class VertexData, class ArcData>
    auto operator|| ( mdd<VertexData, ArcData, 2> const& lhs
                    , mdd<VertexData, ArcData, 2> const& rhs ) -> mdd<VertexData, ArcData, 2>
    {
        return impl::m_ref<VertexData, ArcData>().template apply<OR>(lhs, rhs);
    }

    template<class VertexData, class ArcData>
    auto operator+ ( mdd<VertexData, ArcData, 2> const& lhs
                   , mdd<VertexData, ArcData, 2> const& rhs ) -> mdd<VertexData, ArcData, 2>
    {
        return impl::m_ref<VertexData, ArcData>().template apply<OR>(lhs, rhs);
    }

    template<class VertexData, class ArcData>
    auto operator^ ( mdd<VertexData, ArcData, 2> const& lhs
                   , mdd<VertexData, ArcData, 2> const& rhs ) -> mdd<VertexData, ArcData, 2>
    {
        return impl::m_ref<VertexData, ArcData>().template apply<XOR>(lhs, rhs);
    }

    template<class VertexData, class ArcData>
    auto operator! ( mdd<VertexData, ArcData, 2> const& lhs ) -> mdd<VertexData, ArcData, 2>
    {
        return impl::m_ref<VertexData, ArcData>().negate(lhs);
    }
}