function [proved, item, residual, reason] = geometric_exclusion( ...
    lo, hi, thetaText, usePairEquality, requestedItem, knownBox)
%GEOMETRIC_EXCLUSION Algebraic certificates on the admissible subset of a box.
%
% A successful item 1..15 proves Phi^2 <= theta^2; item 16 only proves
% opposite-pair incompatibility. The residual is an upper-bound witness,
% <=0 (strictly <0 for 16), not necessarily the full polynomial range.
% requestedItem lets the verifier recheck precisely the recorded claim.
% Proofs and stable item numbering are documented in Theory/.
%
% No geometric constraints are imposed on the saved box: inadmissible points
% are irrelevant, but every admissible point remains covered by the proof.

if nargin < 5 || isempty(requestedItem)
    requestedItem = [9,8,1,5,6,7,2,3,4,12,13,14,15,10,11,16];
end
if nargin < 6 || isempty(knownBox)
    x = infsup(reshape(lo,1,4),reshape(hi,1,4));
else
    x = knownBox;
end
if any(~ismember(requestedItem,1:16))
    error('fence:badGeometricItem','Geometric items must be integers 1..16.');
end
constants = cached_constants(thetaText);
t = constants.t;
k = constants.tangent;
proved = false;
item = 0;
residual = infsup(NaN,NaN);
reason = '';
b1 = x(1); b2 = x(2); a1 = x(3); a2 = x(4);
corners = [];
area = [];
sides = [];
p = [];
q = [];
P = [];
C = [];
N = [];
linearVertices = [];
shortResiduals = [];
chordResiduals = [];
pairAngleResiduals = [];
pairResiduals = [];
names = {'vertical chord (sharp corners)','chord normal to CB', ...
    'chord normal to BA','chord normal to AD', ...
    'area-dependent short CB','area-dependent short BA','area-dependent short AD', ...
    'vertex D polynomial','vertex C polynomial','vertex B quadratic', ...
    'vertex A quadratic','pair-5 angle sum','pair-6 angle sum', ...
    'rational pair-5 threshold','rational pair-6 threshold', ...
    'rational opposite-pair incompatibility'};

for candidate = reshape(requestedItem,1,[])
    if candidate == 16 && ~usePairEquality
        continue
    end
    if candidate>=5 && candidate<=13 && ~constants.angleRangeValid
        continue
    end
    if (candidate<=7 || candidate>=12) && isempty(area)
        [corners, area] = corner_geometry(lo,hi);
    end
    if candidate>=2 && candidate<=7 && isempty(sides)
        sides = [(b1-1).^2+b2.^2, (a1-b1).^2+(a2-b2).^2, a1.^2+a2.^2];
    end
    if ((candidate>=2 && candidate<=4) || candidate>=14) && isempty(p)
        p = b1.*a2-a1.*b2;
        q = (b1-1).*a2+(1-a1).*b2;
    end
    if candidate >= 12 && isempty(P)
        P = [b1-a1, a1.*(b1-1)+a2.*b2];
        C = [a2-b2, a2+b2-2.*area];
    end
    if candidate>=14 && isempty(N)
        N = [a2.*p+b2.*q, a2.*b2+p.*q];
    end

    switch candidate
        case 1
            values = [corners.a2.^2-t.*corners.area, ...
                      corners.b2.^2-t.*corners.area];
            bound = max(sup(values),[],'all','includenan');
            witness = intval(bound);
        case {2,3,4}
            if isempty(chordResiduals)
                heights = [b2,q; p,q; a2,p];
                scaledSides = t.*area.*sides(:);
                values = heights.^2-[scaledSides,scaledSides];
                chordResiduals = intval(max(sup(values),[],2,'includenan'));
            end
            side = candidate-1;
            witness = chordResiduals(side);
        case {5,6,7}
            if isempty(shortResiduals)
                shortResiduals = sides-constants.eta.*area;
            end
            witness = shortResiduals(candidate-4);
        case {8,9}
            if isempty(linearVertices)
                linearVertices = [a2,b2]-k.*[a1,1-b1];
            end
            witness = linearVertices(candidate-7);
        case {10,11}
            witness = -sharp_vertex_lower(lo,hi,k,candidate);
        case {12,13}
            if isempty(pairAngleResiduals)
                pairAngleResiduals = constants.pairTangent.*P-abs(C);
            end
            pair = candidate-11;
            witness = pairAngleResiduals(pair);
        case {14,15}
            pair = candidate-13;
            if ~positive(P(pair)) || ~positive(area) || ~nonnegative(N(pair))
                continue
            end
            if isempty(pairResiduals)
                P2 = P.^2;
                C2 = C.^2;
                pairResiduals = N.*(15.*P2+4.*C2) - ...
                                t.*(2.*area).*P.*(15.*P2+9.*C2);
            end
            witness = pairResiduals(pair);
        case 16
            if ~positive(P) || ~positive(area) || ~nonnegative(N)
                continue
            end
            % Compare S*F5^2 and S*F6^2: the common area factor cancels.
            lower = 3.*N.*P ./ (3.*P.^2+C.^2);
            upper = N.*(15.*P.^2+4.*C.^2) ./ (P.*(15.*P.^2+9.*C.^2));
            differences = [upper(1)-lower(2), upper(2)-lower(1)];
            [~, direction] = min(sup(differences));
            witness = differences(direction);
    end
    if finite_interval(witness) && ...
       ((candidate < 16 && sup(witness)<=0) || ...
        (candidate == 16 && sup(witness)<0))
        proved = true;
        item = candidate;
        residual = witness;
        reason = names{candidate};
        return
    end
