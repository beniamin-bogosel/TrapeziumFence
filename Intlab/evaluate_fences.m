function evaluation = evaluate_fences(lo, hi, form, requestedItems, stopUpper, ...
                                      stopWhenPairDisjoint, knownNaturalArea, ...
                                      knownBox, stopWhenActivePairVertexIncompatible)
%EVALUATE_FENCES Rigorous enclosures of the six normalized fence candidates.
%
% evaluation.items contains, in order, the four vertex candidates followed by
% the pairs (DC,BA) and (CB,AD).  evaluation.minimum combines the finite items
% actually evaluated.  Its upper endpoint is therefore a rigorous upper bound
% for the minimum over all six candidates, even when early stopping means its
% lower endpoint need not enclose that full minimum.  Nonfinite candidates are
% omitted, which is also safe for the one-sided upper-bound certificate.
%
% Advanced search use:
%   evaluate_fences(lo,hi,form,items,stopUpper)
% evaluates items in the supplied order.  With a finite stopUpper it stops as
% soon as one enclosure proves upper <= stopUpper.  Pair disjointness is
% tested as soon as both pair items have been evaluated; it never forces an
% unconditional explicit-fence proof to do extra work.
% The optional ninth argument enables the active-pair/vertex condition:
% it lazily adds natural vertex bounds before any derivatives, and rechecks
% after centering. A single possible vertex intersection stops that test.

if nargin < 3 || isempty(form)
    form = 'natural';
end
if nargin < 4 || isempty(requestedItems)
    requestedItems = 1:6;
end
if nargin < 5
    stopUpper = [];
end
if nargin < 6
    stopWhenPairDisjoint = false;
end
if nargin < 7
    knownNaturalArea = [];
end
if nargin < 8
    knownBox = [];
end
if nargin<9, stopWhenActivePairVertexIncompatible = false; end
validateattributes(lo, {'double'}, {'real','finite','vector','numel',4});
validateattributes(hi, {'double'}, {'real','finite','vector','numel',4});
lo = reshape(lo, 1, 4);
hi = reshape(hi, 1, 4);
if any(lo > hi)
    error('fence:improperBox', 'Every lower endpoint must be <= its upper endpoint.');
end
requestedItems = unique(reshape(requestedItems, 1, []), 'stable');
if isempty(requestedItems) || any(requestedItems < 1) || ...
   any(requestedItems > 6) || any(requestedItems ~= floor(requestedItems))
    error('fence:badItems', 'Requested fence items must be integers from 1 through 6.');
end
if ~isempty(stopUpper) && ...
   ~(isnumeric(stopUpper) && isscalar(stopUpper) && isfinite(stopUpper))
    error('fence:badStopUpper', 'stopUpper must be an empty value or a finite scalar.');
end

if isempty(knownBox)
    X = infsup(lo, hi);
else
    X = knownBox;
end
natural = infsup(NaN(1,6), NaN(1,6));
naturalGeometry = build_geometry(X, knownNaturalArea);
evaluatedMask = false(1, 6);
naturalOrder = requestedItems;

naturalStopped = false;
for item = naturalOrder
    natural(item) = evaluate_item(naturalGeometry, item);
    evaluatedMask(item) = true;
    if ~isempty(stopUpper) && certified_by_upper(natural, evaluatedMask, stopUpper)
        naturalStopped = true;
        break
    end
    if stopWhenPairDisjoint && pair_disjoint(natural)
        naturalStopped = true;
        break
    end
end

activePairVertexExcluded = false;
activePairVertexIntersection = infsup(NaN,NaN);
if stopWhenActivePairVertexIncompatible && ~naturalStopped
    % High priority: try the common-pair/vertex condition before derivatives.
    % Reuse both pair intervals; compute vertices lazily, stopping as soon
    % as one might meet their intersection. No duplicate geometry is built.
    [activePairVertexExcluded,activePairVertexIntersection,natural,evaluatedMask] = ...
        check_active_pair_vertices(naturalGeometry,natural,evaluatedMask);
    naturalStopped = activePairVertexExcluded || ...
        (~isempty(stopUpper) && certified_by_upper(natural,evaluatedMask,stopUpper));
