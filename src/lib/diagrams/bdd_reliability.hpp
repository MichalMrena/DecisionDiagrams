#ifndef MIX_DD_BDD_RELIABILITY_HPP
#define MIX_DD_BDD_RELIABILITY_HPP

#include "bdd.hpp"
#include "bdd_manipulator.hpp"
#include "mdd_reliability.hpp"
#include "../utils/hash.hpp"
#include "../utils/alloc_manager.hpp"
#include "../utils/more_iterator.hpp"
#include "../utils/more_vector.hpp"
#include "../utils/more_algorithm.hpp"

#include <limits>
#include <utility>
#include <iterator>

namespace mix::dd
{
    template<class VertexData, class ArcData, class Allocator>
    class bdd_reliability : public mdd_reliability<VertexData, ArcData, 2, Allocator>
    {
    public:
        using bdd_t    = bdd<VertexData, ArcData, Allocator>;
        using vertex_t = typename bdd_t::vertex_t;
        using arc_t    = typename bdd_t::arc_t;
        using prob_v   = std::vector<double>;

    public:
        explicit bdd_reliability (Allocator const& alloc = Allocator {});

        auto calculate_probabilities (bdd_t& f, prob_v const& ps)  -> void;
        auto get_availability        (bdd_t const& f)              -> double;
        auto get_unavailability      (bdd_t const& f)              -> double;
        auto availability            (bdd_t&  f, prob_v const& ps) -> double;
        auto availability            (bdd_t&& f, prob_v const& ps) -> double;
        auto unavailability          (bdd_t&  f, prob_v const& ps) -> double;
        auto unavailability          (bdd_t&& f, prob_v const& ps) -> double;

        auto dpbds (bdd_t sf)                  -> std::vector<bdd_t>;
        auto dpbd  (bdd_t sf, index_t const i) -> bdd_t;

        auto structural_importance  (bdd_t&  dpbd) -> double;
        auto structural_importance  (bdd_t&& dpbd) -> double;
        auto structural_importances (std::vector<bdd_t>&  dpbds) -> std::vector<double>;
        auto structural_importances (std::vector<bdd_t>&& dpbds) -> std::vector<double>;

        auto birnbaum_importance  (bdd_t&  dpbd, prob_v const& ps) -> double;
        auto birnbaum_importance  (bdd_t&& dpbd, prob_v const& ps) -> double;
        auto birnbaum_importances (std::vector<bdd_t>&  dpbds, prob_v const& ps) -> std::vector<double>;
        auto birnbaum_importances (std::vector<bdd_t>&& dpbds, prob_v const& ps) -> std::vector<double>;

        auto criticality_importance  (bdd_t&  dpbd, prob_v const& ps, double const U, index_t const i) -> double;
        auto criticality_importance  (bdd_t&& dpbd, prob_v const& ps, double const U, index_t const i) -> double;
        auto criticality_importances (std::vector<bdd_t>&  dpbds, prob_v const& ps, double const U)    -> std::vector<double>;
        auto criticality_importances (std::vector<bdd_t>&& dpbds, prob_v const& ps, double const U)    -> std::vector<double>;

        auto fussell_vesely_importance  (bdd_t dpbd, prob_v const& ps, double const U, index_t const i) -> double;
        auto fussell_vesely_importances (std::vector<bdd_t> dpbds, prob_v const& ps, double const U)    -> std::vector<double>;

        template<class VectorType>
        auto mcvs (std::vector<bdd_t> dpbds) -> std::vector<VectorType>;

    private:
        using vertex_pair_t = std::pair<const vertex_t*, const vertex_t*>;
        using memo_map      = std::unordered_map<const vertex_pair_t, vertex_t*, utils::tuple_hash_t<const vertex_pair_t>>;
        using manager_t     = utils::alloc_manager<Allocator>;
        using manipulator_t = bdd_manipulator<VertexData, ArcData, Allocator>;
        using base_t        = mdd_reliability<VertexData, ArcData, 2, Allocator>;
        using prob_table    = typename base_t::prob_table;

    private:
        auto to_prob_table  (prob_v const& ps) -> prob_table;
        auto mnf            (bdd_t dpbd) -> bdd_t;
        auto to_dpbd_e      (bdd_t d, index_t const i, bool_t const from) -> bdd_t;
        auto find_positions (bdd_t const& d, index_t const i)             -> std::vector<std::pair<vertex_t*, bool_t>>;
        auto new_vertex     ( vertex_t* const low
                            , vertex_t* const high
                            , index_t   const i
                            , bool      const mark) -> vertex_t*;
 
        // TODO static from manipulator
        auto is_redundant (vertex_t const* const v ) const -> bool;

    private:
        memo_map memo_;
        id_t     nextId_;
    };