end
end

function c = cached_constants(thetaText)
persistent savedText savedConstants
if isempty(savedText) || ~strcmp(savedText,thetaText)
    theta = intval(char(thetaText));
    c.t = theta.^2;
    piValue = intval('pi');
    c.eta = 3.*c.t-piValue;
    c.angleRangeValid = inf(c.eta)>0 && sup(c.t)<inf(piValue./2);
    c.tangent = infsup(NaN,NaN);
    c.pairTangent = infsup(NaN,NaN);
    if c.angleRangeValid
        c.tangent = tan(c.t);
        c.pairTangent = tan(piValue-2.*c.t);
    end
    savedText = char(thetaText);
    savedConstants = c;
end
c = savedConstants;
end

function [corners, area] = corner_geometry(lo,hi)
persistent upperMask
if isempty(upperMask)
    upperMask = false(16,4);
    for coordinate = 1:4
        upperMask(:,coordinate) = logical(bitget((0:15).',coordinate));
    end
end
points = repmat(reshape(lo,1,4),16,1);
upper = repmat(reshape(hi,1,4),16,1);
points(upperMask) = upper(upperMask);
points = intval(points);
corners.b2 = points(:,2);
corners.a2 = points(:,4);
corners.area = (points(:,2)+points(:,1).*points(:,4)-points(:,3).*points(:,2))./2;
% Area is multi-affine, so these endpoint bounds cover the complete box.
area = infsup(min(inf(corners.area),[],1,'includenan'), ...
              max(sup(corners.area),[],1,'includenan'));
end

function lower = sharp_vertex_lower(lo,hi,k,item)
if item == 11
    own = [3,4]; other = [1,2];
else
    own = [1,2]; other = [3,4];
end
u = intval([lo(other(1));hi(other(1));lo(other(1));hi(other(1))]);
v = intval([lo(other(2));lo(other(2));hi(other(2));hi(other(2))]);
if item == 11
    linear = [-k.*u+v, -k.*v-u];
    constant = intval(zeros(4,1));
else
    linear = [-k.*(1+u)-v, -k.*v-1+u];
    constant = k.*u+v;
end
stationary = -linear./(2.*k);
lowerBox = lo(own);
upperBox = hi(own);
location = infsup(max(lowerBox,min(upperBox,inf(stationary))), ...
                  max(lowerBox,min(upperBox,sup(stationary))));
values = k.*location.^2+linear.*location;
% Minimize in the own vertex; the other vertex enters affinely, so its
% four corners suffice. Every operation on the minimizer is outward rounded.
lower = intval(min(inf(values(:,1)+values(:,2)+constant),[],1,'includenan'));
end

function yes = finite_interval(value)
yes = all(isfinite(inf(value)),'all') && all(isfinite(sup(value)),'all');
end

function yes = positive(value)
yes = finite_interval(value) && all(inf(value)>0,'all');
end

function yes = nonnegative(value)
yes = finite_interval(value) && all(inf(value)>=0,'all');
end