end

consistencyWarning = false(1, 6);
centeredMask = false(1, 6);
if strcmpi(form, 'natural')
    items = natural;
elseif strcmpi(form, 'centered') || strcmpi(form, 'hybrid')
    items = natural;
    if strcmpi(form, 'hybrid')
        % Profiling on the fine survivor region found that every useful
        % centered explicit-fence certificate came from the (DC,BA) pair.
        % Trying only that item retains the main dependency improvement while
        % avoiding a full automatic-gradient cascade on boxes that will split.
        itemOrder = intersect(5, requestedItems, 'stable');
    else
        itemOrder = requestedItems;
    end
    if naturalStopped
        itemOrder = [];
    elseif ~isempty(stopUpper)
        naturalUpper = sup(natural(itemOrder));
        [~, order] = sort(naturalUpper);
        itemOrder = itemOrder(order);
    end

    if ~isempty(itemOrder)
        midpoint = lo + (hi - lo) ./ 2;
        midpointInterval = intval(midpoint);
        displacement = X - midpointInterval;
        midpointGeometry = build_geometry(midpointInterval);
        if strcmpi(form, 'centered')
            G = gradientinit(X(:));
            gradientGeometry = build_geometry(G);
        end
    end
    for item = itemOrder
        try
            valueAtMidpoint = evaluate_item(midpointGeometry, item);
            if strcmpi(form, 'hybrid')
                derivative = pair5_interval_derivative(X);
            else
                gradientValue = evaluate_item(gradientGeometry, item);
                derivative = gradientValue.dx;
            end
            centered = valueAtMidpoint;
            for coordinate = 1:4
                % Form this difference in interval arithmetic.  Computing a
                % binary64 radius=(hi-lo)/2 could round down and make the
                % centered enclosure too small.
                centered = centered + derivative(coordinate) .* displacement(coordinate);
            end
            if finite_interval(centered)
                if finite_interval(natural(item))
                    [empty, overlap] = emptyintersect(natural(item), centered);
                    if empty
                        % Two mathematically valid enclosures must overlap.
                        consistencyWarning(item) = true;
                        error('fence:centeredInconsistency', ...
                              ['Natural and centered enclosures are disjoint ' ...
                               'for fence item %d.'], item);
                    else
                        items(item) = overlap;
                        centeredMask(item) = true;
                    end
                else
                    items(item) = centered;
                    centeredMask(item) = true;
                end
            end
        catch exception
            expectedFallback = any(strcmp(exception.identifier, ...
                {'fence:indeterminateAngleDerivative', ...
                 'fence:gammaDerivativeAtZero', ...
                 'fence:pair5DerivativeFallback'}));
            if ~expectedFallback
                rethrow(exception)
            end
            % Natural interval evaluation remains a rigorous fallback.
        end
        if ~isempty(stopUpper)
            if certified_by_upper(items, evaluatedMask, stopUpper)
                break
            end
        end
        if stopWhenPairDisjoint && pair_disjoint(items)
            break
        end
    end
else
    error('fence:badForm', ...
          'form must be ''natural'', ''hybrid'', or ''centered''.');
end

if stopWhenActivePairVertexIncompatible && ~naturalStopped && any(centeredMask)
    % Centering can shrink J enough to remove its last possible vertex.
    % Without a tightened enclosure, repeating the natural test adds no proof.
    [activePairVertexExcluded,activePairVertexIntersection,items,evaluatedMask] = ...
        check_active_pair_vertices(naturalGeometry,items,evaluatedMask);
end

itemLower = inf(items);
itemUpper = sup(items);
finite = ~isnan(itemLower) & ~isnan(itemUpper) & ...
         isfinite(itemLower) & isfinite(itemUpper);
if any(finite)
    minimumLower = min(itemLower(finite));
    itemUpper(~finite) = inf;
    [minimumUpper, activeItem] = min(itemUpper);
    minimumEnclosure = infsup(minimumLower, minimumUpper);
