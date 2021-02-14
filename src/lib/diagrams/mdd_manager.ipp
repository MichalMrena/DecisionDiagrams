#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "../utils/more_math.hpp"
#include "../utils/more_assert.hpp"

#include <numeric>
#include <memory>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    mdd_manager<VertexData, ArcData, P>::mdd_manager
        (std::size_t const varCount) :
        manager_ {varCount}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::set_domains
        (log_v domains) -> void
    {
        utils::runtime_assert(this->var_count() == domains.size(), "mdd_manager::set_domains: Domains vector size must match var count.");
        domains_ = std::move(domains);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::set_order
        (index_v levelToIndex) -> void
    {
        manager_.set_order(std::move(levelToIndex));
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

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::var_count
        () const -> std::size_t
    {
        return manager_.get_var_count();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_index
        (level_t const l) const -> index_t
    {
        return manager_.get_index(l);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_level
        (index_t const i) const -> level_t
    {
        return manager_.get_level(i);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_domain
        (index_t const i) const -> log_t
    {
        return domains_.size() ? domains_[i] : P;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_last_level
        () const -> level_t
    {
        return manager_.get_last_level();
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_manager<VertexData, ArcData, P>::get_domain_product
        () const -> std::size_t
    {
        return domains_.size() ? std::reduce( std::begin(domains_), std::end(domains_)
                                            , std::size_t {1}, std::multiplies() )
                               : utils::int_pow(P, this->var_count());
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