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
        mdd_creator (manager_t* manager);

        auto just_val   (log_t const val)     -> mdd_t;
        auto just_var   (index_t const index) -> mdd_t;
        auto operator() (index_t const index) -> mdd_t;

    private:
        manager_t* manager_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    mdd_creator<VertexData, ArcData, P>::mdd_creator
        (manager_t* manager) :
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
        // TODO map utils::range(0, P - 1) to array
        auto const vals = std::array<log_t, P>{};
        std::iota(std::begin(vals), std::end(vals), log_t {0});
        auto const leaves = utils::map_to_array(vals, [this](auto const lv)
        {
            return this->manager_->terminal_vertex(lv);
        });
        return mdd_t {manager_->internal_vertex(index, leaves)};
    }
}

#endif