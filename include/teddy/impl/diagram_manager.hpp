#ifndef MIX_DD_DIAGRAM_MANAGER_HPP
#define MIX_DD_DIAGRAM_MANAGER_HPP

#include "node_manager.hpp"
#include "diagram.hpp"
#include "utils.hpp"

namespace teddy
{
    template<class Data, degree Degree, domain Domain>
    class diagram_manager
    {
    public:
        using diagram_t = diagram<Data, Degree>;

    public:
        auto constant (uint_t) -> diagram_t;
        auto variable (index_t) -> diagram_t;

        template<class Op>
        auto apply (diagram_t const&, diagram_t const&) -> diagram_t;

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