function evaluation = demo_reference_trapezium()
%DEMO_REFERENCE_TRAPEZIUM Evaluate the explicit competitor from the paper.

setup_intlab;
coordinates = [0.6417451566, 0.7071006812, ...
               0.3582548434, 0.7071006812];
evaluation = evaluate_fences(coordinates, coordinates, 'natural');

fprintf('Reference trapezium, coordinates (b1,b2,a1,a2):\n');
disp(coordinates)
for k = 1:6
    fprintf('  %-12s [%.17g, %.17g]\n', evaluation.labels{k}, ...
            inf(evaluation.items(k)), sup(evaluation.items(k)));
end
fprintf('  minimum      [%.17g, %.17g]\n', ...
        inf(evaluation.minimum), sup(evaluation.minimum));
theta = intval('1.0496');
fprintf('  certified minimum > 1.0496: %s\n', ...
        string(inf(evaluation.minimum) > sup(theta)));
end
