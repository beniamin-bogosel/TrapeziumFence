function [discard, reason, sideSquared, x] = certainly_inadmissible(lo, hi, useHalfDomain)
%CERTAINLY_INADMISSIBLE Prove a normalized box contains no admissible shape.

if nargin < 3
    useHalfDomain = false;
end
sideSquared = [];
x = [];

% These two tests need no interval arithmetic: the upper endpoint is already
% the exact binary64 upper endpoint stored for the box.
if hi(2) < 0
    discard = true;
    reason = 'b2 is certainly negative';
    return
end
if hi(4) < 0
    discard = true;
    reason = 'a2 is certainly negative';
    return
end

x = infsup(reshape(lo,1,4), reshape(hi,1,4));
b1 = x(1); b2 = x(2); a1 = x(3); a2 = x(4);

CB = [b1 - 1, b2];
BA = [a1 - b1, a2 - b2];
AD = [-a1, -a2];
sideSquared = [dot2(CB,CB), dot2(BA,BA), a1.^2 + a2.^2];

if sup(cross2(CB, BA)) < 0
    discard = true;
    reason = 'turn at B is certainly clockwise';
    return
end
if sup(cross2(BA, AD)) < 0
    discard = true;
    reason = 'turn at A is certainly clockwise';
    return
end

if inf(sideSquared(1)) > 1
    discard = true;
    reason = '|CB| is certainly greater than 1';
    return
end
if inf(sideSquared(2)) > 1
    discard = true;
    reason = '|BA| is certainly greater than 1';
    return
end
if inf(sideSquared(3)) > 1
    discard = true;
    reason = '|AD| is certainly greater than 1';
    return
end

if useHalfDomain && sup(b1 + a1) < 1
    discard = true;
    reason = 'box is certainly outside b1+a1 >= 1 symmetry half';
    return
end

discard = false;
reason = '';
end

function value = dot2(P, Q)
value = P(1).*Q(1) + P(2).*Q(2);
end

function value = cross2(P, Q)
value = P(1).*Q(2) - P(2).*Q(1);
end
