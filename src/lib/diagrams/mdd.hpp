#ifndef MIX_DD_MDD_HPP
#define MIX_DD_MDD_HPP

#include "graph.hpp"
#include "mdd_level_iterator.hpp"
#include "var_vals.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/alloc_manager.hpp"
#include "../utils/print.hpp"
#include "../data_structures/simple_map.hpp"

#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include <ostream>
#include <utility>
#include <algorithm>
#include <iterator>

namespace mix::dd
{
    template<class VertexData, class ArcData, class Allocator = std::allocator<vertex<VertexData, ArcData, 2>>> class bdd;
    template<class VertexData, class ArcData, class Allocator = std::allocator<vertex<VertexData, ArcData, 2>>> class bdd_creator;
    template<class VertexData, class ArcData, class Allocator = std::allocator<vertex<VertexData, ArcData, 2>>> class bdd_manipulator;
    template<class VertexData, class ArcData, class Allocator = std::allocator<vertex<VertexData, ArcData, 2>>> class bdd_reliability;

    template<class VertexData, class ArcData, std::size_t P, class Allocator = std::allocator<vertex<VertexData, ArcData, P>>> class mdd;
    template<class VertexData, class ArcData, std::size_t P, class Allocator = std::allocator<vertex<VertexData, ArcData, P>>> class mdd_creator;
    template<class VertexData, class ArcData, std::size_t P, class Allocator = std::allocator<vertex<VertexData, ArcData, P>>> class mdd_manipulator;
    template<class VertexData, class ArcData, std::size_t P, class Allocator = std::allocator<vertex<VertexData, ArcData, P>>> class mdd_reliability;

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    class mdd
    {
    public:
        using vertex_t       = vertex<VertexData, ArcData, P>;
        using arc_t          = arc<VertexData, ArcData, P>;
        using log_t          = typename log_val_traits<P>::type;
        using iterator       = mdd_level_iterator<VertexData, ArcData, P, false>;
        using const_iterator = mdd_level_iterator<VertexData, ArcData, P, true>;

        friend class mdd_creator<VertexData, ArcData, P, Allocator>;
        friend class mdd_manipulator<VertexData, ArcData, P>;
        friend class mdd_reliability<VertexData, ArcData, P, Allocator>;
        friend class bdd_reliability<VertexData, ArcData, Allocator>;

    public:
        /**
            Constructor that accepts an allocator.
            Useful only if you provide a custom allocator.
         */
        explicit mdd  (Allocator const& alloc = Allocator {});

        /**
            Copy constructor.
         */
        mdd  (mdd const&);

        /**
            Move constructor.
         */
        mdd  (mdd&&) noexcept;

        /**
            Destructor.
         */
        ~mdd ();

        /**
            @brief Swaps this diagram with the other one in constant time.
            @param other Diagram to swap with.
         */
        auto swap (mdd& other) noexcept -> void;

        /**
            Copy and move assign operator.
            See. https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
            @param rhs - diagram that is to be asigned into this one.
        */
        auto operator= (mdd rhs) noexcept -> mdd&;

        /**
            @return the allocator associated with the diagram.
         */
        auto get_allocator () const -> Allocator;

    public:
        /**
            @tparam VariableValues - type that stores values of variables.
            @tparam GetIthVal - functor that returns value of i-th variable.
                    std::vector<bool>, std::bitset<N>, bit_vector<N, bool_t> and 
                    any integral type are supported by default. 
                    See "var_vals.hpp" for implementation details.
            @param  vars values of the variables. 
            @return value of the function for given input. 
        */
        template< class VariableValues
                , class GetIthVal = get_var_val<P, VariableValues> >
        auto get_value (VariableValues const& vars) const -> log_t;

        /**
            Prints the diagram in a dot format to the output stream.
            @param ost output stream e.g. std::cout or std::fstream.
        */
        auto to_dot_graph (std::ostream& ost) const -> void;

        /**
            @return Pointer to the vertex node of the diagram.
         */
        auto get_root () const -> vertex_t*;

        /**
            @return Pointer to the leaf vertex that represents given value.
                    nullptr if there is no such vertex.
         */
        auto get_leaf (log_t const val) const -> vertex_t*;

        /**
            @return Number of vertices in the diagram.
        */
        auto vertex_count () const -> std::size_t;

        /**
            @return Number of variables in the function.
                    Function doesn't necessarily depends on all of them.
         */
        auto variable_count () const -> index_t;

        /**
            Inserts indices of all variables that are present in this diagram
            into the output range.

            @param out output iterator.
         */
        template<class OutputIt>
        auto dependency_set (OutputIt out) const -> void;

        /**
            @return begin forward level order iterator.
         */
        auto begin () -> iterator;

        /**
            @return end forward level order iterator.
         */
        auto end () -> iterator;

        /**
            @return begin forward level order const iterator.
         */
        auto begin () const -> const_iterator;

        /**
            @return end forward level order const iterator.
         */
        auto end () const -> const_iterator;

    protected:
        using leaf_val_map = ds::simple_map<vertex_t*, log_t>;
        using manager_t    = utils::alloc_manager<Allocator>;

    protected:
        mdd ( vertex_t* const  root
            , leaf_val_map     leafToVal
            , Allocator const& alloc );

        auto clear () -> void;

        auto fill_levels () const -> std::vector< std::vector<vertex_t*> >;
        auto leaf_index  () const -> index_t;

        auto is_leaf (vertex_t const* const v) const -> bool;
        auto value   (vertex_t* const v)       const -> log_t;

        template<class UnaryFunction>
        auto traverse_pre (vertex_t* const v, UnaryFunction&& f) const -> void;

        template<class UnaryFunction>
        auto traverse_post (vertex_t* const v, UnaryFunction&& f) const -> void;

    protected:
        leaf_val_map leafToVal_;
        vertex_t*    root_;
        manager_t    manager_;
    };

