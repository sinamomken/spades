#pragma once

#include "config_struct.hpp"
#include "omni/erroneous_connection_remover.hpp"
#include "omni/mf_ec_remover.hpp"
#include "simplification_settings.hpp"
#include "detail_coverage.hpp"

namespace debruijn {
namespace simplification {

template<class Graph>
bool TopologyRemoveErroneousEdges(
    Graph &g,
    const debruijn_graph::debruijn_config::simplification::topology_based_ec_remover& tec_config,
    std::function<void(typename Graph::EdgeId)> removal_handler) {
    INFO("Removing connections based on topology");
    size_t max_length = LengthThresholdFinder::MaxErroneousConnectionLength(
        g.k(), tec_config.max_ec_length_coefficient);

    shared_ptr<Predicate<typename Graph::EdgeId>> condition = make_shared<DefaultUniquenessPlausabilityCondition<Graph>>(g, tec_config.uniqueness_length, tec_config.plausibility_length);

    return omnigraph::RemoveErroneousEdgesInLengthOrder(g, condition, max_length, removal_handler);
}

template<class Graph>
bool MultiplicityCountingRemoveErroneousEdges(
    Graph &g,
    const debruijn_graph::debruijn_config::simplification::topology_based_ec_remover& tec_config,
    std::function<void(typename Graph::EdgeId)> removal_handler) {
    INFO("Removing connections based on topological multiplicity counting");
    size_t max_length = LengthThresholdFinder::MaxErroneousConnectionLength(
        g.k(), tec_config.max_ec_length_coefficient);

    shared_ptr<func::Predicate<typename Graph::EdgeId>> condition
        = make_shared<MultiplicityCountingCondition<Graph>>(g, tec_config.uniqueness_length,
            /*plausibility*/MakePathLengthLowerBound(g, PlausiblePathFinder<Graph>(g, 2 * tec_config.plausibility_length), tec_config.plausibility_length));

    return omnigraph::RemoveErroneousEdgesInLengthOrder(g, condition, max_length, removal_handler);
}

template<class Graph>
bool RemoveThorns(
    Graph &g,
    const debruijn_graph::debruijn_config::simplification::interstrand_ec_remover& isec_config,
    std::function<void(typename Graph::EdgeId)> removal_handler) {
    INFO("Removing interstrand connections");
    size_t max_length = LengthThresholdFinder::MaxErroneousConnectionLength(
        g.k(), isec_config.max_ec_length_coefficient);

    shared_ptr<func::Predicate<typename Graph::EdgeId>> condition
            = func::And<typename Graph::EdgeId>(make_shared<LengthUpperBound<Graph>>(g, max_length),
                                       make_shared<ThornCondition<Graph>>(g, isec_config.uniqueness_length, isec_config.span_distance));

    return omnigraph::RemoveErroneousEdgesInCoverageOrder(g, condition, numeric_limits<double>::max(), removal_handler);
}

template<class Graph>
bool TopologyReliabilityRemoveErroneousEdges(
    Graph &g,
    const debruijn_graph::debruijn_config::simplification::tr_based_ec_remover& trec_config,
    std::function<void(typename Graph::EdgeId)> removal_handler) {
    INFO("Removing connections based on topology and reliable coverage");
    size_t max_length = LengthThresholdFinder::MaxErroneousConnectionLength(
        g.k(), trec_config.max_ec_length_coefficient);

    shared_ptr<func::Predicate<typename Graph::EdgeId>> condition
            = func::And<typename Graph::EdgeId>(make_shared<CoverageUpperBound<Graph>>(g, trec_config.unreliable_coverage),
                                       make_shared<PredicateUniquenessPlausabilityCondition<Graph>>(
                                               g,
                                               /*uniqueness*/MakePathLengthLowerBound(g, UniquePathFinder<Graph>(g), trec_config.uniqueness_length),
                                               /*plausibility*/make_shared<func::AlwaysTrue<typename Graph::EdgeId>>()));

    return omnigraph::RemoveErroneousEdgesInLengthOrder(g, condition, max_length, removal_handler);
}

template<class Graph>
bool MaxFlowRemoveErroneousEdges(
    Graph &g,
    const debruijn_graph::debruijn_config::simplification::max_flow_ec_remover& mfec_config,
    omnigraph::HandlerF<Graph> removal_handler = 0) {
    if (!mfec_config.enabled)
        return false;
    INFO("Removing connections based on max flow strategy");
    size_t max_length = LengthThresholdFinder::MaxErroneousConnectionLength(
        g.k(), (size_t) mfec_config.max_ec_length_coefficient);
    omnigraph::MaxFlowECRemover<Graph> erroneous_edge_remover(
        g, max_length, mfec_config.uniqueness_length,
        mfec_config.plausibility_length, removal_handler);
    return erroneous_edge_remover.Process();
}

template<class Graph>
bool RemoveHiddenEC(Graph& g,
                    const debruijn_graph::FlankingCoverage<Graph>& flanking_cov,
                    const debruijn_graph::debruijn_config::simplification::hidden_ec_remover& her_config,
                    const SimplifInfoContainer& info,
                    omnigraph::HandlerF<Graph> removal_handler) {
    if (her_config.enabled) {
        INFO("Removing hidden erroneous connections");
        return HiddenECRemover<Graph>(g, her_config.uniqueness_length, flanking_cov,
                               her_config.unreliability_threshold, info.detected_coverage_bound(),
                               her_config.relative_threshold, removal_handler).Run();
    }
    return false;
}

}
}
