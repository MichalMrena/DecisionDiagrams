#ifndef MIX_DD_MDD_RELIABILITY_HPP
#define MIX_DD_MDD_RELIABILITY_HPP

#include "bdd.hpp"

#include <numeric>
#include <functional>

namespace mix::dd
{
    template<std::size_t P>
    struct val_change
    {
        using log_t = typename log_val_traits<P>::type;
        log_t from;
        log_t to;
    };

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    class mdd_reliability
    {
    public:
        using mdd_t       = std::conditional_t< 2 == P
                                              , bdd<VertexData, ArcData, Allocator>
                                              , mdd<VertexData, ArcData, P, Allocator> >;
        using vertex_t   = typename mdd_t::vertex_t;
        using arc_t      = typename mdd_t::arc_t;
        using log_t      = typename log_val_traits<P>::type;
        using prob_table = std::vector<std::array<double, P>>;

    public:
        explicit mdd_reliability (Allocator const& alloc = Allocator {});

        auto calculate_probabilities (mdd_t& f, prob_table const& ps)                     -> void;
        auto get_probability         (mdd_t const& f, log_t const level)                  -> double;
        auto get_availability        (mdd_t const& f, log_t const level)                  -> double;
        auto get_unavailability      (mdd_t const& f, log_t const level)                  -> double;
        auto availability            (mdd_t&  f, log_t const level, prob_table const& ps) -> double;
        auto availability            (mdd_t&& f, log_t const level, prob_table const& ps) -> double;
        auto unavailability          (mdd_t&  f, log_t const level, prob_table const& ps) -> double;
        auto unavailability          (mdd_t&& f, log_t const level, prob_table const& ps) -> double;

        auto dpbd              (mdd_t sf, index_t const i, val_change<P> const var, val_change<P> const f) -> mdd_t;
        auto dpbd_integrated_1 (mdd_t sf, index_t const i, val_change<P> const var, log_t const fVal)      -> mdd_t;
        auto dpbd_integrated_2 (mdd_t sf, index_t const i, val_change<P> const var)                        -> mdd_t;
        auto dpbd_integrated_3 (mdd_t sf, index_t const i, val_change<P> const var, log_t const fVal)      -> mdd_t;

        auto dpbds              (mdd_t sf, val_change<P> const var, val_change<P> const f) -> std::vector<mdd_t>;
        auto dpbds_integrated_1 (mdd_t sf, val_change<P> const var, log_t const fVal)      -> std::vector<mdd_t>;
        auto dpbds_integrated_2 (mdd_t sf, val_change<P> const var)                        -> std::vector<mdd_t>;
        auto dpbds_integrated_3 (mdd_t sf, val_change<P> const var, log_t const fVal)      -> std::vector<mdd_t>;

        auto structural_importance  (mdd_t&  dpbd, std::size_t const domainSize)                       -> double;
        auto structural_importance  (mdd_t&& dpbd, std::size_t const domainSize)                       -> double;
        auto structural_importance  (mdd_t&  dpbd, index_t const i, std::vector<log_t> const& domains) -> double;
        auto structural_importance  (mdd_t&& dpbd, index_t const i, std::vector<log_t> const& domains) -> double;
        auto structural_importances (std::vector<mdd_t>&  dpbds, std::vector<log_t> const& domains)    -> std::vector<double>;
        auto structural_importances (std::vector<mdd_t>&& dpbds, std::vector<log_t> const& domains)    -> std::vector<double>;

        auto birnbaum_importance  (mdd_t&  dpbd, prob_table const& ps) -> double;
        auto birnbaum_importance  (mdd_t&& dpbd, prob_table const& ps) -> double;
        auto birnbaum_importances (std::vector<mdd_t>&  dpbds, prob_table const& ps) -> std::vector<double>;
        auto birnbaum_importances (std::vector<mdd_t>&& dpbds, prob_table const& ps) -> std::vector<double>;

    private:
        auto sum_terminals (mdd_t const& f, log_t const from, log_t const to) -> double;

        template<class Derivative>
        auto dpbds_impl (mdd_t& sf, Derivative&& d) -> std::vector<mdd_t>;

    protected:
        using manager_t     = utils::alloc_manager<Allocator>;
        using manipulator_t = mdd_manipulator<VertexData, ArcData, P, Allocator>;
        using creator_t     = mdd_creator<VertexData, ArcData, P, Allocator>;

    protected:
        manager_t manager_;
    };

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    mdd_reliability<VertexData, ArcData, P, Allocator>::mdd_reliability
        (Allocator const& alloc) :
        manager_ {alloc}
    {
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::calculate_probabilities
        (mdd_t& f, prob_table const& ps) -> void
    {
        for (auto& vertex : f)
        {
            vertex.data = 0.0;
        }
        f.get_root()->data = 1.0;

        for (auto& vertex : f)
        {
            if (!f.is_leaf(&vertex))
            {
                for (auto i = 0u; i < P; ++i)
                {
                    vertex.get_son(i)->data += vertex.data * ps[vertex.get_index()][i];
                }
            }
        }
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::get_probability
        (mdd_t const& f, log_t const level) -> double
    {
        auto const leaf = f.get_leaf(level);
        return leaf ? leaf->data : 0;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::get_availability
        (mdd_t const& f, log_t const level) -> double
    {
        return this->sum_terminals(f, level, P);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::get_unavailability
        (mdd_t const& f, log_t const level) -> double
    {
        return this->sum_terminals(f, 0, level);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::availability
        (mdd_t& f, log_t const level, prob_table const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_availability(f, level);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::availability
        (mdd_t&& f, log_t const level, prob_table const& ps) -> double
    {
        return this->availability(f, level, ps);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::unavailability
        (mdd_t& f, log_t const level, prob_table const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_unavailability(f, level);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::unavailability
        (mdd_t&& f, log_t const level, prob_table const& ps) -> double
    {
        return this->unavailability(f, level, ps);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbd
        (mdd_t sf, index_t const i, val_change<P> const var, val_change<P> const f) -> mdd_t
    {
        using equal_t = EQUAL_TO<P, domain_e::nonhomogenous>;
        using and_t   = AND<P, domain_e::nonhomogenous>;

        auto m = manipulator_t(manager_.get_alloc());
        auto c = creator_t(manager_.get_alloc());

        auto lhs = m.apply( m.restrict_var(sf.clone(), i, var.from)
                          , equal_t()
                          , c.just_val(f.from) );
        auto rhs = m.apply( m.restrict_var(sf.move(), i, var.to)
                          , equal_t()
                          , c.just_val(f.to) );

        return m.apply(std::move(lhs), and_t(), std::move(rhs));
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbd_integrated_1
        (mdd_t sf, index_t const i, val_change<P> const var, log_t const fVal) -> mdd_t
    {
        using equal_t = EQUAL_TO<P, domain_e::nonhomogenous>;
        using less_t  = LESS<P, domain_e::nonhomogenous>;
        using and_t   = AND<P, domain_e::nonhomogenous>;

        auto m = manipulator_t(manager_.get_alloc());
        auto c = creator_t(manager_.get_alloc());

        auto lhs = m.apply( m.restrict_var(sf.clone(), i, var.from)
                          , equal_t()
                          , c.just_val(fVal) );
        auto rhs = m.apply( m.restrict_var(sf.move(), i, var.to)
                          , less_t()
                          , c.just_val(fVal) );

        return m.apply(std::move(lhs), and_t(), std::move(rhs));
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbd_integrated_2
        (mdd_t sf, index_t const i, val_change<P> const var) -> mdd_t
    {
        using greater_t = GREATER<P, domain_e::nonhomogenous>;

        auto m = manipulator_t(manager_.get_alloc());
        auto c = creator_t(manager_.get_alloc());

        auto lhs = m.restrict_var(sf.clone(), i, var.from);
        auto rhs = m.restrict_var(sf.move(), i, var.to);

        return m.apply(std::move(lhs), greater_t(), std::move(rhs));
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbd_integrated_3
        (mdd_t sf, index_t const i, val_change<P> const var, log_t const fVal) -> mdd_t
    {
        using greater_eq_t = GREATER_EQUAL<P, domain_e::nonhomogenous>;
        using less_t       = LESS<P, domain_e::nonhomogenous>;
        using and_t        = AND<P, domain_e::nonhomogenous>;

        auto m = manipulator_t(manager_.get_alloc());
        auto c = creator_t(manager_.get_alloc());

        auto lhs = m.apply( m.restrict_var(sf.clone(), i, var.from)
                          , greater_eq_t()
                          , c.just_val(fVal) );
        auto rhs = m.apply( m.restrict_var(sf.move(), i, var.to)
                          , less_t()
                          , c.just_val(fVal) );

        return m.apply(std::move(lhs), and_t(), std::move(rhs));
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbds
        (mdd_t sf, val_change<P> const var, val_change<P> const f) -> std::vector<mdd_t>
    {
        using namespace std::placeholders;
        auto const d = std::bind(&mdd_reliability::dpbd, this, _1, _2, var, f);
        return this->dpbds_impl(sf, d);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbds_integrated_1
        (mdd_t sf, val_change<P> const var, log_t const fVal) -> std::vector<mdd_t>
    {
        using namespace std::placeholders;
        auto const d = std::bind(&mdd_reliability::dpbd_integrated_1, this, _1, _2, var, fVal);
        return this->dpbds_impl(sf, d);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbds_integrated_2
        (mdd_t sf, val_change<P> const var) -> std::vector<mdd_t>
    {
        using namespace std::placeholders;
        auto const d = std::bind(&mdd_reliability::dpbd_integrated_2, this, _1, _2, var);
        return this->dpbds_impl(sf, d);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbds_integrated_3
        (mdd_t sf, val_change<P> const var, log_t const fVal) -> std::vector<mdd_t>
    {
        using namespace std::placeholders;
        auto const d = std::bind(&mdd_reliability::dpbd_integrated_3, this, _1, _2, var, fVal);
        return this->dpbds_impl(sf, d);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::structural_importance
        (mdd_t& dpbd, std::size_t const domainSize) -> double
    {
        auto const onesCount = static_cast<double>(dpbd.satisfy_count(1) / P);
        return domainSize ? onesCount / domainSize : 0;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::structural_importance
        (mdd_t&& dpbd, std::size_t const domainSize) -> double
    {
        return this->structural_importance(dpbd, domainSize);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::structural_importance
        (mdd_t& dpbd, index_t const i, std::vector<log_t> const& domains) -> double
    {
        auto const domProduct = std::reduce( std::begin(domains), std::end(domains)
                                           , std::size_t {1}, std::multiplies() );
        return domains[i] ? this->structural_importance(dpbd, i, domProduct / domains[i]) : 0;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::structural_importance
        (mdd_t&& dpbd, index_t const i, std::vector<log_t> const& domains) -> double
    {
        return this->structural_importance(dpbd, i, domains);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::structural_importances
        (std::vector<mdd_t>& dpbds, std::vector<log_t> const& domains) -> std::vector<double>
    {
        auto const domProduct = std::reduce( std::begin(domains), std::end(domains)
                                           , std::size_t {1}, std::multiplies() );
        auto zs = utils::zip(utils::range(0u, dpbds.size()), dpbds);
        return utils::map(zs, dpbds.size(), [this, domProduct, &domains](auto&& pair)
        {
            auto&& [i, d] = pair;
            return domains[i] ? this->structural_importance(d, domProduct / domains[i]) : 0;
        });
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::structural_importances
        (std::vector<mdd_t>&& dpbds, std::vector<log_t> const& domains) -> std::vector<double>
    {
        return this->structural_importances(dpbds, domains);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::birnbaum_importance
        (mdd_t& dpbd, prob_table const& ps) -> double
    {
        this->calculate_probabilities(dpbd, ps);
        return this->get_probability(dpbd, 1);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::birnbaum_importance
        (mdd_t&& dpbd, prob_table const& ps) -> double
    {
        return this->birnbaum_importance(dpbd, ps);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::birnbaum_importances
        (std::vector<mdd_t>& dpbds, prob_table const& ps) -> std::vector<double>
    {
        using namespace std::placeholders;
        std::for_each( std::begin(dpbds), std::end(dpbds)
                     , std::bind(&mdd_reliability::calculate_probabilities, this, _1, std::cref(ps)) );
        return utils::map(dpbds, std::bind(&mdd_reliability::get_probability, this, _1, 1));
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::birnbaum_importances
        (std::vector<mdd_t>&& dpbds, prob_table const& ps) -> std::vector<double>
    {
        return this->birnbaum_importances(dpbds, ps);
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::sum_terminals
        (mdd_t const& f, log_t const from, log_t const to) -> double
    {
        auto sumval = 0.0;
        for (auto i = from; i < to; ++i)
        {
            sumval += this->get_probability(f, i);
        }
        return sumval;
    }

    template<class VertexData, class ArcData, std::size_t P, class Allocator>
    template<class Derivative>
    auto mdd_reliability<VertexData, ArcData, P, Allocator>::dpbds_impl
        (mdd_t& sf, Derivative&& d) -> std::vector<mdd_t>
    {
        auto const varCount = sf.variable_count();
        auto dpbds = utils::vector<mdd_t>(varCount);
 
        for (auto i = 0u; i < varCount - 1; ++i)
        {
            dpbds.emplace_back(d(sf, i));
        }
        dpbds.emplace_back(d(std::move(sf), varCount - 1));

        return dpbds;
    }
}

#endif