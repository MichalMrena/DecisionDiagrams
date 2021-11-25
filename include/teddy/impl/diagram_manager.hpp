#ifndef MIX_DD_DIAGRAM_MANAGER_HPP
#define MIX_DD_DIAGRAM_MANAGER_HPP

#include "diagram.hpp"
#include "operators.hpp"
#include "node_manager.hpp"
#include "pla_file.hpp"
#include "utils.hpp"
#include <cmath>
#include <concepts>
#include <iterator>
#include <ranges>

namespace teddy
{
    template<class Vars>
    concept in_var_values = requires (Vars vs, index_t i)
    {
        { vs[i] } -> std::convertible_to<uint_t>;
    };

    template<class Vars>
    concept out_var_values = requires (Vars vs, index_t i, uint_t v)
    {
        vs[i] = v;
    };

    enum class fold_type
    {
        Left,
        Tree
    };

    template<class Data, degree Degree, domain Domain>
    class diagram_manager
    {
    public:
        using diagram_t = diagram<Data, Degree>;
        using node_t    = typename diagram<Data, Degree>::node_t;

    public:
        auto constant (uint_t) -> diagram_t;

        auto variable (index_t) -> diagram_t;

        auto variable_not (index_t) -> diagram_t; // enable only for bdd

        auto operator() (index_t) -> diagram_t;

        template<std::ranges::input_range Is>
        auto variables (Is const&) -> std::vector<diagram_t>;

        template<std::input_iterator I, std::sentinel_for<I> S>
        auto from_vector (I, S) -> diagram_t;

        template<std::ranges::input_range R>
        auto from_vector (R&&) -> diagram_t;

        auto from_pla (pla_file const&, fold_type) -> std::vector<diagram_t>;

        template<bin_op Op>
        auto apply (diagram_t const&, diagram_t const&) -> diagram_t;

        template<bin_op Op, std::ranges::input_range R>
        auto left_fold (R const&) -> diagram_t;

        template< bin_op               Op
                , std::input_iterator  I
                , std::sentinel_for<I> S >
        auto left_fold (I, S) -> diagram_t;

        template<bin_op Op, std::ranges::random_access_range R>
        auto tree_fold (R&) -> diagram_t;

        template< bin_op                      Op
                , std::random_access_iterator I
                , std::sentinel_for<I>        S >
        auto tree_fold (I, S) -> diagram_t;

        template<in_var_values Vars>
        auto evaluate (diagram_t const&, Vars const&) const -> uint_t;

        auto satisfy_count (uint_t, diagram_t&) -> std::size_t;

        template<out_var_values Vars>
        auto satisfy_all (uint_t, diagram_t const&) const -> std::vector<Vars>;

        template< out_var_values             Vars
                , std::output_iterator<Vars> Out >
        auto satisfy_all_g (uint_t, diagram_t const&, Out) const -> void;

        auto cofactor (diagram_t const&, index_t, uint_t) -> diagram_t;

        template<uint_to_bool F>
        auto booleanize (diagram_t const&, F = utils::not_zero) -> diagram_t;

        auto dependency_set (diagram_t const&) const -> std::vector<index_t>;

        template<std::output_iterator<index_t> O>
        auto dependency_set_g (diagram_t const&, O) const -> void;

        auto reduce (diagram_t const&) -> diagram_t;

        auto node_count () const -> std::size_t;

        auto node_count (diagram_t const&) const -> std::size_t;

        auto to_dot_graph (std::ostream&) const -> void;

        auto to_dot_graph (std::ostream&, diagram_t const&) const -> void;

        auto gc () -> void;

    public:
        auto get_var_count () const -> std::size_t;
        auto get_order     () const -> std::vector<index_t> const&;
        auto get_domains   () const -> std::vector<uint_t>;

    private:
        template<class F>
        auto transform_internal (node_t*, F&&) -> node_t*;

        template<uint_to_uint F>
        auto transform_terminal (node_t*, F) -> node_t*;