else
    activeItem = 0;
    minimumEnclosure = infsup(inf, inf);
end

evaluation.items = items;
evaluation.minimum = minimumEnclosure;
evaluation.activeItem = activeItem;
evaluation.labels = {'vertex D','vertex C','vertex B','vertex A', ...
                     'pair (DC,BA)','pair (CB,AD)'};
evaluation.form = lower(form);
evaluation.consistencyWarning = consistencyWarning;
evaluation.evaluated = evaluatedMask;
evaluation.centered = centeredMask;
evaluation.activePairVertexExcluded = activePairVertexExcluded;
evaluation.activePairVertexIntersection = activePairVertexIntersection;
end

function [proved,common,items,evaluated] = check_active_pair_vertices(geometry,items,evaluated)
[proved,common] = active_pair_vertex_exclusion(items);
if proved || ~finite_interval(common), return; end
for vertex = [2,1,4,3]
    if ~evaluated(vertex)
        items(vertex) = evaluate_item(geometry,vertex);
        evaluated(vertex) = true;
    end
    value = items(vertex);
    if ~finite_interval(value) || ...
       (sup(value)>=inf(common) && inf(value)<=sup(common))
        return % a possible active vertex, or an unknown interval: fail closed
    end
end
[proved,common] = active_pair_vertex_exclusion(items);
end

function geometry = build_geometry(x, knownArea)
if nargin < 2
    knownArea = [];
end
zero = x(1) .* 0;
one = zero + 1;
geometry.D = point(zero, zero);
geometry.C = point(one, zero);
geometry.B = point(x(1), x(2));
geometry.A = point(x(3), x(4));
geometry.DC = subtract(geometry.C, geometry.D);
geometry.CB = subtract(geometry.B, geometry.C);
geometry.BA = subtract(geometry.A, geometry.B);
geometry.AD = subtract(geometry.D, geometry.A);
if isempty(knownArea)
    geometry.area = quadrilateral_area(x);
else
    geometry.area = knownArea;
end
end

function fence = evaluate_item(geometry, item)
C = geometry.C;
B = geometry.B;
A = geometry.A;

switch item
    case 1
        % At D the second side is the unit vector DC=(1,0).
        fence = vertex_from_cross_dot(abs(A.y), A.x);
    case 2
        % At C the first side is CD=(-1,0).
        fence = vertex_from_cross_dot(abs(B.y), C.x - B.x);
    case 3
        fence = vertex_direction_fence(negate(geometry.CB), geometry.BA);
    case 4
        fence = vertex_direction_fence(negate(geometry.BA), geometry.AD);
    case 5
        fence = pair_fence(geometry.DC, geometry.BA, geometry.area, ...
                           true, B.y, A.y, 5);
    case 6
        fence = pair_fence(geometry.CB, geometry.AD, geometry.area, ...
                           false, B.y, A.y, 6);
    otherwise
        error('fence:badItem', 'Fence item must be between 1 and 6.');
end
end

function fence = vertex_direction_fence(firstDirection, secondDirection)
fence = vertex_from_cross_dot(abs(cross2(firstDirection, secondDirection)), ...
                              dot2(firstDirection, secondDirection));
end

function fence = vertex_from_cross_dot(crossMagnitude, dotProduct)
angle = interval_atan2_nonnegative(crossMagnitude, dotProduct);
fence = sqrt(angle);
end

function fence = pair_fence(line1Direction, line2Direction, area, ...
                            line1HasUnitLength, b2, a2, pairNumber)
crossDirections = cross2(line1Direction, line2Direction);
dotDirections = dot2(line1Direction, line2Direction);
gamma = interval_atan2_nonnegative(abs(crossDirections), -dotDirections);

% The apex formula is clearer and sharper away from parallel supporting
% lines.  The second formula is algebraically equal and remains regular when
% the lines approach parallelism.  For the acute line angle delta,
% delta>0.08 is equivalent to |cross|>|dot|*tan(0.08).  Testing that inequality
% avoids evaluating a second interval atan2; failure merely selects the safe
% formula and cannot invalidate a proof.
if certainly_separated_from_parallel(crossDirections, dotDirections)
    triangleSum = apex_triangle_sum(crossDirections, area, b2, a2, pairNumber);
    fenceSquared = gamma .* triangleSum ./ area;
