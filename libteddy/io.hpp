#ifndef LIBTEDDY_CORE_IO_HPP
#define LIBTEDDY_CORE_IO_HPP

#include <libteddy/core.hpp>
#include <libteddy/details/io_impl.hpp>
#include <libteddy/details/pla_file.hpp>

#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <iterator>

namespace teddy
{
struct io
{
    /**
     *  \brief Creates BDDs defined by PLA file
     *
     *  \param manager Diagram manager
     *  \param file PLA file loaded in the instance of \c pla_file class
     *  \param foldType fold type used in diagram creation
     *  \return Vector of diagrams
     */
    static auto from_pla (
        bdd_manager& manager,
        pla_file const& file,
        fold_type foldType = fold_type::Tree
    ) -> std::vector<bdd_manager::diagram_t>;

    /**
     *  \brief Creates diagram from a truth vector of a function
     *
     *  Example for the function f(x) = max(x0, x1, x2):
     *  \code
     *  // Truth table:
     *  +----+----+----+----++----+-----+----+---+
     *  | x1 | x2 | x3 | f  || x1 |  x2 | x3 | f |
     *  +----+----+----+----++----+-----+----+---+
     *  | 0  | 0  | 0  | 0  || 1  |  0  | 0  | 1 |
     *  | 0  | 0  | 1  | 1  || 1  |  0  | 1  | 1 |
     *  | 0  | 0  | 2  | 2  || 1  |  0  | 2  | 2 |
     *  | 0  | 1  | 0  | 1  || 1  |  1  | 0  | 1 |
     *  | 0  | 1  | 1  | 1  || 1  |  1  | 1  | 1 |
     *  | 0  | 1  | 2  | 2  || 1  |  1  | 2  | 2 |
     *  +----+----+----+----++----+-----+----+---+
     *  \endcode
     *
     *  \code
     *  // Truth vector:
     *  [0 1 2 1 1 2 1 1 2 1 1 2]
     *  \endcode
     *
     *  \code
     *  // Teddy code:
     *  teddy::ifmdd_manager<3> manager(3, 100, {2, 2, 3});
     *  std::vector<int> vec {0, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2};
     *  diagram_t f = manager.from_vector(vec);
     *  \endcode
     *
     *  \tparam I Iterator type for the input range
     *  \tparam S Sentinel type for \p I (end iterator)
     *  \param manager Diagram manager
     *  \param first Iterator to the first element of the truth vector
     *  \param last Sentinel for \p first (end iterator)
     *  \return Diagram representing function given by the truth vector
     */
    template<
        class Data,
        class Degree,
        class Domain,
        std::input_iterator I,
        std::sentinel_for<I> S>
    static auto from_vector (
        diagram_manager<Data, Degree, Domain>& manager,
        I first,
        S last
    ) -> diagram_manager<Data, Degree, Domain>::diagram_t;

    /**
     *  \brief Creates diagram from a truth vector of a function
     *
     *  See the other overload for details.
     *
     *  \tparam R Type of the range that contains the truth vector
     *  \param vector Range representing the truth vector
     *  Elements of the range must be convertible to int
     *  \return Diagram representing function given by the truth vector
     */
    template<class Data, class Degree, class Domain, std::ranges::input_range R>
    static auto from_vector (
        diagram_manager<Data, Degree, Domain>& manager,
        R const& vector
    ) -> diagram_manager<Data, Degree, Domain>::diagram_t;

    /**
     *  \brief Creates diagram from a truth vector of a function
     *
     *  See the other overload for details.
     *
     *  \param vector Range representing the truth vector
     *  Elements of the range must be convertible to int
     *  \return Diagram representing function given by the truth vector
     */
    template<class Data, class Degree, class Domain>
    static auto from_vector (
        diagram_manager<Data, Degree, Domain>& manager,
        std::initializer_list<int> vector
    ) -> diagram_manager<Data, Degree, Domain>::diagram_t;

    /**
     *  \brief Creates truth vector from the diagram
     *
     *  Significance of variables is the same as in the \c from_vector
     *  function i.e., variable on the last level of the diagram is
     *  least significant. The following assertion holds:
     *  \code
     *  assert(manager.from_vector(manager.to_vector(d)))
     *  \endcode
     *
     *  \param manager Diagram manager
     *  \param diagram Diagram
     *  \return Vector of ints representing the truth vector
     */
    template<class Data, class Degree, class Domain>
    static auto to_vector (
        diagram_manager<Data, Degree, Domain> const& manager,
        diagram_manager<Data, Degree, Domain>::diagram_t const& diagram
    ) -> std::vector<int32>;

    /**
     *  \brief Creates truth vector from the diagram
     *  \tparam O Output iterator type
     *  \param manager Diagram manager
     *  \param diagram Diagram
     *  \param out Output iterator that is used to output the truth vector
     */
    template<
        class Data,
        class Degree,
        class Domain,
        std::output_iterator<teddy::int32> O>
    static auto to_vector_g (
        diagram_manager<Data, Degree, Domain> const& manager,
        diagram_manager<Data, Degree, Domain>::diagram_t const& diagram,
        O out
    ) -> void;

