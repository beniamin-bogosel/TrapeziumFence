function derivative = pair5_interval_derivative(x)
%PAIR5_INTERVAL_DERIVATIVE Derivative enclosure for the (DC,BA) fence.
%
% This is a deliberately small four-variable interval jet specialized to the
% pair that matters near the conjectured trapezoid. It avoids INTLAB gradient
% object dispatch while keeping the parallel-safe formula visible in one place.
% Each jet has fields v (value interval) and d (four interval derivatives).

variable = cell(1,4);
for k = 1:4
    variable{k} = jet_variable(x(k), k);
end
b1 = variable{1}; b2 = variable{2};
a1 = variable{3}; a2 = variable{4};

area = jet_divide(jet_subtract( ...
    jet_add(b2, jet_multiply(b1, a2)), jet_multiply(a1, b2)), ...
    jet_constant(2));
dx = jet_subtract(a1, b1);
dy = jet_subtract(a2, b2);

gamma = jet_atan2_nonnegative(jet_absolute(dy), jet_negate(dx));
ratio = jet_gamma_over_sin(gamma);
lineLength = jet_sqrt(jet_add(jet_multiply(dx, dx), ...
                              jet_multiply(dy, dy)));

twiceArea = jet_multiply(jet_constant(2), area);
a2Offset = jet_absolute(jet_subtract(a2, twiceArea));
b2Offset = jet_absolute(jet_subtract(b2, twiceArea));
distanceNumerator = jet_add( ...
    jet_multiply(a2Offset, jet_absolute(b2)), ...
    jet_multiply(b2Offset, jet_absolute(a2)));

denominator = jet_multiply(jet_multiply(jet_constant(2), area), lineLength);
fenceSquared = jet_divide(jet_multiply(ratio, distanceNumerator), denominator);
fence = jet_sqrt(fenceSquared);
if ~finite_interval(fence.v) || ~finite_interval(fence.d)
    fallback('The specialized pair derivative became nonfinite.');
end
derivative = fence.d;
end

function a = jet_variable(value, coordinate)
a.v = value;
a.d = intval(zeros(1,4));
a.d(coordinate) = intval(1);
end

function a = jet_constant(value)
a.v = intval(value);
a.d = intval(zeros(1,4));
end

function c = jet_add(a, b)
c.v = a.v + b.v;
c.d = a.d + b.d;
end

function c = jet_subtract(a, b)
c.v = a.v - b.v;
c.d = a.d - b.d;
end

function c = jet_negate(a)
c.v = -a.v;
c.d = -a.d;
end

function c = jet_multiply(a, b)
c.v = a.v .* b.v;
c.d = a.d .* b.v + a.v .* b.d;
end

function c = jet_divide(a, b)
if ~finite_interval(b.v) || in(0, b.v)
    fallback('A pair-fence denominator contains zero.');
end
c.v = a.v ./ b.v;
c.d = (a.d .* b.v - a.v .* b.d) ./ (b.v.^2);
end

function c = jet_sqrt(a)
if ~finite_interval(a.v) || inf(a.v) <= 0
    fallback('A pair-fence square root is not bounded away from zero.');
end
c.v = sqrt(a.v);
c.d = a.d ./ (2 .* c.v);
end

function c = jet_absolute(a)
c.v = abs(a.v);
if inf(a.v) >= 0
    c.d = a.d;
elseif sup(a.v) <= 0
    c.d = -a.d;
else
    magnitude = max(abs(inf(a.d)), abs(sup(a.d)));
    c.d = infsup(-magnitude, magnitude);
end
end

function c = jet_atan2_nonnegative(y, x)
c.v = interval_atan2_nonnegative(y.v, x.v);
denominator = x.v.^2 + y.v.^2;
if ~finite_interval(c.v) || ~finite_interval(denominator) || ...
   inf(denominator) <= 0
    fallback('The pair angle contains the indeterminate origin.');
end
c.d = (x.v .* y.d - y.v .* x.d) ./ denominator;
end

function c = jet_gamma_over_sin(gamma)
c.v = gamma_over_sin_interval(gamma.v);
range = gamma.v;
piInterval = intval('pi');
if ~finite_interval(c.v) || inf(range) < 0 || ...
   sup(range) >= inf(piInterval)
    fallback('The pair angle is not contained in [0,pi).');
end

if inf(range) > 0
    sine = sin(range);
    derivativeRange = (sine - range .* cos(range)) ./ (sine.^2);
elseif sup(range) <= inf(piInterval ./ 2)
    % For 0 <= t <= pi/2,
    %   sin(t)-t*cos(t) = integral_0^t s*sin(s) ds <= t^3/3,
    %   sin(t) >= 2t/pi,
    % hence 0 <= (t/sin(t))' <= pi^2*t/12.
    upperAngle = intval(sup(range));
    derivativeUpper = sup(piInterval.^2 .* upperAngle ./ 12);
    derivativeRange = infsup(0, derivativeUpper);
else
    fallback('The stable gamma/sin derivative bound is too wide.');
end
c.d = derivativeRange .* gamma.d;
end

function fallback(message)
error('fence:pair5DerivativeFallback', '%s', message);
end

function yes = finite_interval(value)
yes = ~any(isnan(inf(value)), 'all') && ~any(isnan(sup(value)), 'all') && ...
      all(isfinite(inf(value)), 'all') && all(isfinite(sup(value)), 'all');
end