else
    ratio = gamma_over_sin_interval(gamma);
    if line1HasUnitLength
        line1Length = area .* 0 + 1;
    else
        line1Length = sqrt(dot2(line1Direction, line1Direction));
    end
    line2Length = sqrt(dot2(line2Direction, line2Direction));
    distanceProductNumerator = parallel_distance_numerator(b2, a2, area, pairNumber);
    distanceProducts = distanceProductNumerator ./ (line1Length .* line2Length);
    fenceSquared = ratio .* distanceProducts ...
                   ./ (2 .* area);
end
fence = sqrt(fenceSquared);
end

function triangleSum = apex_triangle_sum(crossDirections, area, b2, a2, pairNumber)
% The line intersection parameter and the two triangle areas also simplify
% exactly in normalized coordinates.  Keeping this formula beside the
% parallel-safe identity makes the two conditioning branches easy to audit.
twiceArea = 2 .* area;
if pairNumber == 5                 % apex = D + parameter*DC
    parameter = (twiceArea - b2) ./ crossDirections;
    triangleSum = (abs(b2 .* (1 - parameter)) + abs(a2 .* parameter)) ./ 2;
else                               % apex = C + parameter*CB
    parameter = a2 ./ crossDirections;
    triangleSum = (abs(b2 .* parameter) + ...
                   abs((parameter - 1) .* (a2 - twiceArea))) ./ 2;
end
end

function numerator = parallel_distance_numerator(b2, a2, area, pairNumber)
% In normalized coordinates the four point-to-line cross products collapse
% to b2, a2, b2-2*area, and a2-2*area.  This exact identity avoids four
% generic point/line calculations in the commonly used parallel-safe form.
twiceArea = 2 .* area;
b2Offset = abs(b2 - twiceArea);
a2Offset = abs(a2 - twiceArea);
if pairNumber == 5                 % lines DC and BA
    numerator = a2Offset .* abs(b2) + b2Offset .* abs(a2);
else                               % lines CB and AD
    numerator = abs(a2) .* abs(b2) + b2Offset .* a2Offset;
end
end

function yes = certainly_separated_from_parallel(crossDirections, dotDirections)
persistent tangentThreshold
if isempty(tangentThreshold)
    tangentThreshold = tan(intval('0.08'));
end
crossRange = value_range(abs(crossDirections));
dotRange = value_range(abs(dotDirections));
yes = inf(crossRange) > sup(dotRange .* tangentThreshold);
end

function P = point(x, y)
P.x = x; P.y = y;
end

function R = subtract(P, Q)
R = point(P.x - Q.x, P.y - Q.y);
end

function R = negate(P)
R = point(-P.x, -P.y);
end

function value = dot2(P, Q)
value = P.x .* Q.x + P.y .* Q.y;
end

function value = cross2(P, Q)
value = P.x .* Q.y - P.y .* Q.x;
end

function range = value_range(value)
if isa(value, 'gradient')
    range = value.x;
else
    range = value;
end
end

function yes = finite_interval(value)
yes = ~any(isnan(inf(value))) && ~any(isnan(sup(value))) && ...
      all(isfinite(inf(value))) && all(isfinite(sup(value)));
end

function yes = pair_disjoint(items)
pair1 = items(5);
pair2 = items(6);
yes = finite_interval(pair1) && finite_interval(pair2) && ...
      (sup(pair1) < inf(pair2) || sup(pair2) < inf(pair1));
end

function yes = certified_by_upper(items, evaluatedMask, stopUpper)
lower = inf(items);
upper = sup(items);
finiteMask = evaluatedMask & ~isnan(lower) & ~isnan(upper) & ...
             isfinite(lower) & isfinite(upper);
yes = any(upper(finiteMask) <= stopUpper);
end