    /**
     *  \brief Prints dot representation of the graph
     *
     *  Prints dot representation of the entire multi rooted graph to
     *  the output stream.
     *
     *  \param manager Diagram manager
     *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
     */
    template<class Data, class Degree, class Domain>
    static auto to_dot (
        diagram_manager<Data, Degree, Domain> const& manager,
        std::ostream& out
    ) -> void;

    /**
     *  \brief Prints dot representation of the diagram
     *
     *  Prints dot representation of the diagram to
     *  the output stream.
     *
     *  \param manager Diagram manager
     *  \param out Output stream (e.g. \c std::cout or \c std::ofstream )
     *  \param diagram Diagram
     */
    template<class Data, class Degree, class Domain>
    static auto to_dot (
        diagram_manager<Data, Degree, Domain> const& manager,
        std::ostream& out,
        diagram_manager<Data, Degree, Domain>::diagram_t const& diagram
    ) -> void;
};

// definitions:

inline auto io::from_pla(
    bdd_manager& manager,
    pla_file const& file,
    fold_type const foldType
) -> std::vector<bdd_manager::diagram_t>
{
    using bdd_t        = bdd_manager::diagram_t;
    auto const product = [&manager] (auto const& cube)
    {
        std::vector<bdd_t> variables;
        variables.reserve(as_usize(cube.size()));
        for (int32 i = 0; i < cube.size(); ++i)
        {
            if (cube.get(i) == 1)
            {
                variables.push_back(manager.variable(i));
            }
            else if (cube.get(i) == 0)
            {
                variables.push_back(manager.variable_not(i));
            }
        }
        return manager.left_fold<ops::AND>(variables);
    };

    auto const sum = [&manager, foldType] (auto& diagrams)
    {
        switch (foldType)
        {
        case fold_type::Left:
            return manager.left_fold<ops::OR>(diagrams);

        case fold_type::Tree:
            return manager.tree_fold<ops::OR>(diagrams);

        default:
            std::puts("Invalid fold type. This should not have happened!");
            std::exit(1);
        }
    };

    std::vector<pla_file::pla_line> const& plaLines = file.get_lines();
    int64 const lineCount                           = file.get_line_count();
    int64 const functionCount                       = file.get_function_count();

    // Create a diagram for each function.
    std::vector<bdd_t> functionDiagrams;
    functionDiagrams.reserve(as_usize(functionCount));
    for (int32 fi = 0; fi < functionCount; ++fi)
    {
        // First create a diagram for each product.
        std::vector<bdd_t> products;
        // products.reserve(lineCount);
        for (int32 li = 0; li < lineCount; ++li)
        {
            // We are doing SOP so we are only interested
            // in functions with value 1.
            if (plaLines[as_usize(li)].fVals_.get(fi) == 1)
            {
                products.push_back(product(plaLines[as_uindex(li)].cube_));
            }
        }

        // In this case we just have a constant function.
        if (products.empty())
        {
            products.push_back(manager.constant(0));
        }

        // Then merge products using OR.
        functionDiagrams.push_back(sum(products));
    }

    return functionDiagrams;
}

template<
    class Data,
    class Degree,
    class Domain,
    std::input_iterator I,
    std::sentinel_for<I> S>
auto io::from_vector( // NOLINT
    diagram_manager<Data, Degree, Domain>& manager,
    I first,
    S last
) -> diagram_manager<Data, Degree, Domain>::diagram_t
{
    using manager_t     = diagram_manager<Data, Degree, Domain>;
    using diagram_t     = typename manager_t::diagram_t;
    using son_container = typename manager_t::son_container;
    using node_t        = typename manager_t::node_t;

    using stack_frame   = struct
    {
        node_t* node;
        int32 level;
    };

    if (0 == manager.get_var_count())
    {
        assert(first != last && std::next(first) == last);
        return manager.constant(*first);
    }

    int32 const lastLevel    = manager.get_var_count() - 1;
    int32 const lastIndex    = manager.nodes_.get_index(lastLevel);
    int32 const preLastIndex = manager.nodes_.get_index(lastLevel - 1);

    if constexpr (std::random_access_iterator<I>)
    {
        [[maybe_unused]] int64 const count
            = manager.nodes_.domain_product(0, lastLevel + 1);
        [[maybe_unused]] auto const dist = std::distance(first, last);
        assert(dist > 0 && dist == count);
    }

    std::vector<stack_frame> stack;
    auto const shrink_stack = [&manager, &stack] ()
    {
        for (;;)
        {
            int32 const currentLevel = stack.back().level;
            if (0 == currentLevel)
            {
                break;
            }

            auto const endId = rend(stack);
            auto stackIt     = rbegin(stack);
            int64 count      = 0;
            while (stackIt != endId && stackIt->level == currentLevel)
            {
                ++stackIt;
                ++count;
            }
            int32 const newIndex  = manager.nodes_.get_index(currentLevel - 1);
            int32 const newDomain = manager.nodes_.get_domain(newIndex);

            // TODO(michal): simplify
            if (count < newDomain)
            {
                break;
            }

            son_container newSons = node_t::make_son_container(newDomain);
            for (int32 k = 0; k < newDomain; ++k)
            {
                newSons[k]
                    = stack[as_uindex(ssize(stack) - newDomain + k)].node;
            }
            node_t* const newNode = manager.nodes_.make_internal_node(
                newIndex,
                TEDDY_MOVE(newSons)
            );
            stack.erase(end(stack) - newDomain, end(stack));
            stack.push_back(stack_frame {newNode, currentLevel - 1});
        }
    };

    int32 const lastDomain    = manager.nodes_.get_domain(lastIndex);
    int32 const preLastDomain = manager.nodes_.get_domain(preLastIndex);
    while (first != last)
    {
        for (int32 o = 0; o < preLastDomain; ++o)
        {
            son_container sons = node_t::make_son_container(lastDomain);
            for (int32 k = 0; k < lastDomain; ++k)
            {
                sons[k] = manager.nodes_.make_terminal_node(*first++);
            }
            node_t* const node = manager.nodes_.make_internal_node(
                lastIndex,
                TEDDY_MOVE(sons)
            );
            stack.push_back(stack_frame {node, lastLevel});
        }
        shrink_stack();
    }

    assert(ssize(stack) == 1);
    return diagram_t(stack.back().node);
}

template<class Data, class Degree, class Domain, std::ranges::input_range R>
auto io::from_vector(
    diagram_manager<Data, Degree, Domain>& manager,
    R const& vector
) -> diagram_manager<Data, Degree, Domain>::diagram_t
{
    return io::from_vector(manager, begin(vector), end(vector));
}

template<class Data, class Degree, class Domain>
auto io::from_vector(
    diagram_manager<Data, Degree, Domain>& manager,
    std::initializer_list<int> const vector
) -> diagram_manager<Data, Degree, Domain>::diagram_t
{
    return io::from_vector(manager, begin(vector), end(vector));
}

template<class Data, class Degree, class Domain>
auto io::to_vector(
    diagram_manager<Data, Degree, Domain> const& manager,
    diagram_manager<Data, Degree, Domain>::diagram_t const& diagram
) -> std::vector<int32>
{
    std::vector<int32> vector;
    vector.reserve(
        as_usize(manager.nodes_.domain_product(0, manager.get_var_count()))
    );
    io::to_vector_g(manager, diagram, std::back_inserter(vector));
    return vector;
}

template<
    class Data,
    class Degree,
    class Domain,
    std::output_iterator<teddy::int32> O>
auto io::to_vector_g(
    diagram_manager<Data, Degree, Domain> const& manager,
    diagram_manager<Data, Degree, Domain>::diagram_t const& diagram,
    O out
) -> void
{
    // TODO(michal): move for-each-in-domain to tools
    if (manager.get_var_count() == 0)
    {
        assert(diagram.unsafe_get_root()->is_terminal());
        *out++ = diagram.unsafe_get_root()->get_value();
        return;
    }

    std::vector<int32> vars(as_usize(manager.get_var_count()));
    bool wasLast = false;
    do
    {
        *out++        = manager.evaluate(diagram, vars);
        bool overflow = true;
        int32 level   = manager.nodes_.get_leaf_level();
        while (level > 0 && overflow)
        {
            --level;
            int32 const index = manager.nodes_.get_index(level);
            ++vars[as_uindex(index)];
            overflow
                = vars[as_uindex(index)] == manager.nodes_.get_domain(index);
            if (overflow)
            {
                vars[as_uindex(index)] = 0;
            }

            wasLast = overflow && 0 == level;
        }
    } while (not wasLast);
}

template<class Data, class Degree, class Domain>
auto io::to_dot(
    diagram_manager<Data, Degree, Domain> const& manager,
    std::ostream& out
) -> void
{
    details::io_impl::to_dot_graph_common(
        manager,
        out,
        [&manager] (auto const& f) { manager.nodes_.for_each_node(f); }
    );
}

template<class Data, class Degree, class Domain>
auto io::to_dot(
    diagram_manager<Data, Degree, Domain> const& manager,
    std::ostream& out,
    diagram_manager<Data, Degree, Domain>::diagram_t const& diagram
) -> void
{
    details::io_impl::to_dot_graph_common(
        manager,
        out,
        [&manager, &diagram] (auto const& f)
        { manager.nodes_.traverse_level(diagram.unsafe_get_root(), f); }
    );
}
} // namespace teddy
#endif