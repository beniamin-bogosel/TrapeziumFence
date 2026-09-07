function ratio = gamma_over_sin_interval(gamma)
%GAMMA_OVER_SIN_INTERVAL Enclose gamma/sin(gamma), including gamma=0.
%
% The continuous value at zero is 1.  On [0,pi), gamma/sin(gamma) is
% increasing, so endpoint evaluation is both rigorous and much sharper than
% naively dividing intervals that contain zero.

if isa(gamma, 'gradient')
    range = gamma.x;
    piInterval = cached_pi();
    if inf(range) <= 0 || sup(range) >= inf(piInterval)
        % The direct gradient quotient contains 0/0 at the origin.  The caller
        % catches this condition and retains the rigorous natural enclosure.
        error('fence:gammaDerivativeAtZero', ...
              'Use the natural gamma/sin(gamma) enclosure on this box.');
    end
    ratio = gamma ./ sin(gamma);
    return
end

piInterval = cached_pi();
lower = inf(gamma);
upper = sup(gamma);
if isnan(lower) || isnan(upper) || upper < 0 || upper >= inf(piInterval)
    ratio = infsup(NaN, NaN);
    return
end
lower = max(lower, 0);

if lower == 0
    lowerValue = 1;
else
    t = intval(lower);
    lowerValue = inf(t ./ sin(t));
end
if upper == 0
    upperValue = 1;
else
    t = intval(upper);
    upperValue = sup(t ./ sin(t));
end
ratio = infsup(lowerValue, upperValue);
end

function value = cached_pi()
persistent piInterval
if isempty(piInterval)
    piInterval = intval('pi');
end
value = piInterval;
end
