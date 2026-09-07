function test_active_pair_vertex_certificate()
%TEST_ACTIVE_PAIR_VERTEX_CERTIFICATE Intersection logic and proof replay.
setup_intlab;
% All entries are normalized fence LENGTHS. Test above, below, and mixed
% disjointness, including different vertices meeting the individual pairs
% without any vertex meeting their common intersection.
pairLo = [1,1.5]; pairHi = [2,2.5];
assert(active_pair_vertex_exclusion(infsup([3,3,3,3,pairLo],[4,4,4,4,pairHi])));
assert(active_pair_vertex_exclusion(infsup([0,0,0,0,pairLo],[0.9,0.9,0.9,0.9,pairHi])));
items = infsup([1.1,2.1,3,3,pairLo],[1.4,2.4,4,4,pairHi]);
[proved,J] = active_pair_vertex_exclusion(items);
assert(proved && inf(J)==1.5 && sup(J)==2);
items(1) = infsup(1.1,1.5); % touching J is NOT a strict exclusion
assert(~active_pair_vertex_exclusion(items));
items(1) = infsup(NaN,NaN);
assert(~active_pair_vertex_exclusion(items));
items(1) = infsup(-inf,inf);
assert(~active_pair_vertex_exclusion(items));
items(5) = infsup(NaN,NaN);
assert(~active_pair_vertex_exclusion(items));
items(5) = infsup(0,1); items(6) = infsup(2,3);
assert(active_pair_vertex_exclusion(items)); % empty J needs no vertex bounds

% A genuine convex box: both pairs overlap, but bottom vertices lie below
% J and upper vertices above J. The old all-vertices-above rule cannot help.
point = [0.56,0.67,0.44,0.67];
lo = point-2e-4; hi = point+2e-4;
assert(~certainly_inadmissible(lo,hi,false));
assert(~p2v0_exclusion(lo,hi));
options = default_options();
options.theta = '0.9'; % avoid conflating this test with threshold exclusions
options.shortEdgeCertificate = false;
options.flatAreaCertificate = false;
options.geometricCertificates = false;
options.p2v0Certificate = false;
options.pairEqualityCertificate = false; % new reduction has its own flag
for form = {'natural','hybrid','centered'}
    evaluation = evaluate_fences(lo,hi,form{1},[5,6],[],false,[],[],true);
    assert(evaluation.activePairVertexExcluded);
    assert(~any(evaluation.centered)); % certificate found before derivatives
    assert(isfinite(inf(evaluation.activePairVertexIntersection)));
    options.form = form{1};
    category = categorize_box(lo,hi,options);
    assert(strcmp(category.status,'active_pair_vertex_incompatible'));
end
options.form = 'hybrid';
options.activePairVertexCertificate = false;
assert(strcmp(categorize_box(lo,hi,options).status,'survive'));
options.activePairVertexCertificate = true;
reference = [0.6417451566,0.7071006812,0.3582548434,0.7071006812];
referenceEvaluation = evaluate_fences(reference-1e-7,reference+1e-7, ...
    'hybrid',[5,6],[],false,[],[],true);
assert(~referenceEvaluation.activePairVertexExcluded);

filename = [tempname '.jsonl'];
options.outputFile = filename;
options.progressEvery = 0;
options.widthFloor = 1;
seed = struct('lo',lo,'hi',hi);
result = run_search(options,seed);
cleanup = onCleanup(@() delete(filename));
assert(result.stats.active_pair_vertex_incompatible==1 && result.stats.complete);
report = verify_seeded_certificate(filename,seed,[],true,0);
assert(report.valid);
original = fileread(filename);
expect_rejected(filename,seed,strrep(original,'"active_pair_vertex_cert":true', ...
                                             '"active_pair_vertex_cert":false'));
expect_rejected(filename,seed,strrep(original,'"active_pair_vertex_cert":true', ...
                                             '"active_pair_vertex_cert":2'));
expect_rejected(filename,seed,strrep(original,'"active_pair_vertex_cert":true,',''));
expect_rejected(filename,seed,strrep(original,'P0_P1_P2V0_sections4_5_remark19_v1','unknown'));
expect_rejected(filename,seed,strrep(original,'"schema":8','"schema":7'));
expect_rejected(filename,seed,strrep(original,'"item":0','"item":1'));

% Correct path and endpoints but a fabricated exclusion/witness must fail.
options.overwrite = true;
falseSeed = struct('lo',reference-1e-7,'hi',reference+1e-7,'path','','seed',1);
writer = open_certificate(filename,options,falseSeed);
leaf = falseSeed; leaf.status = 'active_pair_vertex_incompatible'; leaf.item = 0;
leaf.enclosure = intval(1);
write_certificate_leaf(writer,leaf);
close_certificate(writer);
expect_rejected(filename,falseSeed,fileread(filename));

% Historical schema 7 never silently acquires the stronger reduction.
options.activePairVertexCertificate = false;
base = run_search(options);
historical = strrep(fileread(filename),'"schema":8','"schema":7');
historical = strrep(historical,'"active_pair_vertex_cert":false,','');
historical = strrep(historical, ...
    '"active_pair_vertex_reduction":"P0_P1_P2V0_sections4_5_remark19_v1",','');
write_text(filename,historical);
old = verify_certificate(filename,[],0);
assert(old.valid);
refinementFile = [tempname '.jsonl'];
options.activePairVertexCertificate = true;
options.widthFloor = 0.5;
options.sourceCertificate = filename;
options.outputFile = refinementFile;
run_search(options,base.survivors);
cleanupRefinement = onCleanup(@() delete(refinementFile));
chain = verify_refinement_chain({filename,refinementFile},[],0);
assert(all([chain.valid]));
fprintf('=== ALL ACTIVE PAIR/VERTEX TESTS PASSED ===\n');
end

function expect_rejected(filename,seeds,contents)
write_text(filename,contents);
rejected = false;
try
    verify_seeded_certificate(filename,seeds,[],true,0);
catch exception
    rejected = startsWith(exception.identifier,'fence:');
end
assert(rejected,'Invalid active pair/vertex certificate was accepted.');
end

function write_text(filename,contents)
fid = fopen(filename,'w');
assert(fid>=0);
cleanup = onCleanup(@() fclose(fid));
fprintf(fid,'%s',contents);
end