    protected:
        diagram_manager ( std::size_t vars
                        , std::size_t nodes
                        , std::vector<index_t> order )
                        requires(domains::is_fixed<Domain>()());

        diagram_manager ( std::size_t vars
                        , std::size_t nodes
                        , domains::mixed
                        , std::vector<index_t> order )
                        requires(domains::is_mixed<Domain>()());

    public:
        diagram_manager (diagram_manager const&) = delete;
        diagram_manager (diagram_manager&&)      = default;
        auto operator=  (diagram_manager const&) -> diagram_manager& = delete;
        auto operator=  (diagram_manager&&)      -> diagram_manager& = default;

    private:
        node_manager<Data, Degree, Domain> nodes_;
    };

    namespace detail
    {
        inline auto default_or_fwd
            (std::size_t const n, std::vector<index_t>& is)
        {
            return is.empty()
                       ? utils::fill_vector(n, utils::identity)
                       : std::vector<index_t>(std::move(is));
        }
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::constant
        (uint_t const v) -> diagram_t
    {
        return diagram_t(nodes_.terminal_node(v));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::variable
        (index_t const i) -> diagram_t
    {
        return diagram_t(nodes_.internal_node(i, nodes_.make_sons(i,
            [this](auto const v)
        {
            return nodes_.terminal_node(v);
        })));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::variable_not
        (index_t const i) -> diagram_t
    {
        return diagram_t(nodes_.internal_node(i, nodes_.make_sons(i,
            [this](auto const v)
        {
            return nodes_.terminal_node(1 - v);
        })));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::operator()
        (index_t const i) -> diagram_t
    {
        return this->variable(i);
    }

    template<class Data, degree Degree, domain Domain>
    template<std::ranges::input_range Is>
    auto diagram_manager<Data, Degree, Domain>::variables
        (Is const& is) -> std::vector<diagram_t>
    {
        static_assert(
            std::convertible_to<std::ranges::range_value_t<Is>, index_t> );
        return utils::fmap(is, [this](auto const i)
        {
            return this->variable(static_cast<index_t>(i));
        });
    }

    template<class Data, degree Degree, domain Domain>
    template<std::input_iterator I, std::sentinel_for<I> S>
    auto diagram_manager<Data, Degree, Domain>::from_vector
        (I first, S last) -> diagram_t
    {
        // TODO
        if (0 == this->get_var_count())
        {
            assert(first != last and std::next(first) == last);
            return diagram_t(nodes_.terminal_node(*first));
        }

        auto const lastLevel = static_cast<level_t>(this->get_var_count() - 1);
        auto const lastIndex = nodes_.get_index(lastLevel);

        if constexpr (std::random_access_iterator<I>)
        {
            namespace rs = std::ranges;
            auto const dist  = static_cast<std::size_t>(
                                   rs::distance(first, last));
            auto const count = nodes_.domain_product(0, lastLevel + 1);
            assert(dist > 0 and dist == count);
        }

        using stack_frame = struct { node_t* node; level_t level; };
        auto stack = std::vector<stack_frame>();
        auto const shrink_stack = [this, &stack]()
        {
            for (;;)
            {
                auto const currentLevel = stack.back().level;
                if (0 == currentLevel)
                {
                    break;
                }

                auto const end = std::rend(stack);
                auto it        = std::rbegin(stack);
                auto count     = 0ul;
                while (it != end and it->level == currentLevel)
                {
                    ++it;
                    ++count;
                }
                auto const newIndex  = nodes_.get_index(currentLevel - 1);
                auto const newDomain = nodes_.get_domain(newIndex);

                if (count < newDomain)
                {
                    break;
                }

                auto newSons = nodes_.make_sons(newIndex,
                    [&stack, newDomain](auto const o)
                {
                    return stack[stack.size() - newDomain + o].node;
                });
                auto const newNode = nodes_.internal_node( newIndex
                                                         , std::move(newSons) );
                stack.erase(std::end(stack) - newDomain, std::end(stack));
                stack.push_back(stack_frame {newNode, currentLevel - 1});
            }
        };

        while (first != last)
        {
            auto sons = nodes_.make_sons(lastIndex, [this, &first](auto const)
            {
                return nodes_.terminal_node(*first++);
            });
            auto const node = nodes_.internal_node(lastIndex, std::move(sons));
            stack.push_back(stack_frame {node, lastLevel});
            shrink_stack();
            // TODO adjust sizes? idealne presunut do node_manazer,
            // aby to robil sam, napriklad podla velkosti poolu
        }

        assert(stack.size() == 1);
        return diagram_t(stack.back().node);
    }

    template<class Data, degree Degree, domain Domain>
    template<std::ranges::input_range R>
    auto diagram_manager<Data, Degree, Domain>::from_vector
        (R&& r) -> diagram_t
    {
        namespace rs = std::ranges;
        return this->from_vector(rs::begin(r), rs::end(r));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::from_pla
        ( pla_file const& file
        , fold_type const foldType ) -> std::vector<diagram_t>
    {
        auto const product = [this](auto const& cube)
        {
            auto vs = std::vector<diagram_t>();
            vs.reserve(cube.size());
            for (auto i = 0u; i < cube.size(); ++i)
            {
                if (cube.get(i) == 1)
                {
                    vs.emplace_back(this->variable(i));
                }
                else if (cube.get(i) == 0)
                {
                    vs.emplace_back(this->variable_not(i));
                }
            }
            return this->left_fold<AND>(vs);
        };

        auto const orFold = [this, foldType](auto& ds)
        {
            switch (foldType)
            {
                case fold_type::Left:
                    return this->left_fold<OR>(ds);

                case fold_type::Tree:
                    return this->tree_fold<OR>(ds);

                default:
                    assert(false);
                    return this->constant(0);
            }
        };

        auto const& plaLines      = file.get_lines();
        auto const  lineCount     = file.line_count();
        auto const  functionCount = file.function_count();

        // Create a diagram for each function.
        auto functionDiagrams = std::vector<diagram_t>();
        functionDiagrams.reserve(functionCount);
        for (auto fi = 0u; fi < functionCount; ++fi)
        {
            // First create a diagram for each product.
            auto products = std::vector<diagram_t>();
            products.reserve(lineCount);
            for (auto li = 0u; li < lineCount; ++li)
            {
                // We are doing SOP so we are only interested
                // in functions with value 1.
                if (plaLines[li].fVals.get(fi) == 1)
                {
                    products.emplace_back(product(plaLines[li].cube));
                }
            }

            // In this case we just have a constant function.
            if (products.empty())
            {
                products.emplace_back(this->constant(0));
            }

            // Then merge products using OR.
            functionDiagrams.emplace_back(orFold(products));
        }

        return functionDiagrams;
    }

    template<class Data, degree Degree, domain Domain>
    template<bin_op Op>
    auto diagram_manager<Data, Degree, Domain>::apply
        (diagram_t const& d1, diagram_t const& d2) -> diagram_t
    {
        auto const go = [this](auto&& go_, auto l, auto r)
        {
            auto const cacheIt = nodes_.template cache_find<Op>(l, r);
            if (cacheIt->matches(l, r))
            {
                return cacheIt->result;
            }

            auto const lhsVal = node_value(l);
            auto const rhsVal = node_value(r);
            auto const opVal  = Op()(lhsVal, rhsVal);
            auto u = static_cast<node_t*>(nullptr);

            if (opVal != Nondetermined)
            {
                u = nodes_.terminal_node(opVal);
            }
            else
            {
                auto const lhsLevel = nodes_.get_level(l);
                auto const rhsLevel = nodes_.get_level(r);
                auto const topLevel = std::min(lhsLevel, rhsLevel);
                auto const topNode  = topLevel == lhsLevel ? l : r;
                auto const topIndex = topNode->get_index();
                auto sons = nodes_.make_sons(topIndex, [=, &go_](auto const k)
                {
                    auto const fst = lhsLevel == topLevel ? l->get_son(k) : l;
                    auto const snd = rhsLevel == topLevel ? r->get_son(k) : r;
                    return go_(go_, fst, snd);
                });

                u = nodes_.internal_node(topIndex, std::move(sons));
            }

            // cache tu má inú veľkosť, čiže môže byť alokované inde teda iterátory neplatia!
            nodes_.template cache_put<Op>(cacheIt, l, r, u);
            return u;
        };

        auto const r = go(go, d1.get_root(), d2.get_root());
        auto const d = diagram_t(r);
        return d;
    }

    template<class Data, degree Degree, domain Domain>
    template<bin_op Op, std::ranges::input_range R>
    auto diagram_manager<Data, Degree, Domain>::left_fold
        (R const& ds) -> diagram_t
    {
        namespace rs = std::ranges;
        return this->left_fold<Op>(rs::begin(ds), rs::end(ds));
    }

    template<class Data, degree Degree, domain Domain>
    template< bin_op               Op
            , std::input_iterator  I
            , std::sentinel_for<I> S >
    auto diagram_manager<Data, Degree, Domain>::left_fold
        (I first, S const last) -> diagram_t
    {
        static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

        auto r = std::move(*first);
        ++first;

        while (first != last)
        {
            r = this->apply<Op>(r, *first);
            ++first;
        }

        return r;
    }

    template<class Data, degree Degree, domain Domain>
    template<bin_op Op, std::ranges::random_access_range R>
    auto diagram_manager<Data, Degree, Domain>::tree_fold
        (R& ds) -> diagram_t
    {
        namespace rs = std::ranges;
        return this->tree_fold<Op>(rs::begin(ds), rs::end(ds));
    }

    template<class Data, degree Degree, domain Domain>
    template< bin_op                      Op
            , std::random_access_iterator I
            , std::sentinel_for<I>        S >
    auto diagram_manager<Data, Degree, Domain>::tree_fold
        (I first, S const last) -> diagram_t
    {
        static_assert(std::same_as<std::iter_value_t<I>, diagram_t>);

        auto const count  = std::distance(first, last);
        auto currentCount = count;
        auto const numOfSteps
            = static_cast<std::size_t>(std::ceil(std::log2(count)));

        for (auto step = 0u; step < numOfSteps; ++step)
        {
            auto const justMoveLast = currentCount & 1;
            currentCount = (currentCount / 2) + justMoveLast;
            auto const pairCount    = currentCount - justMoveLast;

            for (auto i = 0u; i < pairCount; ++i)
            {
                *(first + i) = this->apply<Op>( *(first + 2 * i)
                                              , *(first + 2 * i + 1) );
            }

            if (justMoveLast)
            {
                *(first + currentCount - 1)
                    = std::move(*(first + 2 * (currentCount - 1)));
            }
        }

        return diagram_t(std::move(*first));
    }

    template<class Data, degree Degree, domain Domain>
    template<in_var_values Vars>
    auto diagram_manager<Data, Degree, Domain>::evaluate
        (diagram_t const& d, Vars const& vs) const -> uint_t
    {
        auto n = d.get_root();

        while (not n->is_terminal())
        {
            auto const i = n->get_index();
            assert(nodes_.is_valid_var_value(i, vs[i]));
            n = n->get_son(vs[i]);
        }

        return n->get_value();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::satisfy_count
        (uint_t const val, diagram_t& d) -> std::size_t
    {
        if constexpr (domains::is_fixed<Domain>()())
        {
            assert(val < Domain()());
        }

        auto constexpr CanUseDataMember
            = std::is_floating_point_v<Data> or std::is_integral_v<Data>;
        using T = std::conditional_t<CanUseDataMember, Data, std::size_t>;

        // Returns a function that returns reference to
        // the data associated with given node.
        auto data = []()
        {
            if constexpr (CanUseDataMember)
            {
                // Simply return reference to the data member.
                return [](auto const n) mutable -> decltype(auto)
                {
                    return (n->data);
                };
            }
            else
            {
                // Return reference to the data that is stored int the map.
                return [map = std::unordered_map<node_t*, T>()]
                    (auto const n) mutable -> T&
                {
                    // If there is no value for given key [] creates new pair
                    // and value-initializes the value (0 for primitive types).
                    return map[n];
                };
            }
        }();

        // Actual satisfy count algorithm.
        nodes_.traverse_post(d.get_root(), [this, val, &data]
            (auto const n) mutable
        {
            if (n->is_terminal())
            {
                data(n) = n->get_value() == val ? 1 : 0;
            }
            else
            {
                data(n) = 0;
                auto const nLevel = nodes_.get_level(n);
                nodes_.for_each_son(n, [=, this, &data](auto const son) mutable
                {
                    auto const sonLevel = nodes_.get_level(son);
                    auto const diff     = nodes_.domain_product( nLevel + 1
                                                               , sonLevel );
                    data(n) += data(son) * static_cast<T>(diff);
                });
            }
        });

        auto const rootAlpha = static_cast<std::size_t>(data(d.get_root()));
        auto const rootLevel = nodes_.get_level(d.get_root());
        return rootAlpha * nodes_.domain_product(0, rootLevel);
    }

    template<class Data, degree Degree, domain Domain>
    template<out_var_values Vars>
    auto diagram_manager<Data, Degree, Domain>::satisfy_all
        (uint_t const val, diagram_t const& d) const -> std::vector<Vars>
    {
        auto vs = std::vector<Vars>();
        this->satisfy_all_g(val, d, std::back_inserter(vs));
        return vs;
    }

    template<class Data, degree Degree, domain Domain>
    template<out_var_values Vars, std::output_iterator<Vars> Out>
    auto diagram_manager<Data, Degree, Domain>::satisfy_all_g
        (uint_t const val, diagram_t const& d, Out out) const -> void
    {
        if constexpr (domains::is_fixed<Domain>()())
        {
            assert(val < Domain()());
        }

        auto xs = Vars {};
        auto go = [this, &xs, val, out]
            (auto&& go_, auto const l, auto const n) mutable
        {
            auto const nodeValue = node_value(n);
            auto const nodeLevel = nodes_.get_level(n);

            if (n->is_terminal() && val != nodeValue)
            {
                return;
            }
            else if (l == nodes_.get_leaf_level() && val == nodeValue)
            {
                *out++ = xs;
                return;
            }
            else if (nodeLevel > l)
            {
                auto const index  = nodes_.get_index(l);
                auto const domain = nodes_.get_domain(index);
                for (auto iv = 0u; iv < domain; ++iv)
                {
                    xs[index] = iv;
                    go_(go_, l + 1, n);
                }
            }
            else
            {
                auto const index = n->get_index();
                nodes_.for_each_son(n, [=, &xs, iv = uint_t {0}]
                    (auto const son) mutable
                {
                    xs[index] = iv;
                    go_(go_, l + 1, son);
                    ++iv;
                });
            }
        };

        go(go, level_t {0}, d.get_root());
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::cofactor
        (diagram_t const& d, index_t const i, uint_t const v) -> diagram_t
    {
        auto const newRoot = this->transform_internal(d.get_root(), [this, i, v]
            (auto&& go, auto&& self, auto const n)
        {
            if (n->get_index() == i)
            {
                // Create redundant node that will
                // be handled by the node manager.
                return nodes_.make_sons(i, [n](auto const)
                {
                    return n;
                });
            }
            else
            {
                // Nothing to restrict here so we just continue downwards.
                return nodes_.make_sons(n->get_index(), [&](auto const k)
                {
                    return go(go, self, n->get_son(k));
                });
            }
        });
        return diagram_t(newRoot);
    }

    template<class Data, degree Degree, domain Domain>
    template<uint_to_bool F>
    auto diagram_manager<Data, Degree, Domain>::booleanize
        (diagram_t const& d, F f) -> diagram_t
    {
        return diagram_t(this->transform_terminal(d.get_root(), f));
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::dependency_set
        (diagram_t const& d) const -> std::vector<index_t>
    {
        auto is = std::vector<index_t>();
        is.reserve(this->get_var_count());
        this->dependency_set_g(d, std::back_inserter(is));
        is.shrink_to_fit();
        return is;
    }

    template<class Data, degree Degree, domain Domain>
    template<std::output_iterator<index_t> O>
    auto diagram_manager<Data, Degree, Domain>::dependency_set_g
        (diagram_t const& d, O o) const -> void
    {
        auto memo = std::vector<bool>(this->get_var_count(), false);
        nodes_.traverse_pre(d.get_root(), [&memo, o](auto const n)
        {
            if (n->is_internal())
            {
                auto const i = n->get_index();
                if (not memo[i])
                {
                    *o++ = i;
                }
                memo[i] = true;
            }
        });
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::reduce
        (diagram_t const& d) -> diagram_t
    {
        auto const newRoot = this->transform_terminal( d.get_root()
                                                     , utils::identity );
        return diagram_t(newRoot);
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::node_count
        () const -> std::size_t
    {
        return nodes_.get_node_count();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::node_count
        (diagram_t const& d) const -> std::size_t
    {
        return nodes_.get_node_count(d.get_root());
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::to_dot_graph
        (std::ostream& ost) const -> void
    {
        nodes_.to_dot_graph(ost);
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::to_dot_graph
        (std::ostream& ost, diagram_t const& d) const -> void
    {
        nodes_.to_dot_graph(ost, d.get_root());
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::get_var_count
        () const -> std::size_t
    {
        return nodes_.get_var_count();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::get_order
        () const -> std::vector<index_t> const&
    {
        return nodes_.get_order();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::get_domains
        () const -> std::vector<uint_t>
    {
        return nodes_.get_domains();
    }

    template<class Data, degree Degree, domain Domain>
    auto diagram_manager<Data, Degree, Domain>::gc
        () -> void
    {
        nodes_.collect_garbage();
    }


    template<class Data, degree Degree, domain Domain>
    template<class F>
    auto diagram_manager<Data, Degree, Domain>::transform_internal
        (node_t* const root, F&& f) -> node_t*
    {
        auto memo = std::unordered_map<node_t*, node_t*>();
        auto const go = [this, &memo](auto&& go_, auto&& f_, auto const n)
        {
            auto const it = memo.find(n);
            if (memo.end() != it)
            {
                return it->second;
            }

            if (n->is_terminal())
            {
                return n;
            }

            auto const u = nodes_.internal_node( n->get_index()
                                               , f_(go_, f_, n) );
            memo.emplace(n, u);
            return u;
        };

        return go(go, f, root);
    }

    template<class Data, degree Degree, domain Domain>
    template<uint_to_uint F>
    auto diagram_manager<Data, Degree, Domain>::transform_terminal
        (node_t* const root, F f) -> node_t*
    {
        auto memo = std::unordered_map<node_t*, node_t*>();
        auto const go = [this, &f](auto&& go_, auto const n)
        {
            if (n->is_terminal())
            {
                auto const newVal = static_cast<uint_t>(f(n->get_value()));
                return nodes_.terminal_node(newVal);
            }
            else
            {
                auto const i = n->get_index();
                return nodes_.internal_node(i, nodes_.make_sons(i,
                    [&go_, &f, n](auto const k)
                {
                    return go_(go_, n->get_son(k));
                }));
            }
        };
        return go(go, root);
    }


    template<class Data, degree Degree, domain Domain>
    diagram_manager<Data, Degree, Domain>::diagram_manager
        ( std::size_t          vars
        , std::size_t          nodes
        , std::vector<index_t> order )
        requires(domains::is_fixed<Domain>()()) :
        nodes_ ( vars
               , nodes
               , detail::default_or_fwd(vars, order) )
    {
    }

    template<class Data, degree Degree, domain Domain>
    diagram_manager<Data, Degree, Domain>::diagram_manager
        ( std::size_t          vars
        , std::size_t          nodes
        , domains::mixed       ds
        , std::vector<index_t> order )
        requires(domains::is_mixed<Domain>()()) :
        nodes_ ( vars
               , nodes
               , detail::default_or_fwd(vars, order)
               , std::move(ds) )
    {
    }
}

#endif