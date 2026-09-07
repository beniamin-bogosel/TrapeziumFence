function category = categorize_box(lo, hi, options)
%CATEGORIZE_BOX Classify one box, without subdividing it.
%
% Status meanings:
%   discarded          certainly inadmissible (or symmetry-pruned)
%   short_edge         a whole non-base edge is <= the safe epsilon
%   flat_area           certified <= theta by the small-area estimate
%   p2v0_nonoptimal     all vertex fences strictly inactive (analytic exclusion)
%   active_pair_vertex_incompatible   no common active pair/vertex value
%   geometric_low       algebraic chord, short-side, angle, or pair bound
%   rational_pair_incompatible   conditional rational pair-equality exclusion
%   pair_incompatible   exact pair intervals are disjoint (conditional use)
%   certified           one explicit fence is certified <= theta
%   survive             none of the preceding tests succeeds

[discard, reason, sideSquared, x] = certainly_inadmissible(lo, hi, options.half);
if discard
    category = result('discarded', reason);
    return
end

% This is the first nontrivial certificate because it reuses the squared
% side intervals already formed by the admissibility test and is much cheaper
% than area, angle, pair, or gradient evaluation.
if options.shortEdgeCertificate
    [shortEdge, edge, lengthEnclosure] = short_edge_certifies_nonoptimal( ...
        sideSquared, options.theta, options.shortEdgeEpsilon, ...
        options.shortEdgeSquaredLimit);
    if shortEdge
        edgeNames = {'CB', 'BA', 'AD'};
        category = result('short_edge', ...
                          sprintf('|%s| <= %s', edgeNames{edge}, ...
                                  options.shortEdgeEpsilon));
        category.item = edge;
        category.enclosure = lengthEnclosure;
        return
    end
end

[theta, flatAreaLimit, polynomialVerticesValid] = cached_thresholds(options.theta);
area = quadrilateral_area(x);
if options.flatAreaCertificate
    % inf(theta^2/4) is no larger than the exact decimal threshold squared,
    % so this comparison is deliberately one-sided and rigorous.
    if finite_interval(area) && sup(area) <= inf(flatAreaLimit)
        category = result('flat_area', '2*sqrt(area) <= theta');
        category.enclosure = 2 .* sqrt(area);
        return
    end
end

if options.p2v0Certificate && max((hi-lo)./[2,1,2,1])<=1/32
    % High priority on fine boxes, before exact fences or derivatives.
    % Coarse-box tests found no benefit. This ordinary binary64 width gate
    % only controls effort; it never certifies or removes any points.
    [proved,item,residual] = p2v0_exclusion(lo,hi,[],x,area);
    if proved
        category = result('p2v0_nonoptimal','all vertex fences strictly inactive');
        category.item = item;
        category.enclosure = residual;
        return
    end
end

if options.geometricCertificates
    % The two linear angle tests and the sharp height bound are cheap and
    % useful enough to precede exact pair evaluation. Less frequent bounds
    % run only if that evaluation is inconclusive (below).
    [proved, item, residual, reason] = geometric_exclusion( ...
        lo, hi, options.theta, options.pairEqualityCertificate, [9,8,1], x);
    if proved
        category = geometric_result(item,residual,reason);
        return
    end
end

% Retain the original six-fence cascade when polynomial vertices are disabled
% or additional centered vertex bounds are requested.
evaluationOrder = [2, 4, 5, 6, 1, 3];
if options.geometricCertificates && polynomialVerticesValid && ...
   ~strcmpi(options.form,'centered')
    % Vertex thresholds are tested algebraically: C/D above, A/B below if
    % needed. Only the active-pair/vertex test adds natural vertex lengths. Full
    % centered mode retains its independent derivative-based vertex bounds.
    evaluationOrder = [5,6];
end
evaluationForm = selected_evaluation_form(lo, hi, options);
evaluation = evaluate_fences(lo, hi, evaluationForm, evaluationOrder, inf(theta), ...
                             options.pairEqualityCertificate, area, x, ...
                             options.activePairVertexCertificate);
if evaluation.activePairVertexExcluded
    category = result('active_pair_vertex_incompatible', ...
                      'pair intersection meets no vertex fence interval');
    category.enclosure = evaluation.activePairVertexIntersection;
    category.evaluation = evaluation;
    return
end
if options.pairEqualityCertificate
    pair1 = evaluation.items(5);
    pair2 = evaluation.items(6);
    if finite_interval(pair1) && finite_interval(pair2) && ...
       (sup(pair1) < inf(pair2) || sup(pair2) < inf(pair1))
        category = result('pair_incompatible', ...
                          'opposite-pair intervals are disjoint');
        category.evaluation = evaluation;
        return
    end
end

if finite_interval(evaluation.minimum) && sup(evaluation.minimum) <= inf(theta)
    category = certified_result(evaluation);
    return
end

if options.geometricCertificates
    [proved, item, residual, reason] = geometric_exclusion( ...
        lo, hi, options.theta, options.pairEqualityCertificate, ...
        [5:7,2:4,12:15,10:11,16], x);
    if proved
        category = geometric_result(item,residual,reason);
        return
    end
end

category = result('survive', 'unresolved by the available interval tests');
category.item = evaluation.activeItem;
category.enclosure = evaluation.minimum;
category.evaluation = evaluation;
end

function category = geometric_result(item,residual,reason)
status = 'geometric_low';
if item==16
    status = 'rational_pair_incompatible';
end
category = result(status,reason);
category.item = item;
category.enclosure = residual; % polynomial residual, not a fence value
end

function form = selected_evaluation_form(lo, hi, options)
form = options.form;
if strcmpi(form, 'hybrid')
    % Centering item 5 pays for itself only on fine boxes. This binary64
    % comparison controls effort, not validity: selecting natural evaluation
    % merely declines an optional stronger enclosure.
    normalizedWidths = (hi - lo) ./ [2, 1, 2, 1];
    if max(normalizedWidths) > options.hybridWidthTrigger
        form = 'natural';
    end
end
end

function category = certified_result(evaluation)
category = result('certified', 'explicit fence <= theta');
category.item = evaluation.activeItem;
category.enclosure = evaluation.minimum;
category.evaluation = evaluation;
end

function category = result(status, reason)
category.status = status;
category.reason = reason;
category.item = 0;
category.enclosure = infsup(NaN, NaN);
category.evaluation = [];
end

function yes = finite_interval(value)
yes = ~any(isnan(inf(value))) && ~any(isnan(sup(value))) && ...
      all(isfinite(inf(value))) && all(isfinite(sup(value)));
end

function [theta, flatAreaLimit, polynomialVerticesValid] = cached_thresholds(decimalTheta)
% Parsing a decimal with intval and squaring it are surprisingly visible in
% a many-leaf profile.  Options are constant throughout a search, so retain
% the outward-rounded values until the decimal literal changes.
persistent savedDecimal savedTheta savedFlatAreaLimit savedVertexDomain
if isempty(savedDecimal) || ~strcmp(savedDecimal, decimalTheta)
    savedDecimal = decimalTheta;
    savedTheta = intval(decimalTheta);
    savedFlatAreaLimit = savedTheta.^2 ./ 4;
    t = savedTheta.^2;
    piValue = intval('pi');
    savedVertexDomain = inf(3.*t-piValue)>0 && sup(t)<inf(piValue./2);
end
theta = savedTheta;
flatAreaLimit = savedFlatAreaLimit;
polynomialVerticesValid = savedVertexDomain;
end
