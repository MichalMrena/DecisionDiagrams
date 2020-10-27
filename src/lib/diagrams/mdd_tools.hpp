#ifndef MIX_DD_MDD_TOOLS_HPP
#define MIX_DD_MDD_TOOLS_HPP

#include "typedefs.hpp"
#include "mdd.hpp"
#include "vertex_manager.hpp"
#include "../utils/more_functional.hpp"
#include "../utils/more_vector.hpp"
#include "../utils/print.hpp"
#include "../utils/string_utils.hpp"

#include <ostream>
#include <string>

namespace mix::dd
{
    template<class VertexData, class ArcData, std::size_t P>
    class mdd_tools
    {
    public:
        using manager_t = vertex_manager<VertexData, ArcData, P>;
        using vertex_t  = vertex<VertexData, ArcData, P>;
        using mdd_t     = mdd<VertexData, ArcData, P>;

    public:
        mdd_tools (manager_t* const manager);

        auto vertex_count (mdd_t const& diagram) const -> std::size_t;

        auto to_dot_graph (std::ostream& ost, mdd_t const& diagram) const -> void;

        template<class VertexOp>
        auto traverse_pre (mdd_t const& diagram, VertexOp&& op) const -> void;

    private:
        using vertex_v  = std::vector<vertex_t*>;
        using vertex_vv = std::vector<vertex_v>;

    private:
        auto fill_levels (mdd_t const& diagram) const -> vertex_vv;

        template<class VertexOp>
        auto traverse_pre (vertex_t* const v, VertexOp&& op) const -> void;

    private:
        manager_t* manager_;
    };

    template<class VertexData, class ArcData, std::size_t P>
    mdd_tools<VertexData, ArcData, P>::mdd_tools
        (manager_t* const manager) :
        manager_ {manager}
    {
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_tools<VertexData, ArcData, P>::vertex_count
        (mdd_t const& diagram) const -> std::size_t
    {
        auto count = 0;
        this->traverse_pre(diagram, [&count](auto){ ++count; });
        return count;
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_tools<VertexData, ArcData, P>::to_dot_graph
        (std::ostream& ost, mdd_t const& diagram) const -> void
    {
        using std::to_string;
        using utils::concat;
        using utils::concat_range;
        using utils::EOL;

        auto labels       = std::vector<std::string>();
        auto arcs         = std::vector<std::string>();
        auto ranks        = std::vector<std::string>();
        auto squareShapes = std::vector<std::string>();
        auto make_label   = [this](auto const v)
        {
            using std::to_string;
            using traits_t = log_val_traits<P>;
            return this->manager_->is_leaf(v) ? traits_t::to_string(this->manager_->get_value(v))
                                              : "x" + to_string(v->get_index());
        };
        auto to_id = [](auto const v)
        {
            return reinterpret_cast<std::uintptr_t>(v);
        };

        auto const levels = this->fill_levels(diagram);
        for (auto const& level : levels)
        {
            if (level.empty())
            {
                continue;
            }

            auto ranksLocal = std::vector<std::string> {"{rank = same;"};
            for (auto const v : level)
            {
                labels.emplace_back(concat(to_id(v) , " [label = " , make_label(v) , "];"));
                ranksLocal.emplace_back(concat(to_id(v) , ";"));

                if (!this->manager_->is_leaf(v))
                {
                    for (auto val = 0u; val < P; ++val)
                    {
                        if constexpr (2 == P)
                        {
                            auto const style = 0 == val ? "dashed" : "solid";
                            arcs.emplace_back(concat(to_id(v) , " -> " , to_id(v->get_son(val)) , " [style = " , style , "];"));
                        }
                        else
                        {
                            arcs.emplace_back(concat(to_id(v) , " -> " , to_id(v->get_son(val)) , " [label = \"" , val , "\"];"));
                        }
                    }
                }
            }

            ranksLocal.emplace_back("}");
            ranks.emplace_back(concat_range(ranksLocal, " "));
        }

        for (auto const leaf : levels.back())
        {
            squareShapes.emplace_back(to_string(to_id(leaf)));
        }
        squareShapes.emplace_back(";");

        ost << concat( "digraph D {"                                                      , EOL
                     , "    " , "node [shape = square] ", concat_range(squareShapes, " ") , EOL
                     , "    " , "node [shape = circle];"                                  , EOL , EOL
                     , "    " , concat_range(labels, concat(EOL, "    "))                 , EOL , EOL
                     , "    " , concat_range(arcs,   concat(EOL, "    "))                 , EOL , EOL
                     , "    " , concat_range(ranks,  concat(EOL, "    "))                 , EOL
                     , "}"                                                                , EOL );
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_tools<VertexData, ArcData, P>::traverse_pre
        (mdd_t const& diagram, VertexOp&& op) const -> void
    {
        this->traverse_pre(diagram.get_root(), std::forward<VertexOp>(op));
        this->traverse_pre(diagram.get_root(), utils::no_op);
    }

    template<class VertexData, class ArcData, std::size_t P>
    auto mdd_tools<VertexData, ArcData, P>::fill_levels
        (mdd_t const& diagram) const -> vertex_vv
    {
        auto levels = vertex_vv(1 + this->manager_->get_var_count());

        this->traverse_pre(diagram, [&, this](auto const v)
        {
            levels[this->manager_->get_level(v)].emplace_back(v);
        });

        return levels;
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class VertexOp>
    auto mdd_tools<VertexData, ArcData, P>::traverse_pre
        (vertex_t* const v, VertexOp&& op) const -> void
    {
        v->toggle_mark();
        op(v);

        for (auto i = 0u; i < P; ++i)
        {
            auto const son = v->get_son(i);
            if (son && v->get_mark() != son->get_mark())
            {
                this->traverse_pre(son, std::forward<VertexOp>(op));
            }
        }
    }
}

#endif