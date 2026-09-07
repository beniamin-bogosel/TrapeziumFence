function [proved, item, residual] = p2v0_exclusion(lo,hi,requestedPair,knownBox,knownArea)
%P2V0_EXCLUSION Prove that every vertex fence is strictly inactive.
% Uses a rational upper bound U for one squared opposite-pair value and
% proves all four interior angles > U. No pair-equality hypothesis is used.
% Lemma 12 excludes a single active pair; Sections 4/5 exclude two active
% pairs with no active vertex; Remark 19 covers parallelograms.
% item is the bounding pair (5 or 6). residual is a strictly negative upper
% bound on tan(U)*dot-cross, not an enclosure of a fence length.

if nargin<3 || isempty(requestedPair), requestedPair = [5,6]; end
if any(~ismember(requestedPair,[5,6]))
    error('fence:badP2V0Item','P2V0 pair items must be 5 or 6.');
end
if nargin<4 || isempty(knownBox), knownBox = infsup(lo,hi); end
if nargin<5 || isempty(knownArea), knownArea = quadrilateral_area(knownBox); end
proved = false; item = 0; residual = infsup(NaN,NaN);
S = 2.*knownArea;
if ~finite_interval(S) || inf(S)<=0, return; end
b1 = knownBox(1); b2 = knownBox(2); a1 = knownBox(3); a2 = knownBox(4);
p = b1.*a2-a1.*b2;
q = (b1-1).*a2+(1-a1).*b2;
P = [b1-a1, a1.*(b1-1)+a2.*b2];
C = [a2-b2, a2+b2-S];
N = [a2.*p+b2.*q, a2.*b2+p.*q];
indices = requestedPair-4;
valid = isfinite(inf(P(indices))) & isfinite(sup(P(indices))) & ...
        isfinite(inf(N(indices))) & isfinite(sup(N(indices))) & ...
        inf(P(indices))>0 & inf(N(indices))>=0;
indices = indices(valid);
if isempty(indices), return; end
P = P(indices); C = C(indices); N = N(indices);
P2 = P.^2; C2 = C.^2;
upper = N.*(15.*P2+4.*C2) ./ (S.*P.*(15.*P2+9.*C2));
bounds = sup(upper);
bounds(~isfinite(inf(upper)) | ~isfinite(bounds)) = inf;
[U,chosen] = min(bounds);
persistent halfPiLower
if isempty(halfPiLower), halfPiLower = inf(intval('pi')./2); end
% Four angles cannot all exceed pi/2. Avoid tan across its pole.
if ~isfinite(U) || U<0 || U>=halfPiLower, return; end
k = tan(intval(U)); % U is an exact binary64 upper bound, rounded outward here
linear = k.*[a1,1-b1]-[a2,b2];
if ~finite_interval(linear) || any(sup(linear)>=0), return; end

% E_A and E_B are separately convex in all coordinates: quadratic in
% their own vertex with coefficient k>=0, affine in the other vertex.
% Their maxima therefore occur among the 16 corners, even when a corner
% itself is inadmissible. General fence functions cannot be corner-tested.
persistent mask
if isempty(mask)
    mask = false(16,4);
    for coordinate=1:4
        mask(:,coordinate) = logical(bitget((0:15).',coordinate));
    end
end
points = repmat(reshape(lo,1,4),16,1);
upperPoints = repmat(reshape(hi,1,4),16,1);
points(mask) = upperPoints(mask);
points = intval(points);
b1 = points(:,1); b2 = points(:,2); a1 = points(:,3); a2 = points(:,4);
p = b1.*a2-a1.*b2;
q = (b1-1).*a2+(1-a1).*b2;
dotB = (b1-1).*(b1-a1)+b2.*(b2-a2);
dotA = a1.*(a1-b1)+a2.*(a2-b2);
values = k.*[dotB,dotA]-[q,p];
if ~finite_interval(values), return; end
bound = max([sup(linear),max(sup(values),[],1,'includenan')]);
if isfinite(bound) && bound<0
    proved = true;
    item = indices(chosen)+4;
    residual = intval(bound);
end
end

function yes = finite_interval(value)
yes = all(isfinite(inf(value)),'all') && all(isfinite(sup(value)),'all');
end
