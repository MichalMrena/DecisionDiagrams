#ifndef MIX_DD_MDD_HPP
#define MIX_DD_MDD_HPP

#include "graph.hpp"
#include "mdd_level_iterator.hpp"
#include "var_vals.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/alloc_manager.hpp"
#include "../utils/more_math.hpp"
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
#include <type_traits>

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
            Creates copy of this diagram using the copy constructor.

            @return deep copy of this diagram.
         */
        auto clone () const -> mdd;

        /**
            Creates new diagrams by moving this diagram into to new one.
            This diagram is left empty.

            @return new diagram with content of this diagram.
         */
        auto move () -> mdd;

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
            Calculates the size size of the satisfying set of the function
            for given value @p val of the function.
            Uses data member of vertices to store intermediated results if possible.
            It is possible when VertexData is floating or at least 32bit integral type.
            Otherwise it uses a map which might be a bit slower.
            @param val value of the function for which the count should be calculated
            @return size of the satisfying set of the function.
        */
        auto satisfy_count (log_t const val) -> std::size_t;

        /**
            Calculates the size size of the satisfying set of the function
            for given value @p val of the function.
            Uses map to store intermediated results.
            @param val value of the function for which the count should be calculated
            @return size of the satisfying set of the function.
         */
        auto satisfy_count (log_t const val) const -> std::size_t;

        /**
            @return begin level order forward iterator.
         */
        auto begin () -> iterator;

        /**
            @return end level order forward iterator.
         */
        auto end () -> iterator;

        /**
            @return begin level order forward const iterator.
         */
        auto begin () const -> const_iterator;

        /**
            @return end level order forward const iterator.
         */
        auto end () const -> const_iterator;

    protected:
        using leaf_val_map = ds::simple_map<vertex_t*, log_t>;
        using manager_t    = utils::alloc_manager<Allocator>;

    private:
        struct use_data_member {};
        struct use_map {};

    protected:
        mdd ( vertex_t* const  root
            , leaf_val_map     leafToVal
            , Allocator const& alloc );

        auto clear () -> void;

        auto fill_levels () const -> std::vector< std::vector<vertex_t*> >;
        auto leaf_index  () const -> index_t;

        auto is_leaf (vertex_t const* const v) const -> bool;
        auto value   (vertex_t* const v)       const -> log_t;

        auto root_alpha (log_t const val, use_data_member) -> std::size_t;
        auto root_alpha (log_t const val, use_map) const   -> std::size_t;

        template<class Container>
        auto fill_container () const -> Container;

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

        template<class T>
        inline constexpr auto can_use_data_member_v = std::is_arithmetic_v<T> && sizeof(T) >= 32;
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
        auto newVerticesMap = std::unordered_map<id_t, vertex_t*>();
        other.traverse_pre(other.root_, [&](auto v) 
        {
            newVerticesMap.emplace(v->get_id(), manager_.create(*v));
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
                auto newVertex = newVerticesMap.at(otherVertex->get_id());
                for (auto i = 0u; i < P; ++i)
                {
                    newVertex->set_son(i, newVerticesMap.at(otherVertex->get_son(i)->get_id()));
                }
            }
        }

        // set new root:
        root_ = newVerticesMap.at(levels.at(other.root_->get_index()).front()->get_id());

        // fill leafToVal map:
        for (auto const [leaf, val] : other.leafToVal_)
        {
            leafToVal_.emplace(newVerticesMap.at(leaf->get_id()), val);
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
    auto mdd<VertexData, ArcData, P, Allocator>::clone
        () const -> mdd
    {
        return mdd(*this);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::move
        () -> mdd
    {
        return mdd(std::move(*this));
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
            return v->get_index() == this->leaf_index() ? traits_t::to_string(leafToVal_.at(v))
                                                        : "x" + to_string(v->get_index());
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
                labels.emplace_back(concat(v->get_id() , " [label = " , make_label(v) , "];"));
                ranksLocal.emplace_back(concat(v->get_id() , ";"));

                if (!this->is_leaf(v))
                {
                    for (auto val = 0u; val < P; ++val)
                    {
                        if constexpr (2 == P)
                        {
                            auto const style = 0 == val ? "dashed" : "solid";
                            arcs.emplace_back(concat(v->get_id() , " -> " , v->get_son(val)->get_id() , " [style = " , style , "];"));
                        }
                        else
                        {
                            arcs.emplace_back(concat(v->get_id() , " -> " , v->get_son(val)->get_id() , " [label = \"" , val , "\"];"));
                        }
                    }
                }
            }

            ranksLocal.emplace_back("}");
            ranks.emplace_back(concat_range(ranksLocal, " "));
        }

        for (auto const& [leaf, val] : leafToVal_)
        {
            squareShapes.emplace_back(to_string(leaf->get_id()));
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
            v = v->get_son(get_var(vars, v->get_index()));
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
        return std::begin(leafToVal_)->first->get_index();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class OutputIt>
    auto mdd<VertexData, ArcData, P, Allocator>::dependency_set
        (OutputIt out) const -> void
    {
        auto yetIn = std::vector<bool>(this->variable_count(), false);
        this->traverse_pre(root_, [&yetIn, &out](auto const v)
        {
            if (!yetIn[v->get_index()])
            {
                *out++ = v;
                yetIn[v->get_index()] = true;
            }
        });
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::satisfy_count
        (log_t const val) -> std::size_t
    {
        if constexpr (aux_impl::can_use_data_member_v<VertexData>)
        {
            auto const rootAlpha = this->root_alpha(val, use_data_member {});
            return rootAlpha * utils::int_pow(P, root_->get_index());
        }
        else
        {
            return const_cast<mdd const&>(*this).satisfy_count(val);
        }
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::satisfy_count
        (log_t const val) const -> std::size_t
    {
        auto const rootAlpha = this->root_alpha(val, use_map {});
        return rootAlpha * utils::int_pow(P, root_->get_index());
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
            if (root_)
            {
                for (auto const& level : this->fill_levels())
                {
                    for (auto const v : level)
                    {
                        manager_.release(v);
                    }
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
            levels[v->get_index()].push_back(v);
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
        return v->get_index() == this->leaf_index();
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::value
        (vertex_t* const v) const -> log_t
    {
        auto constexpr N = log_val_traits<P>::nondetermined;
        return this->is_leaf(v) ? leafToVal_.at(v) : N;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::root_alpha
        (log_t const val, use_data_member) -> std::size_t
    {
        this->traverse_post(root_, [this, val](auto const v)
        {
            if (this->is_leaf(v))
            {
                v->data = this->value(v) == val ? 1 : 0;
            }
            else
            {
                v->data = 0;
                for (auto i = 0u; i < P; ++i)
                {
                    v->data += v->get_son(i)->data
                             * utils::int_pow(P, v->get_son(i)->get_index() - v->get_index() - 1);
                }
            }
        });

        return root_->data;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd<VertexData, ArcData, P, Allocator>::root_alpha
        (log_t const val, use_map) const -> std::size_t
    {
        auto dataMap = std::unordered_map<vertex_t*, std::size_t>();

        this->traverse_post(root_, [this, &dataMap, val](auto const v)
        {
            auto alpha = VertexData {0};
            if (this->is_leaf(v))
            {
                alpha = this->value(v) == val ? 1 : 0;
            }
            else
            {
                for (auto i = 0u; i < P; ++i)
                {
                    alpha += dataMap.at(v->get_son(i)) 
                           * utils::int_pow(P, v->get_son(i)->get_index() - v->get_index() - 1);
                }
            }
            dataMap.emplace(v, alpha);
        });

        return dataMap.at(root_);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class Container>
    auto mdd<VertexData, ArcData, P, Allocator>::fill_container
        () const -> Container
    {
        auto c = Container();
        auto outIt = std::inserter(c, std::end(c));

        this->traverse_pre(root_, [&outIt](auto const v)
        {
            outIt = v;
        });

        return c;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class UnaryFunction>
    auto mdd<VertexData, ArcData, P, Allocator>::traverse_pre
        (vertex_t* const v, UnaryFunction&& f) const -> void
    {
        v->toggle_mark();
        f(v);

        for (auto i = 0u; i < P; ++i)
        {
            if (! this->is_leaf(v) && v->get_mark() != v->get_son(i)->get_mark())
            {
                this->traverse_pre(v->get_son(i), f);
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class UnaryFunction>
    auto mdd<VertexData, ArcData, P, Allocator>::traverse_post
        (vertex_t* const v, UnaryFunction&& f) const -> void
    {
        v->toggle_mark();

        for (auto i = 0u; i < P; ++i)
        {
            if (! this->is_leaf(v) && v->get_mark() != v->get_son(i)->get_mark())
            {
                this->traverse_post(v->get_son(i), f);
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