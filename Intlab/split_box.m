function [leftChild, rightChild, didSplit] = split_box(box)
%SPLIT_BOX Bisect the widest normalized coordinate with no coverage gap.
%
% A single binary64 split point is assigned to BOTH touching endpoints.
% Consequently [lo,split] union [split,hi] equals the parent as a statement
% about exact real numbers represented by the stored binary64 values.  No
% independently rounded center-radius reconstruction is used.

spans = [2, 1, 2, 1];
[~, coordinate] = max((box.hi - box.lo) ./ spans);
split = box.lo(coordinate) + ...
        (box.hi(coordinate) - box.lo(coordinate)) ./ 2;

leftChild = box;
rightChild = box;
if ~(box.lo(coordinate) < split && split < box.hi(coordinate))
    didSplit = false;  % no representable interior binary64 number
    return
end

leftChild.hi(coordinate) = split;
rightChild.lo(coordinate) = split;
leftChild.path = [box.path '0'];
rightChild.path = [box.path '1'];
didSplit = true;

assert(leftChild.lo(coordinate) == box.lo(coordinate));
assert(leftChild.hi(coordinate) == rightChild.lo(coordinate));
assert(rightChild.hi(coordinate) == box.hi(coordinate));
end
