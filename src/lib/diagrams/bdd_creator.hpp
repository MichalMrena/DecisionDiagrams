#ifndef MIX_DD_BDD_CREATOR_HPP
#define MIX_DD_BDD_CREATOR_HPP

#include "bdd.hpp"
#include "bdd_manipulator.hpp"
#include "mdd_creator.hpp"
#include "pla_file.hpp"
#include "../utils/hash.hpp"
#include "../utils/more_vector.hpp"
#include "../utils/more_algorithm.hpp"
#include "../data_structures/peekable_stack.hpp"

#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>

namespace mix::dd
{
    enum class fold_e {left, tree};

    template<class VertexData, class ArcData, class Allocator>
    class bdd_creator : public mdd_creator<VertexData, ArcData, 2, Allocator>
    {
    public:
        using bdd_t         = bdd<VertexData, ArcData, Allocator>;
        using leaf_val_map  = typename bdd_t::leaf_val_map;
        using vertex_t      = typename bdd_t::vertex_t;
        using manipulator_t = bdd_manipulator<VertexData, ArcData, Allocator>;

    public:
        explicit bdd_creator (Allocator const& alloc = Allocator {});

        auto product (std::vector<bool_var> const& vars) -> bdd_t;
        auto sum     (std::vector<bool_var> const& vars) -> bdd_t;

        template<class InputIt>
        auto product (InputIt varsIt, InputIt varsEnd) -> bdd_t;

        template<class InputIt>
        auto sum (InputIt varsIt, InputIt varsEnd) -> bdd_t;

        auto from_pla (pla_file const& file, fold_e const mm = fold_e::tree) -> std::vector<bdd_t>;

        template<class Range>
        auto from_vector (Range const& range) -> bdd_t;

        template<class InputIt>
        auto from_vector (InputIt first, InputIt last) -> bdd_t;

    private:
        template<class InputIt>
        auto concat_impl (InputIt varsIt, InputIt varsEnd, bool_t absorbingVal) -> bdd_t;

        auto or_merge        (std::vector<bdd_t> diagrams, fold_e) -> bdd_t;
        auto line_to_product (pla_line const& line)                -> bdd_t;

        auto try_insert ( vertex_t* const falseSon
                        , vertex_t* const trueSon
                        , index_t   const level ) -> vertex_t*; 

    private:
        struct stack_frame
        {
            vertex_t* vertexPtr;
            index_t   level;
            stack_frame( vertex_t* const pVertexPtr
                       , index_t   const pLevel ) :
                vertexPtr {pVertexPtr},
                level     {pLevel} {}
        };

    private:
        using arc_t      = typename bdd_t::arc_t;
        using arc_arr_t  = typename vertex_t::star_arr;
        using vertex_key = std::pair<vertex_t*, vertex_t*>;
        using level_map  = std::unordered_map<vertex_key const, vertex_t*, utils::tuple_hash_t<vertex_key const>>;
        using levels_v   = std::vector<level_map>;
        using stack_t    = ds::peekable_stack<stack_frame>;
        using base_t       = mdd_creator<VertexData, ArcData, 2, Allocator>;

    private:
        levels_v  levels_;
        stack_t   stack_;
        id_t      nextId_;
    };

