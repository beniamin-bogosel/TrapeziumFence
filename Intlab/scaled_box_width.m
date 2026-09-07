function width = scaled_box_width(lo, hi)
%SCALED_BOX_WIDTH Outward upper bound for the largest normalized width.

lowerEndpoints = intval(reshape(lo,1,4));
upperEndpoints = intval(reshape(hi,1,4));
widths = (upperEndpoints - lowerEndpoints) ./ intval([2,1,2,1]);
width = max(sup(widths));
end
