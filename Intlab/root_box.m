function box = root_box()
%ROOT_BOX Normalized domain in coordinate order (b1,b2,a1,a2).
%
% Public paper notation is
%   D=(0,0), C=(1,0), B=(b1,b2), A=(a1,a2).

box.lo = [0, 0, -1, 0];
box.hi = [2, 1,  1, 1];
box.path = '';
box.seed = 1;
end