    /**
        Swaps rhs and lhs using their member function.
     */
    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto swap ( mdd<VertexData, ArcData, P, Allocator>& lhs 
              , mdd<VertexData, ArcData, P, Allocator>& rhs) noexcept -> void;

// definitions:
    namespace aux_impl
    {
        template<class Alloc, class = std::void_t<>>
        struct is_recycling : public std::false_type { };

        template<class Alloc>
        struct is_recycling<Alloc, std::void_t<typename Alloc::is_recycling>> : public std::true_type { };

        template<class Alloc>
        inline constexpr auto is_recycling_v = is_recycling<Alloc>::value; 
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd<VertexData, ArcData, P, Allocator>::mdd
        (Allocator const& alloc) :
        root_    {nullptr},
        manager_ {alloc}
    {
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd<VertexData, ArcData, P, Allocator>::mdd
        (mdd const& other) :
        manager_ {other.manager_}
    {
        if (! other.root_)
        {
            return;
        }

        // first we copy each vertex:
        auto newVerticesMap = std::unordered_map<id_t, vertex_t*> {};
        // std::unordered_map<id_t, vertex_t*> newVerticesMap;
        other.traverse_pre(other.root_, [&](auto v) 
        {
            newVerticesMap.emplace(v->id, manager_.create(*v));
        });

        // now we iterate the other diagram from the bottom level:
        auto const levels = other.fill_levels();
        auto levelsIt     = std::next(std::rbegin(levels));
        auto levelsItEnd  = std::rend(levels);

        // making deep copy here:
        while (levelsIt != levelsItEnd)
        {
            for (auto const otherVertex : *levelsIt++)
            {
                auto newVertex = newVerticesMap.at(otherVertex->id);
                for (auto i = 0u; i < P; ++i)
                {
                    newVertex->son(i) = newVerticesMap.at(otherVertex->son(i)->id);
                }
            }
        }

        // set new root:
        root_ = newVerticesMap.at(levels.at(other.root_->index).front()->id);

        // fill leafToVal map:
        for (auto const [leaf, val] : other.leafToVal_)
        {
            leafToVal_.emplace(newVerticesMap.at(leaf->id), val);
        }
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd<VertexData, ArcData, P, Allocator>::mdd
        (mdd&& other) noexcept :
        leafToVal_ {std::move(other.leafToVal_)},
        root_      {std::exchange(other.root_, nullptr)},
        manager_   {std::move(other.manager_)}
    {
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd<VertexData, ArcData, P, Allocator>::mdd 
        ( vertex_t* const  root
        , leaf_val_map     leafToVal
        , Allocator const& alloc ) :
        leafToVal_ {std::move(leafToVal)},
        root_      {root},
        manager_   {alloc}
    {
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd<VertexData, ArcData, P, Allocator>::~mdd()
    {
        this->clear();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::swap
        (mdd& other) noexcept -> void
    {
        using std::swap;
        swap(root_, other.root_);
        swap(leafToVal_, other.leafToVal_);
        swap(manager_, other.manager_);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::operator=
        (mdd rhs) noexcept -> mdd&
    {
        this->swap(rhs);
        return *this;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::get_allocator
        () const -> Allocator
    {
        return manager_.get_alloc();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::to_dot_graph
        (std::ostream& ost) const -> void
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
            return v->index == this->leaf_index() ? traits_t::to_string(leafToVal_.at(v))
                                                  : "x" + to_string(v->index);
        };

        for (auto const& level : this->fill_levels())
        {
            if (level.empty())
            {
                continue;
            }

            auto ranksLocal = std::vector<std::string> {"{rank = same;"};
            for (auto const v : level)
            {
                labels.emplace_back(concat(v->id , " [label = " , make_label(v) , "];"));
                ranksLocal.emplace_back(concat(v->id , ";"));

                if (!this->is_leaf(v))
                {
                    for (auto val = 0u; val < P; ++val)
                    {
                        if constexpr (2 == P)
                        {
                            auto const style = 0 == val ? "dashed" : "solid";
                            arcs.emplace_back(concat(v->id , " -> " , v->son(val)->id , " [style = " , style , "];"));
                        }
                        else
                        {
                            arcs.emplace_back(concat(v->id , " -> " , v->son(val)->id , " [label = \"" , val , "\"];"));
                        }
                    }
                }
            }

            ranksLocal.emplace_back("}");
            ranks.emplace_back(concat_range(ranksLocal, " "));
        }

        for (auto const& [leaf, val] : leafToVal_)
        {
            squareShapes.emplace_back(to_string(leaf->id));
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

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::get_root
        () const -> vertex_t*
    {
        return root_;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::get_leaf
        (log_t const val) const -> vertex_t*
    {
        auto const it = std::find_if(std::begin(leafToVal_), std::end(leafToVal_),
            [val](auto const& pair)
        {
            return val == pair.second;
        });

        return it == std::end(leafToVal_) ? nullptr : it->first;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class VariableValues, class GetIthVal>
    auto mdd<VertexData, ArcData, P, Allocator>::get_value
        (VariableValues const& vars) const -> log_t
    {
        auto get_var = GetIthVal {};
        auto v       = root_;

        while (! this->is_leaf(v))
        {
            v = v->son(get_var(vars, v->index));
        }
        
        return leafToVal_.at(v);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::vertex_count
        () const -> std::size_t
    {
        auto size = 0ul;
        this->traverse_pre(root_, [&size](auto&&) { ++size; });
        return size;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::variable_count
        () const -> index_t
    {
        return std::begin(leafToVal_)->first->index;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class OutputIt>
    auto mdd<VertexData, ArcData, P, Allocator>::dependency_set
        (OutputIt out) const -> void
    {
        auto yetIn = std::vector<bool>(this->variable_count(), false);
        this->traverse_pre(root_, [&yetIn, &out](auto const v)
        {
            if (!yetIn[v->index])
            {
                *out++ = v;
                yetIn[v->index] = true;
            }
        });
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::begin
        () -> iterator
    {
        return iterator {root_, this->variable_count()};
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::end
        () -> iterator
    {
        return iterator();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::begin
        () const -> const_iterator
    {
        return const_iterator {root_, this->variable_count()};
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::end
        () const -> const_iterator
    {
        return const_iterator();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::clear
        () -> void
    {
        if constexpr (aux_impl::is_recycling_v<Allocator>)
        {
            if (root_)
            {
                this->traverse_pre(root_, [this](auto v) 
                {
                    manager_.release(v);
                });
            }
        }
        else
        {
            for (auto const& level : this->fill_levels())
            {
                for (auto const v : level)
                {
                    manager_.release(v);
                }
            }
        }

        root_ = nullptr;
        leafToVal_.clear();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::fill_levels
        () const -> std::vector< std::vector<vertex_t*> >
    {
        auto levels = std::vector<std::vector<vertex_t*>>(this->variable_count() + 1);

        if (!root_)
        {
            return levels;
        }

        this->traverse_pre(root_, [&levels](vertex_t* const v) 
        {
            levels[v->index].push_back(v);
        });

        return levels;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::leaf_index
        () const -> index_t
    {
        return this->variable_count();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::is_leaf
        (vertex_t const* const v) const -> bool
    {
        return v->index == this->leaf_index();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::value
        (vertex_t* const v) const -> log_t
    {
        auto constexpr N = log_val_traits<P>::nondetermined;
        return this->is_leaf(v) ? leafToVal_.at(v) : N;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class UnaryFunction>
    auto mdd<VertexData, ArcData, P, Allocator>::traverse_pre
        (vertex_t* const v, UnaryFunction&& f) const -> void
    {
        v->mark = ! v->mark;

        f(v);

        for (auto i = 0u; i < P; ++i)
        {
            if (! this->is_leaf(v) && v->mark != v->son(i)->mark)
            {
                this->traverse_pre(v->son(i), f);
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class UnaryFunction>
    auto mdd<VertexData, ArcData, P, Allocator>::traverse_post
        (vertex_t* const v, UnaryFunction&& f) const -> void
    {
        v->mark = ! v->mark;

        for (auto i = 0u; i < P; ++i)
        {
            if (! this->is_leaf(v) && v->mark != v->son(i)->mark)
            {
                this->traverse_post(v->son(i), f);
            }
        }
        
        f(v);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto swap ( mdd<VertexData, ArcData, P, Allocator>& lhs 
              , mdd<VertexData, ArcData, P, Allocator>& rhs) noexcept -> void
    {
        lhs.swap(rhs);
    }
}

#endif