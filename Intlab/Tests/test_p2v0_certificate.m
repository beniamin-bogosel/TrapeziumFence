function test_p2v0_certificate()
%TEST_P2V0_CERTIFICATE Strict inactivity, independent replay, and old schemas.
setup_intlab;
options = default_options();
assert(options.p2v0Certificate);
options.pairEqualityCertificate = false;
square = [1,1,0,1];
lo = square-1e-5; hi = square+1e-5;
for pair = [5,6]
    [proved,item,residual] = p2v0_exclusion(lo,hi,pair);
    assert(proved && item==pair && sup(residual)<0);
end
category = categorize_box(lo,hi,options);
assert(strcmp(category.status,'p2v0_nonoptimal'));
assert(isempty(category.evaluation)); % high priority; no six-fence evaluation
rectangle = [1,0.75,0,0.75];
exact = evaluate_fences(rectangle,rectangle,'natural');
assert(sup(exact.items(5))<inf(exact.items(6)));
assert(p2v0_exclusion(rectangle,rectangle)); % unequal pairs still excluded

% This box contains the reference optimum; do not exclude it just because
% its decimal midpoint is not itself an exact maximizer.
reference = [0.6417451566,0.7071006812,0.3582548434,0.7071006812];
assert(~p2v0_exclusion(reference-1e-7,reference+1e-7));
activeVertex = [0.55,0.7,0.45,0.7];
exact = evaluate_fences(activeVertex,activeVertex,'natural');
assert(sup(exact.items(1))<min(inf(exact.items(5:6))));
assert(~p2v0_exclusion(activeVertex,activeVertex));
assert(~p2v0_exclusion(zeros(1,4),zeros(1,4)));
assert(~p2v0_exclusion(zeros(1,4),ones(1,4)*1e200));
assert(~p2v0_exclusion(root_box().lo,root_box().hi));

filename = [tempname '.jsonl'];
options.outputFile = filename;
options.progressEvery = 0;
options.widthFloor = 1;
seed = struct('lo',lo,'hi',hi);
result = run_search(options,seed);
cleanup = onCleanup(@() delete(filename));
assert(result.stats.complete && result.stats.p2v0_nonoptimal==1);
report = verify_seeded_certificate(filename,seed);
assert(report.valid);
summary = analyze_certificate(filename);
assert(summary.counts.p2v0_nonoptimal==1);
original = fileread(filename);
expect_rejected(filename,seed,strrep(original,'"p2v0_cert":true', ...
                                             '"p2v0_cert":false'));
expect_rejected(filename,seed,strrep(original,'"p2v0_cert":true,',''));
expect_rejected(filename,seed,strrep(original,'"p2v0_cert":true', ...
                                             '"p2v0_cert":2'));
expect_rejected(filename,seed,strrep(original,'"schema":8','"schema":6'));
expect_rejected(filename,seed,regexprep(original,'"item":[56]','"item":1'));
% Correct box/path but false mathematical claim: test independent replay,
% not merely malformed metadata or a path mismatch.
options.overwrite = true;
falseSeed = struct('lo',activeVertex,'hi',activeVertex,'path','','seed',1);
writer = open_certificate(filename,options,falseSeed);
leaf = falseSeed; leaf.status = 'p2v0_nonoptimal'; leaf.item = 5;
leaf.enclosure = intval(-1); % an invented witness must not be trusted
write_certificate_leaf(writer,leaf);
close_certificate(writer);
expect_rejected(filename,falseSeed,fileread(filename));

% A genuine schema-6 root stage can be refined with the current schema.
options.p2v0Certificate = false;
run_search(options);
historical = strrep(fileread(filename),'"schema":8','"schema":6');
historical = strrep(historical,'"p2v0_cert":false,','');
write_text(filename,historical);
old = verify_certificate(filename);
assert(old.valid);
refinementFile = [tempname '.jsonl'];
options.p2v0Certificate = true;
options.widthFloor = 0.5;
options.outputFile = refinementFile;
options.sourceCertificate = filename;
run_search(options,old.survivors);
cleanupRefinement = onCleanup(@() delete(refinementFile));
chain = verify_refinement_chain({filename,refinementFile});
assert(all([chain.valid]));
% Complete, nontrivial root cover with the production-priority rule enabled.
options = default_options();
options.widthFloor = 0.125;
options.outputFile = filename;
options.overwrite = true;
options.progressEvery = 100;
full = run_search(options);
assert(full.stats.complete);
verified = verify_certificate(filename);
assert(verified.valid);
fprintf('=== ALL P2V0 CERTIFICATE TESTS PASSED ===\n');
end

function expect_rejected(filename,seeds,contents)
write_text(filename,contents);
rejected = false;
try
    verify_seeded_certificate(filename,seeds);
catch exception
    rejected = startsWith(exception.identifier,'fence:');
end
assert(rejected,'Invalid P2V0 certificate was accepted.');
end

function write_text(filename,contents)
fid = fopen(filename,'w');
assert(fid>=0);
cleanup = onCleanup(@() fclose(fid));
fprintf(fid,'%s',contents);
end
