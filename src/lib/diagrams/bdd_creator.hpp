#ifndef MIX_DD_BDD_CREATOR_
#define MIX_DD_BDD_CREATOR_

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
    enum class merge_mode_e {sequential, iterative};

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

        template<class InputIt>
        auto product (InputIt varsIt, InputIt varsEnd) -> bdd_t;

        template<class Range>
        auto product (Range&& range) -> bdd_t;

        auto from_pla (pla_file const& file, merge_mode_e const mm = merge_mode_e::iterative) -> std::vector<bdd_t>;

        template<class Range>
        auto from_vector (Range const& range) -> bdd_t;

        template<class InputIt>
        auto from_vector (InputIt first, InputIt last) -> bdd_t;

    private: 
        auto or_merge            (std::vector<bdd_t> diagrams, merge_mode_e) -> bdd_t;
        auto or_merge_iterative  (std::vector<bdd_t> diagrams) -> bdd_t;
        auto or_merge_sequential (std::vector<bdd_t> diagrams) -> bdd_t;
        auto line_to_product     (pla_line const& line)        -> bdd_t;

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
        using base       = mdd_creator<VertexData, ArcData, 2, Allocator>;

    private:
        levels_v  levels_;
        stack_t   stack_;
        id_t      nextId_;
    };

    template<class VertexData, class ArcData, class Allocator>
    bdd_creator<VertexData, ArcData, Allocator>::bdd_creator
        (Allocator const& alloc) :
        base    {alloc},
        nextId_ {0}
    {
    }

    template<class VertexData = double, class ArcData = void>
    auto x (index_t const i) -> bdd<VertexData, ArcData>
    {
        using creator_t = bdd_creator<VertexData, ArcData>;
        auto creator = creator_t {};
        return creator.just_var(i);
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class InputIt>
    auto bdd_creator<VertexData, ArcData, Allocator>::product
        (InputIt varsIt, InputIt varsEnd) -> bdd_t
    {
        // Create vertex for each variable and point false arc to the false leaf.
        // (Or true arc in case the variable is complemented(negated).)
        auto const varCount  = 1 + (*std::prev(varsEnd)).first;
        auto const falseLeaf = base::manager_.create(0, varCount);
        auto const trueLeaf  = base::manager_.create(1, varCount);
        auto nextId   = id_t {2};
        auto vertices = utils::map(varsIt, varsEnd, [&](auto const& pair)
        {
            auto const index       = std::get<0>(pair);
            auto const isNegated   = std::get<1>(pair);
            auto const vertex      = base::manager_.create(nextId++, index);
            vertex->son(isNegated) = falseLeaf;
            return vertex;
        });

        // Add true leaf at the end so that no special case needs to be handled
        // in the following linking.
        vertices.emplace_back(trueLeaf);

        // Link vertices using their true (or false if it is complemented) arcs.
        auto it  = std::begin(vertices);
        auto end = std::prev(std::end(vertices));
        while (it != end)
        {
             auto const vertex  = *it++;
             auto const value   = static_cast<bool>(vertex->son(0));
             vertex->son(value) = *it;
        }

        // Finally just create the diagram.
        return bdd_t { vertices.front()
                     , { {trueLeaf, 1}, {falseLeaf, 0} }
                     , base::manager_.get_alloc() };
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class Range>
    auto bdd_creator<VertexData, ArcData, Allocator>::product
        (Range&& range) -> bdd_t
    {
        return this->product(std::begin(range), std::end(range));
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::from_pla 
        (pla_file const& file, merge_mode_e const mm) -> std::vector<bdd_t>
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
            base::manager_.create(nextId_++, varCount), 
            base::manager_.create(nextId_++, varCount)
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
            base::manager_.release(valToLeaf.at(1));
        }

        if (root == valToLeaf.at(1))
        {
            leafToVal.erase(valToLeaf.at(0));
            base::manager_.release(valToLeaf.at(0));
        }

        return bdd_t { root
                     , std::move(leafToVal)
                     , base::manager_.get_alloc() };
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::or_merge
        (std::vector<bdd_t> diagrams, merge_mode_e mm) -> bdd_t
    {
        switch (mm)
        {
        case merge_mode_e::iterative:
            return this->or_merge_iterative(std::move(diagrams));
        
        case merge_mode_e::sequential:
            return this->or_merge_sequential(std::move(diagrams));

        default:
            throw std::runtime_error {"Non-exhaustive enum switch."};
        }
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::or_merge_iterative
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        auto const numOfSteps = static_cast<std::size_t>(std::ceil(std::log2(diagrams.size())));
        auto diagramCount     = diagrams.size();
        auto m = manipulator_t {base::manager_.get_alloc()};
		
        for (auto step = 0u; step < numOfSteps; ++step)
        {
            auto const justMoveLast = diagramCount & 1;
            diagramCount = (diagramCount >> 1) + (diagramCount & 1);
            auto const iterCount    = diagramCount - justMoveLast;
		
            for (auto i = 0u; i < iterCount; ++i)
            {
                diagrams[i] = m.apply( std::move(diagrams[2 * i])
                                     , OR {}
                                     , std::move(diagrams[2 * i + 1]) );
            }
		
            if (justMoveLast)
            {
                diagrams[diagramCount - 1] = std::move(diagrams[2 * (diagramCount - 1)]);
            }
        }
		
        return bdd_t {std::move(diagrams.front())};
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::or_merge_sequential
        (std::vector<bdd_t> diagrams) -> bdd_t
    {
        auto m   = manipulator_t {base::manager_.get_alloc()};
        auto it  = std::next(std::begin(diagrams));
        auto end = std::end(diagrams);
        while (it != end)
        {
            diagrams.front() = m.apply( std::move(diagrams.front())
                                      , OR {}
                                      , std::move(*it++) );
        }

        return bdd_t {std::move(diagrams.front())};
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_creator<VertexData, ArcData, Allocator>::line_to_product
        (pla_line const& line) -> bdd_t
    {
        auto vars = cube_to_pairs(line.cube);

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

        auto const newVertex = base::manager_.create( nextId_++
                                                    , level
                                                    , arc_arr_t {arc_t {falseSon}, arc_t {trueSon}} );
        levels_.at(level).emplace(key, newVertex);
        
        return newVertex;
    }

}

#endif