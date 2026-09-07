function [theta, epsilon, epsilonSquared] = short_edge_parameters(thetaText, epsilonText)
%SHORT_EDGE_PARAMETERS Validate and cache the analytic short-edge threshold.
%
% The strengthened proof requires epsilon^2 <= t*(3*t-pi)/4, t=theta^2,
% with pi/3 < t < pi/2. Retain the earlier pi/3 angle-dichotomy test as a
% fallback for historical certificates and other parameter choices.

persistent savedThetaText savedEpsilonText savedTheta savedEpsilon savedSquared

thetaText = char(thetaText);
epsilonText = char(epsilonText);
if ~isempty(savedThetaText) && strcmp(savedThetaText, thetaText) && ...
   strcmp(savedEpsilonText, epsilonText)
    theta = savedTheta;
    epsilon = savedEpsilon;
    epsilonSquared = savedSquared;
    return
end

decimalPattern = '^\+?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$';
if isempty(regexp(thetaText, decimalPattern, 'once')) || ...
   isempty(regexp(epsilonText, decimalPattern, 'once'))
    error('fence:badShortEdgeParameters', ...
          'theta and shortEdgeEpsilon must be positive decimal literals.');
end

theta = intval(thetaText);
epsilon = intval(epsilonText);
epsilonSquared = epsilon.^2;
if ~finite_positive(theta) || ~finite_positive(epsilon)
    error('fence:badShortEdgeParameters', ...
          'theta and shortEdgeEpsilon must be positive and finite.');
end

piUpper = intval('3.142');
requiredUpper = piUpper ./ 3 + 4 .* epsilonSquared ./ theta.^2;
t = theta.^2;
piValue = intval('pi');
newLimit = t .* (3.*t-piValue) ./ 4;
newProof = inf(3.*t-piValue)>0 && sup(t)<inf(piValue./2) && ...
           sup(epsilonSquared)<=inf(newLimit);
oldProof = sup(requiredUpper)<inf(t);
if ~(newProof || oldProof)
    error('fence:unsafeShortEdgeEpsilon', ...
          ['shortEdgeEpsilon=%s is not certified safe for theta=%s by ' ...
           'either of the analytic short-side bounds.'], ...
          epsilonText, thetaText);
end

savedThetaText = thetaText;
savedEpsilonText = epsilonText;
savedTheta = theta;
savedEpsilon = epsilon;
savedSquared = epsilonSquared;
end

function yes = finite_positive(value)
yes = all(isfinite(inf(value))) && all(isfinite(sup(value))) && inf(value) > 0;
end
