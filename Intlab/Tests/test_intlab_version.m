function test_intlab_version()
%TEST_INTLAB_VERSION Regression and certification-logic tests.

setup_intlab;
fprintf('=== MATLAB/INTLAB fence tests ===\n');
assert(strcmp(default_options().form, 'centered'));
assert(default_options().shortEdgeCertificate);
assert(strcmp(default_options().shortEdgeEpsilon,'0.212'));
assert(default_options().geometricCertificates);
assert(default_options().hybridWidthTrigger == 0.03125);

reference = [0.6417451566, 0.7071006812, ...
             0.3582548434, 0.7071006812];
evaluation = evaluate_fences(reference, reference, 'natural');
assert(inf(evaluation.minimum) > sup(intval('1.0496')));
expectedItems = [1.0496858157365985, 1.0496858157365985, ...
                 1.4281989853767524, 1.4281989853767524, ...
                 1.0496858140905033, 1.0496858159922250];
assert(all(abs(mid(evaluation.items) - expectedItems) < 5e-14));

obtuse = [0.5, 0.4, 0.4, 0.4];
obtuseEvaluation = evaluate_fences(obtuse, obtuse, 'natural');
assert(abs(mid(obtuseEvaluation.items(6)) - 1.3097413128223512) < 1e-10);

smallLo = [0.6417450566,0.7071005812,0.3582547434,0.7071005812];
smallHi = [0.6417452566,0.7071007812,0.3582549434,0.7071007812];
smallNatural = evaluate_fences(smallLo, smallHi, 'natural');
assert(~emptyintersect(smallNatural.items(5), smallNatural.items(6)));
smallCentered = evaluate_fences(smallLo, smallHi, 'centered');
smallHybrid = evaluate_fences(smallLo, smallHi, 'hybrid');
pointEvaluation = evaluate_fences(reference, reference, 'natural');
for item = 1:6
    assert(in(pointEvaluation.items(item), smallCentered.items(item)));
end
assert(in(pointEvaluation.items(5), smallHybrid.items(5)));
assert(smallHybrid.centered(5)); % stable derivative also works through gamma=0
assert(~smallHybrid.centered(6));

ratio = gamma_over_sin_interval(infsup(0, 0.1));
pointGamma = intval(0.1); % exact binary64 endpoint of the interval above
pointRatio = pointGamma ./ sin(pointGamma);
assert(in(1, ratio) && in(pointRatio, ratio));

box = root_box();
[left, right, didSplit] = split_box(box);
assert(didSplit && left.hi(1) == right.lo(1));
assert(isequal(left.lo, box.lo) && isequal(right.hi, box.hi));

% Non-dyadic endpoints exercise the rounding-sensitive child-cover case.
sensitive.lo = [-0.8223235157420244, -0.24, 0.28, -0.41];
sensitive.hi = [-0.2741078385806724, -0.16, 0.32, -0.39];
sensitive.path = '';
sensitive.seed = 1;
[left, right, didSplit] = split_box(sensitive);
assert(didSplit && left.hi(1) == right.lo(1));
assert(left.lo(1) == sensitive.lo(1) && right.hi(1) == sensitive.hi(1));

options = default_options();
options.form = 'natural';
options.activePairVertexCertificate = false; % specifically test the earlier cascade
options.p2v0Certificate = false; % exercise the pre-P2V0 cascade below
[bad, ~] = certainly_inadmissible([1.8,0.9,-0.9,0.9], ...
                                  [2.0,1.0,-0.8,1.0], false);
assert(bad);

% Hybrid remains an explicit opt-in and avoids derivatives on coarse boxes.
coarseOptions = default_options();
coarseOptions.form = 'hybrid';
coarseCategory = categorize_box(root_box().lo, root_box().hi, coarseOptions);
assert(~isempty(coarseCategory.evaluation));
assert(strcmp(coarseCategory.evaluation.form, 'natural'));

category = categorize_box(reference, reference, options);
assert(strcmp(category.status, 'pair_incompatible'));
options.pairEqualityCertificate = false;
category = categorize_box(reference, reference, options);
assert(strcmp(category.status, 'survive')); % the competitor is above theta

flatLo = [0.79,0.09,0.19,0.09];
flatHi = [0.81,0.11,0.21,0.11];
options = default_options();
category = categorize_box(flatLo, flatHi, options);
assert(strcmp(category.status, 'flat_area'));

% The high-priority analytic short-edge rule precedes area and fence work.
shortPoint = [0.8, 0.5, 0.05, 0.1];  % |AD| = sqrt(0.0125) < 0.122
options = default_options();
category = categorize_box(shortPoint, shortPoint, options);
assert(strcmp(category.status, 'short_edge'));
assert(category.item == 3);
assert(sup(category.enclosure) < inf(intval(options.shortEdgeEpsilon)));

% A box is not eliminated merely because it contains some short-edge points:
% one fixed edge must be short throughout the complete box.
[discard, ~, sideSquared] = certainly_inadmissible( ...
    [0.8,0.5,0.05,0.1], [0.8,0.5,0.13,0.1], false);
assert(~discard);
[shortCertified, ~, ~] = short_edge_certifies_nonoptimal( ...
    sideSquared, options.theta, '0.122');
assert(~shortCertified);

unsafeRejected = false;
try
    short_edge_parameters('1.0496', '0.213');
catch exception
    unsafeRejected = strcmp(exception.identifier, ...
                            'fence:unsafeShortEdgeEpsilon');
end
assert(unsafeRejected);

interiorLo = [0.64,0.44,0.34,0.44];
interiorHi = [0.66,0.46,0.36,0.46];
options.theta = '1.04';
options.shortEdgeCertificate = false;
options.flatAreaCertificate = false;
options.pairEqualityCertificate = false;
options.geometricCertificates = false; % specifically test the original cascade
options.activePairVertexCertificate = false;
options.p2v0Certificate = false;
category = categorize_box(interiorLo, interiorHi, options);
assert(strcmp(category.status, 'certified'));
assert(category.item == 2);
assert(category.evaluation.evaluated(2));
assert(~any(category.evaluation.evaluated([5,6]))); % cheap vertex stopped the cascade

% A tiny smoke search also exercises path order and certificate serialization.
options.widthFloor = 1;
options.maxLeaves = 20;
options.progressEvery = 0;
options.keepLeaves = true;
temporaryCertificate = [tempname '.jsonl'];
removeTemporary = onCleanup(@() delete_if_present(temporaryCertificate));
options.outputFile = temporaryCertificate;
smoke = run_search(options);
assert(smoke.stats.leaves > 0);
verification = verify_certificate(temporaryCertificate);
assert(verification.valid);

overwriteRejected = false;
try
    run_search(options);
catch exception
    overwriteRejected = strcmp(exception.identifier, 'fence:certificateExists');
end
assert(overwriteRejected);

temporaryRefinement = [tempname '.jsonl'];
removeRefinement = onCleanup(@() delete_if_present(temporaryRefinement));
refineOptions = options;
refineOptions.widthFloor = 0.5;
refineOptions.maxLeaves = inf;
refineOptions.keepLeaves = false;
refineOptions.outputFile = temporaryRefinement;
refineOptions.sourceCertificate = temporaryCertificate;
refinement = run_search(refineOptions, verification.survivors);
assert(refinement.stats.complete);
chain = verify_refinement_chain( ...
    {temporaryCertificate, temporaryRefinement});
assert(all([chain.valid]));
clear removeRefinement
clear removeTemporary

fprintf('=== ALL MATLAB/INTLAB TESTS PASSED ===\n');
end

function delete_if_present(filename)
if isfile(filename)
    delete(filename);
end
end
