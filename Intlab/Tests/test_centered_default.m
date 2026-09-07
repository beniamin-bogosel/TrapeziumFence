function test_centered_default()
%TEST_CENTERED_DEFAULT Production default, fine-cell exclusion, and replay.
setup_intlab;
options = default_options();
assert(strcmp(options.form,'centered'));

% A stored hybrid survivor at widthFloor=0.003125. Full centering should
% strengthen its bounds without changing its endpoints or subdivision.
seed.lo = [0.6640625,0.75390625,0.3515625,0.693359375];
seed.hi = [0.66796875,0.755859375,0.35546875,0.6953125];
hybridOptions = options;
hybridOptions.form = 'hybrid';
assert(strcmp(categorize_box(seed.lo,seed.hi,hybridOptions).status,'survive'));
category = categorize_box(seed.lo,seed.hi,options);
assert(~strcmp(category.status,'survive'));
assert(any(category.evaluation.centered));

filename = [tempname '.jsonl'];
cleanup = onCleanup(@() delete_if_present(filename));
options.outputFile = filename;
options.progressEvery = 0;
result = run_search(options,seed);
assert(result.stats.complete && result.stats.leaves==1 && result.stats.survivor==0);
report = verify_seeded_certificate(filename,seed,[],true,0);
assert(report.valid && strcmp(report.metadata.form,'centered'));

% The search must inherit centered and yield a full root cover. The new
% public driver is separately exercised by test_certification_process.
rootFile = [tempname '.jsonl'];
cleanupRoot = onCleanup(@() delete_if_present(rootFile));
options.outputFile = rootFile;
options.widthFloor = 0.5;
result = run_search(options);
assert(result.stats.complete && strcmp(result.options.form,'centered'));
report = verify_certificate(rootFile,[],0);
assert(report.valid && strcmp(report.metadata.form,'centered'));
fprintf('=== ALL CENTERED DEFAULT TESTS PASSED ===\n');
end

function delete_if_present(filename)
if isfile(filename), delete(filename); end
end