    template<class VertexData, class ArcData, class Allocator>
    bdd_creator<VertexData, ArcData, Allocator>::bdd_creator
        (Allocator const& alloc) :
        base_t    {alloc},
        nextId_ {0}
    {
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class InputIt>
    auto bdd_creator<VertexData, ArcData, Allocator>::product
        (InputIt varsIt, InputIt varsEnd) -> bdd_t
    {
        return this->concat_impl(varsIt, varsEnd, 0);
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class InputIt>
    auto bdd_creator<VertexData, ArcData, Allocator>::sum
        (InputIt varsIt, InputIt varsEnd) -> bdd_t
    {
        return this->concat_impl(varsIt, varsEnd, 1);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::product
        (std::vector<bool_var> const& vars) -> bdd_t
    {
        return this->product(std::begin(vars), std::end(vars));
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::sum
        (std::vector<bool_var> const& vars) -> bdd_t
    {
        return this->sum(std::begin(vars), std::end(vars));
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::from_pla 
        (pla_file const& file, fold_e const mm) -> std::vector<bdd_t>
    {
        auto const& plaLines      = file.get_lines();
        auto const  lineCount     = file.line_count();
        auto const  functionCount = file.function_count();

        // Create a diagram for each function. 
        auto functionDiagrams = utils::vector<bdd_t>(functionCount);
        for (auto fi = 0u; fi < functionCount; ++fi)
        {
            // First create a diagram for each product.
            auto productDiagrams = utils::vector<bdd_t>(lineCount);
            for (auto li = 0u; li < lineCount; ++li)
            {
                // We are doing SOP so we are only interested in functions with value 1.
                if (plaLines[li].fVals.at(fi) == 1)
                {
                    productDiagrams.emplace_back(this->line_to_product(plaLines[li]));
                }
            }

            // In this case we just have a constant function.
            if (productDiagrams.empty())
            {
                productDiagrams.emplace_back(this->just_val(0));
            }

            // Then merge products using OR.
            functionDiagrams.emplace_back(this->or_merge(std::move(productDiagrams), mm));
        }

        return functionDiagrams;
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class Range>
    auto bdd_creator<VertexData, ArcData, Allocator>::from_vector
        (Range const& range) -> bdd_t
    {
        return this->from_vector(std::begin(range), std::end(range));
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class InputIt>
    auto bdd_creator<VertexData, ArcData, Allocator>::from_vector
        (InputIt first, InputIt last) -> bdd_t
    {
        nextId_ = 0;
        auto const varCount  = static_cast<index_t>(std::log2(std::distance(first, last)));
        auto const valToLeaf = std::array<vertex_t*, 2>
        {
            base_t::manager_.create(nextId_++, varCount), 
            base_t::manager_.create(nextId_++, varCount)
        };

        auto leafToVal = leaf_val_map 
        { 
            {valToLeaf.at(0), 0}
          , {valToLeaf.at(1), 1} 
        };

        levels_.resize(varCount + 1);

        while (first != last)
        {
            auto       son     = static_cast<vertex_t*>(nullptr);
            auto const currVal = static_cast<bool>(*first++);
            auto const nextVal = static_cast<bool>(*first++);

            if (currVal == nextVal)
            {
                son = valToLeaf.at(currVal);
            }
            else
            {
                son = this->try_insert(valToLeaf.at(currVal), valToLeaf.at(nextVal), varCount - 1);
            }

            stack_.emplace(son, varCount - 1);

            while (stack_.size() > 1)
            {
                if (stack_.peek(0).level != stack_.peek(1).level)
                {
                    break;
                }

                auto const stackLevel = stack_.peek(0).level;
                if (stack_.peek(0).vertexPtr == stack_.peek(1).vertexPtr)
                {
                    auto const v = stack_.peek(0).vertexPtr;
                    stack_.pop();
                    stack_.pop();
                    stack_.emplace(v, stackLevel - 1);
                }
                else
                {
                    auto const negativeTarget = stack_.peek(1).vertexPtr;
                    auto const positiveTarget = stack_.peek(0).vertexPtr;
                    stack_.pop();
                    stack_.pop();
                    son = this->try_insert(negativeTarget, positiveTarget, stackLevel - 1);
                    stack_.emplace(son, stackLevel - 1);
                }
            }
        }

        auto const root = stack_.peek(0).vertexPtr;
        levels_.clear();
        stack_.clear();

        if (root == valToLeaf.at(0))
        {
            leafToVal.erase(valToLeaf.at(1));
            base_t::manager_.release(valToLeaf.at(1));
        }

        if (root == valToLeaf.at(1))
        {
            leafToVal.erase(valToLeaf.at(0));
            base_t::manager_.release(valToLeaf.at(0));
        }

        return bdd_t { root
                     , std::move(leafToVal)
                     , base_t::manager_.get_alloc() };
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class InputIt>
    auto bdd_creator<VertexData, ArcData, Allocator>::concat_impl
        (InputIt varsIt, InputIt varsEnd, bool_t const absorbingVal) -> bdd_t
    {
        // Create vertex for each variable and point absorbing arc to the absorbing leaf.
        // (Or the other arc in case the variable is complemented.)
        auto const tmpIndex      = std::numeric_limits<index_t>::max();
        auto const otherVal      = bool_t {!absorbingVal};
        auto const absorbingLeaf = base_t::manager_.create(id_t {0}, tmpIndex);
        auto const otherLeaf     = base_t::manager_.create(id_t {1}, tmpIndex);
        auto nextId   = id_t {2};
        auto vertices = utils::map(varsIt, varsEnd, [&](auto const& boolVar)
        {
            auto const vertex         = base_t::manager_.create(nextId++, boolVar.index);
            auto const absorbingIndex = boolVar.complemented ? otherVal : absorbingVal;
            vertex->set_son(absorbingIndex, absorbingLeaf);
            return vertex;
        });
        auto const leafIndex = 1 + vertices.back()->get_index();
        absorbingLeaf->set_index(leafIndex);
        otherLeaf->set_index(leafIndex);

        // Add other leaf at the end so that no special case needs to be handled
        // in the following linking.
        vertices.emplace_back(otherLeaf);

        // Link vertices using their other (or absorbing if it is complemented) arcs.
        auto it  = std::begin(vertices);
        auto end = std::prev(std::end(vertices));
        while (it != end)
        {
             auto const vertex     = *it++;
             auto const otherIndex = vertex->get_son(0) ? 1 : 0;
             vertex->set_son(otherIndex, *it);
        }

        // Finally just create the diagram.
        return bdd_t { vertices.front()
                     , { {absorbingLeaf, absorbingVal}, {otherLeaf, otherVal} }
                     , base_t::manager_.get_alloc() };
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::or_merge
        (std::vector<bdd_t> diagrams, fold_e mm) -> bdd_t
    {
        auto m = manipulator_t(base_t::manager_.get_alloc());
        switch (mm)
        {
            case fold_e::tree:
                return m.tree_fold(std::move(diagrams), OR());

            case fold_e::left:
                return m.left_fold(std::move(diagrams), OR());

            default:
                throw std::runtime_error {"Non-exhaustive enum switch."};
        }
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::line_to_product
        (pla_line const& line) -> bdd_t
    {
        auto const vars = cube_to_bool_vars(line.cube);

        // If there is no relevant variable i.e. line looks like "------",
        // we simply return constant 0 which is the neutral element for OR.
        return vars.empty() ? this->just_val(0) 
                            : this->product(std::begin(vars), std::end(vars));
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::try_insert 
        (vertex_t* const falseSon, vertex_t* const trueSon, index_t const level) -> vertex_t*
    {
        auto const key = vertex_key {falseSon, trueSon};
        auto inGraphIt = levels_.at(level).find(key);
        if (inGraphIt != levels_.at(level).end())
        {
            return (*inGraphIt).second;
        }

        auto const newVertex = base_t::manager_.create( nextId_++
                                                      , level
                                                      , arc_arr_t {arc_t {falseSon}, arc_t {trueSon}} );
        levels_.at(level).emplace(key, newVertex);

        return newVertex;
    }

}

#endif