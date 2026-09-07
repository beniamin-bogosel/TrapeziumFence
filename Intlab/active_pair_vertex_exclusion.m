function [proved, common] = active_pair_vertex_exclusion(items)
%ACTIVE_PAIR_VERTEX_EXCLUSION Necessary common value of active fences.
% items(1:4) are vertex FENCE LENGTH intervals, items(5:6) are opposite
% pair FENCE LENGTH intervals. Do not mix lengths with vertex angles.
% After the independent P0/P1/P2V0 reductions, a maximizer must have both
% pairs and at least one vertex active. Hence J=I5 intersect I6 must meet
% at least one vertex interval. Disjointness above OR below J excludes.
% This is an analytic non-optimality test, not a threshold certificate.

assert(numel(inf(items))==6,'Expected six fence-length intervals.');
proved = false;
common = infsup(NaN,NaN);
lower = inf(items); upper = sup(items);
finite = isfinite(lower) & isfinite(upper);
if ~all(finite(5:6)), return; end
left = max(lower(5:6));
right = min(upper(5:6));
if left>right
    proved = true; % already no possible common active-pair value
    return
end
common = infsup(left,right); % endpoint selection needs no rounding arithmetic
if ~all(finite(1:4)), return; end
% Closed intervals: touching even one vertex is inconclusive.
proved = all(upper(1:4)<left | lower(1:4)>right);
end
