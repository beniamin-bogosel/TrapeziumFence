function [certified, edge, lengthEnclosure] = ...
    short_edge_certifies_nonoptimal(sideSquared, thetaText, epsilonText, ...
                                    epsilonSquaredLower)
%SHORT_EDGE_CERTIFIES_NONOPTIMAL Apply the analytic short-side lemma.
%
% sideSquared contains rigorous enclosures for |CB|^2, |BA|^2, |AD|^2.
% The box is excluded only when one fixed edge has upper endpoint at most the
% certified epsilon throughout the entire box.

if nargin < 4 || isempty(epsilonSquaredLower)
    [~, ~, epsilonSquared] = short_edge_parameters(thetaText, epsilonText);
    epsilonSquaredLower = inf(epsilonSquared);
end
[smallestUpper, edge] = min(sup(sideSquared));
certified = smallestUpper <= epsilonSquaredLower;

if certified
    lengthEnclosure = sqrt(sideSquared(edge));
else
    edge = 0;
    lengthEnclosure = infsup(NaN, NaN);
end
end