    template<class VertexData, class ArcData, class Allocator>
    bdd_reliability<VertexData, ArcData, Allocator>::bdd_reliability
        (Allocator const& alloc) :
        base_t   {alloc},
        nextId_  {0}
    {
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::calculate_probabilities
        (bdd_t& f, prob_v const& ps) -> void
    {
        base_t::calculate_probabilities(f, this->to_prob_table(ps));
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::get_availability
        (bdd_t const& f) -> double
    {
        return base_t::get_probability(f, 1);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::get_unavailability
        (bdd_t const& f) -> double
    {
        return 1 - this->get_availability(f);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::availability
        (bdd_t& f, prob_v const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_probability(f, 1);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::availability
        (bdd_t&&f, prob_v const& ps) -> double
    {
        return this->availability(f, ps);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::unavailability
        (bdd_t& f, prob_v const& ps) -> double
    {
        this->calculate_probabilities(f, ps);
        return this->get_probability(f, 0);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::unavailability
        (bdd_t&&f, prob_v const& ps) -> double
    {
        return this->unavailability(f, ps);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::dpbds
        (bdd_t sf) -> std::vector<bdd_t>
    {
        auto const varCount = sf.variable_count();
        auto dpbds = utils::vector<bdd_t>(varCount);

        for (auto i = 0u; i < varCount - 1; ++i)
        {
            dpbds.emplace_back(this->dpbd(sf, i));
        }
        dpbds.emplace_back(this->dpbd(std::move(sf), varCount - 1));

        return dpbds;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::dpbd
        (bdd_t sf, index_t const i) -> bdd_t
    {
        auto m      = manipulator_t {base_t::manager_.get_alloc()};
        auto sfCopy = sf.clone();
        return m.apply( m.negate(m.restrict_var(std::move(sf), i, 0))
                      , AND()
                      , m.restrict_var(std::move(sfCopy), i, 1) );
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::structural_importance
        (bdd_t& dpbd) -> double
    {
        auto const sc = static_cast<double>(dpbd.satisfy_count() / 2);
        return sc / utils::two_pow(dpbd.variable_count() - 1);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::structural_importance
        (bdd_t&& dpbd) -> double
    {
        return this->structural_importance(dpbd);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::structural_importances
        (std::vector<bdd_t>& dpbds) -> std::vector<double>
    {
        auto sis = utils::vector<double>(dpbds.size());

        for (auto& dpbd : dpbds)
        {
            sis.emplace_back(this->structural_importance(dpbd));
        }

        return sis;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::structural_importances
        (std::vector<bdd_t>&& dpbds) -> std::vector<double>
    {
        return this->structural_importances(dpbds);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::birnbaum_importance
        (bdd_t& dpbd, prob_v const& ps) -> double
    {
        return this->availability(dpbd, ps);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::birnbaum_importance
        (bdd_t&& dpbd, prob_v const& ps) -> double
    {
        return this->birnbaum_importance(dpbd, ps);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::birnbaum_importances
        (std::vector<bdd_t>& dpbds, prob_v const& ps) -> std::vector<double>
    {
        auto bis = utils::vector<double>(dpbds.size());

        for (auto& dpbd : dpbds)
        {
            bis.emplace_back(this->birnbaum_importance(dpbd, ps));
        }

        return bis;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::birnbaum_importances
        (std::vector<bdd_t>&& dpbds, prob_v const& ps) -> std::vector<double>
    {
        return this->birnbaum_importances(dpbds, ps);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::criticality_importance
        (bdd_t& dpbd, prob_v const& ps, double const U, index_t const i) -> double
    {
        auto const bi = this->birnbaum_importance(dpbd, ps);
        auto const qi = 1 - ps.at(i);
        return bi * (qi / U);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::criticality_importance
        (bdd_t&& dpbd, prob_v const& ps, double const U, index_t const i) -> double
    {
        return this->criticality_importance(dpbd, ps, U, i);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::criticality_importances
        (std::vector<bdd_t>& dpbds, prob_v const& ps, double const U) -> std::vector<double>
    {
        auto cis = utils::vector<double>(dpbds.size());
        auto is  = utils::range(0u, dpbds.size());

        for (auto&& [dpbd, i] : utils::zip(dpbds, is))
        {
            cis.emplace_back(this->criticality_importance(dpbd, ps, U, i));
        }

        return cis;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::criticality_importances
        (std::vector<bdd_t>&& dpbds, prob_v const& ps, double const U) -> std::vector<double>
    {
        return this->criticality_importances(dpbds, ps, U);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::fussell_vesely_importance
        (bdd_t dpbd, prob_v const& ps, double const U, index_t const i) -> double
    {
        auto const prMnf = this->availability(this->mnf(std::move(dpbd)), ps);
        auto const qi    = 1 - ps[i];
        return qi * (prMnf / U);
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::fussell_vesely_importances
        (std::vector<bdd_t> dpbds, prob_v const& ps, double const U) -> std::vector<double>
    {
        auto is   = utils::range(0u, dpbds.size());
        auto fvis = utils::vector<double>(dpbds.size());

        for (auto&& [dpbd, i] : utils::zip(dpbds, is))
        {
            fvis.emplace_back(this->fussell_vesely_importance(std::move(dpbd), ps, U, i));
        }

        return fvis;
    }

    template<class VertexData, class ArcData, class Allocator>
    template<class VectorType>
    auto bdd_reliability<VertexData, ArcData, Allocator>::mcvs
        (std::vector<bdd_t> dpbds) -> std::vector<VectorType>
    {
        auto is     = utils::range(0u, dpbds.size());
        auto dpbdes = utils::vector<bdd_t>(dpbds.size());
        for (auto&& [dpbd, i] : utils::zip(dpbds, is))
        {
            dpbdes.emplace_back(this->to_dpbd_e(std::move(dpbd), i, 0));
        }

        auto m   = manipulator_t {base_t::manager_.get_alloc()};
        auto it  = std::next(std::begin(dpbdes));
        auto end = std::end(dpbdes);
        while (it != end)
        {
            dpbdes.front() = m.apply( std::move(dpbdes.front())
                                    , PI_CONJ {}
                                    , std::move(*it++) );
        }

        auto cutVectors = std::vector<VectorType>();
        dpbdes.front().template satisfy_all<VectorType>(std::back_inserter(cutVectors));
        return cutVectors;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::to_prob_table
        (prob_v const& ps) -> prob_table
    {
        return utils::map(ps, [](auto const p)
        {
            return std::array<double, 2> {1 - p, p};
        });
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::mnf
        (bdd_t dpbd) -> bdd_t
    {
        auto redundantVs = std::vector<vertex_t*>();
        auto const maybe_skip_son = [this, &dpbd](auto const v, auto const i)
        {
            if (!dpbd.is_leaf(v) && !dpbd.is_leaf(v->son(i)) && this->is_redundant(v->son(i)))
            {
                v->son(i) = v->son(i)->son(0);
            }
        };

        auto const falseLeaf = dpbd.false_leaf();
        auto falseLeafInputDegree = 0u;

        dpbd.traverse_post(dpbd.root_, [&](auto const v)
        {
            maybe_skip_son(v, 0);
            maybe_skip_son(v, 1);
            if (!dpbd.is_leaf(v) && falseLeaf == v->son(0))
            {
                v->son(0) = v->son(1);
                redundantVs.emplace_back(v);
                ++falseLeafInputDegree;
            }

            if (!dpbd.is_leaf(v) && falseLeaf == v->son(1))
            {
                ++falseLeafInputDegree;
            }
        });

        if (this->is_redundant(dpbd.root_))
        {
            dpbd.root_ = dpbd.root_->son(0);
        }

        if (falseLeafInputDegree == redundantVs.size())
        {
            dpbd.leafToVal_.erase(falseLeaf);
            base_t::manager_.release(falseLeaf);
        }

        for (auto const v : redundantVs)
        {
            base_t::manager_.release(v);
        }

        return dpbd;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::to_dpbd_e
        (bdd_t d, index_t const i, bool_t const from) -> bdd_t
    {
        nextId_                    = std::numeric_limits<id_t>::max();
        auto const insertPositions = this->find_positions(d, i);
        auto const undefinedLeaf   = this->new_vertex(nullptr, nullptr, d.leaf_index(), d.root_->mark);
        d.leafToVal_.emplace(undefinedLeaf, log_val_traits<2>::undefined);

        for (auto [vertex, sonIndex] : insertPositions)
        {
            auto const target     = vertex->son(sonIndex);
            auto const low        = 0 == from ? target : undefinedLeaf;
            auto const high       = 0 == from ? undefinedLeaf : target;
            vertex->son(sonIndex) = this->new_vertex(low, high, i, d.root_->mark);
        }

        if (0 == i)
        {
            auto const low  = 0 == from ? d.root_ : undefinedLeaf;
            auto const high = 0 == from ? undefinedLeaf : d.root_;
            d.root_         = this->new_vertex(low, high, i, d.root_->mark);
        }

        memo_.clear();
        return d;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::find_positions
        (bdd_t const& d, index_t const i) -> std::vector<std::pair<vertex_t*, bool_t>>
    {
        auto positions = std::vector<std::pair<vertex_t*, bool_t>>();

        d.traverse_pre(d.root_, [&d, &positions, i](auto const v)
        {
            if (d.is_leaf(v) || v->index > i)
            {
                return;
            }

            if (v->son(0)->index > i)
            {
                positions.emplace_back(v, 0);
            }

            if (v->son(1)->index > i)
            {
                positions.emplace_back(v, 1);
            }
        });

        return positions;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::new_vertex
        (vertex_t* const low, vertex_t* const high, index_t const i, bool const mark) -> vertex_t*
    {
        using arr_t = typename vertex_t::star_arr;

        auto const key = vertex_pair_t {low, high};
        auto const it  = memo_.find(key);

        if (it != memo_.end())
        {
            it->second->mark = mark;
            return it->second;
        }

        auto const v = base_t::manager_.create(nextId_--, i, arr_t {arc_t {low}, arc_t {high}}, mark);
        memo_.emplace(key, v);

        return v;
    }

    template<class VertexData, class ArcData, class Allocator>
    auto bdd_reliability<VertexData, ArcData, Allocator>::is_redundant
        (const vertex_t* const v) const -> bool
    {
        return std::all_of( std::begin(v->forwardStar), std::end(v->forwardStar)
                          , [fid = v->son(0)->id] (auto&& arc) { return arc.target->id == fid; } );
    }
}

#endif