#ifndef MIX_DD_MDD_MANAGER_HPP
#include "../mdd_manager.hpp"
#endif

#include "../data_structures/peekable_stack.hpp"

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
    template<class InputIt>
    auto mdd_manager<VertexData, ArcData, P>::from_vector
        (InputIt first, InputIt last) -> mdd_t
    {
        using stack_frame = struct { vertex_t* vertex; level_t level; };
        auto stack = ds::peekable_stack<stack_frame>();

        auto const lastLevel     = this->get_last_level();
        auto const lastIndex     = this->get_index(lastLevel);
        auto const lastVarDomain = this->get_domain(this->get_index(lastLevel));

        auto const shrink_stack = [=, &stack]()
        {
            auto const vertices_available = [&stack](auto const level)
            {
                auto count  = 0u;
                auto offset = 0u;
                while (offset < stack.size() && stack.peek(offset).level == level)
                {
                    ++count;
                    ++offset;
                }
                return count;
            };

            for (;;)
            {
                auto const currentLevel = stack.top().level;
                if (0 == currentLevel)
                {
                    break;
                }

                auto const newIndex     = this->get_index(currentLevel - 1);
                auto const newDomain    = this->get_domain(newIndex);

                if (vertices_available(currentLevel) < newDomain)
                {
                    break;
                }

                auto const newSons = utils::fill_array_n<P>(newDomain, [=, &stack](auto const o)
                {
                    return stack.peek(newDomain - o - 1).vertex;
                });
                auto const newVertex = vertexManager_.internal_vertex(newIndex, newSons);
                stack.pop_n(newDomain);
                stack.push(stack_frame {newVertex, currentLevel - 1});
            }
        };

        auto const new_bottom_vertex = [=, this](auto const& vals)
        {
            auto const leaves = utils::fmap_to_array(vals, [=, this](auto const val)
            {
                return vertexManager_.terminal_vertex(val);
            });
            return vertexManager_.internal_vertex(lastIndex, leaves);
        };

        while (first != last)
        {
            auto const vals = utils::fill_array_n<P>(lastVarDomain, [&first](auto const)
            {
                return *first++;
            });

            auto const vertex = new_bottom_vertex(vals);
            stack.push(stack_frame {vertex, lastLevel});
            shrink_stack();
        }

        return mdd_t {stack.top().vertex};
    }

    template<class VertexData, class ArcData, std::size_t P>
    template<class Range>
    auto mdd_manager<VertexData, ArcData, P>::from_vector
        (Range&& range) -> mdd_t
    {
        return this->from_vector(std::begin(range), std::end(range));
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