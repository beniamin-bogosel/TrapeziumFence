function angle = interval_atan2_nonnegative(y, x)
%INTERVAL_ATAN2_NONNEGATIVE Rigorous atan2(y,x) when y is nonnegative.
%
% INTLAB 13 has no real-interval atan2 overload.  In a fixed half-plane we
% use atan(y/x), preserving derivatives when x and y are gradients.  If an
% interval crosses the y-axis, the natural enclosure [0,pi] is safe.  A
% gradient call instead throws so that evaluate_fences can fall back to the
% natural enclosure for that item.

yRange = value_range(y);
xRange = value_range(x);
piInterval = cached_pi();

if any(isnan(inf(yRange))) || inf(yRange) < 0
    angle = infsup(NaN, NaN);
elseif inf(xRange) > 0
    angle = atan(y ./ x);
elseif sup(xRange) < 0
    angle = piInterval - atan(y ./ (-x));
elseif inf(xRange) == 0 && sup(xRange) == 0 && inf(yRange) > 0
    angle = piInterval ./ 2;
elseif isa(x, 'gradient') || isa(y, 'gradient')
    error('fence:indeterminateAngleDerivative', ...
          'atan2 derivative enclosure crosses the y-axis');
else
    % This includes the indeterminate (0,0) limit.  Every geometric angle in
    % the upper half-plane lies in this interval.
    angle = infsup(0, sup(piInterval));
end
end

function value = cached_pi()
persistent piInterval
if isempty(piInterval)
    piInterval = intval('pi');
end
value = piInterval;
end

function range = value_range(value)
if isa(value, 'gradient')
    range = value.x;
else
    range = value;
end
end
