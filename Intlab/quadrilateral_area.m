function area = quadrilateral_area(x)
%QUADRILATERAL_AREA Area of D=(0,0), C=(1,0), B, A.
%
% x may be a four-component INTLAB interval or gradient vector in the order
% (b1,b2,a1,a2).  The formula is the shoelace formula simplified completely.

b1 = x(1); b2 = x(2); a1 = x(3); a2 = x(4);
area = (b2 + b1 .* a2 - a1 .* b2) ./ 2;
end
