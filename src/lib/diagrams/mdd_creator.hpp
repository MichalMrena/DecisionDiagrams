#ifndef MIX_DD_MDD_CREATOR_HPP
#define MIX_DD_MDD_CREATOR_HPP

#include "mdd.hpp"
#include "typedefs.hpp"
#include "vertex_manager.hpp"
#include "../utils/more_algorithm.hpp"

#include <cstddef>
#include <numeric>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_creator
    {
    public:
        using mdd_t     = mdd<VertexData, ArcData, P>;
        using log_t     = typename log_val_traits<P>::type;
        using manager_t = vertex_manager<VertexData, ArcData, P>;

    public:
        mdd_creator (manager_t* const manager);

        auto just_val   (log_t const val)     -> mdd_t;
        auto just_var   (index_t const index) -> mdd_t;
        auto operator() (index_t const index) -> mdd_t;

    protected:
        template<class LeafVals>
        auto just_var_impl ( index_t const index
                           , LeafVals&& vals ) -> mdd_t;

    private:
        manager_t* manager_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    mdd_creator<VertexData, ArcData, P>::mdd_creator
        (manager_t* const manager) :
        manager_ {manager}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_creator<VertexData, ArcData, P>::just_val
        (log_t const val) -> mdd_t
    {
        return mdd_t {manager_->terminal_vertex(val)};
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_creator<VertexData, ArcData, P>::just_var
        (index_t const index) -> mdd_t
    {
        return this->just_var_impl(index, utils::range(log_t {0}, log_t {P}));
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_creator<VertexData, ArcData, P>::operator()
        (index_t const index) -> mdd_t
    {
        return this->just_var(index);
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class LeafVals>
    auto mdd_creator<VertexData, ArcData, P>::just_var_impl
        (index_t const index, LeafVals&& vals) -> mdd_t
    {
        auto const leaves = utils::map_to_array<P>(vals, [this](auto const lv)
        {
            return this->manager_->terminal_vertex(lv);
        });
        return mdd_t {manager_->internal_vertex(index, leaves)};
    }
}

#endif