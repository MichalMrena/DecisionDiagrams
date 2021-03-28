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
        (std::size_t const varCount, std::size_t const vertexCount) :
        manager_ (varCount, vertexCount)
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::set_domains
        (log_v domains) -> void
    {
        utils::runtime_assert(manager_.get_var_count() == domains.size(), "mdd_manager::set_domains: Domains vector size must match var count.");
        manager_.set_domains(std::move(domains));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::set_order
        (index_v levelToIndex) -> void
    {
        manager_.set_order(std::move(levelToIndex));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_order
        () const -> index_v const&
    {
        return manager_.get_order();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::set_cache_ratio
        (std::size_t denominator) -> void
    {
        manager_.set_cache_ratio(denominator);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::set_pool_ratio
        (std::size_t denominator) -> void
    {
        manager_.set_pool_ratio(denominator);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::swap_vars
        (index_t const i) -> void
    {
        manager_.swap_vars(i);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::clear
        () -> void
    {
        manager_.clear();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::collect_garbage
        () -> void
    {
        manager_.collect_garbage();
    }

    template<std::size_t P>
    auto make_mdd_manager(std::size_t const varCount)
    {
        return mdd_manager<double, void, P>(varCount);
    }

    namespace mm_impl
    {
        template<class VertexData, class ArcData, std::size_t P>
        inline auto manager_ = static_cast<mdd_manager<VertexData, ArcData, P>*>(nullptr);

        template<class VertexData, class ArcData, std::size_t P>
        auto m_ref() -> mdd_manager<VertexData, ArcData, P>&
        {
            return *manager_<VertexData, ArcData, P>;
        }
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto register_manager(mdd_manager<VertexData, ArcData, P>& m)
    {
        mm_impl::manager_<VertexData, ArcData, P> = std::addressof(m);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator&& ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<AND>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator|| ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<OR>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator^ ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<XOR>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator== ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<EQUAL_TO>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator!= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<NOT_EQUAL_TO>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator< ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<LESS>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator<= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<LESS_EQUAL>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator> ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<GREATER>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator>= ( mdd<VertexData, ArcData, P> const& lhs
                    , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<GREATER_EQUAL>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator+ ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<PLUS>(lhs, rhs);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto operator* ( mdd<VertexData, ArcData, P> const& lhs
                   , mdd<VertexData, ArcData, P> const& rhs ) -> mdd<VertexData, ArcData, P>
    {
        return mm_impl::m_ref<VertexData, ArcData, P>().template apply<MULTIPLIES>(lhs, rhs);
    }
}