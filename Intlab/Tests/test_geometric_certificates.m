function test_geometric_certificates()
%TEST_GEOMETRIC_CERTIFICATES Every new rule, replay, and fail-closed metadata.
setup_intlab;
options = default_options();
theta = options.theta;
reference = [0.6417451566,0.7071006812,0.3582548434,0.7071006812];
assert(~geometric_exclusion(reference,reference,theta,false));
assert(~geometric_exclusion(reference-1e-7,reference+1e-7,theta,false));
short_edge_parameters(theta,'0.122'); % historical setting remains supported
short_edge_parameters(theta,'0.212');

% One convex positive-area point for each stable item ID. Some points prove
% several rules; requestedItem ensures every implementation is exercised.
square = [1,1,0,1];
thin = [1,0.1,0,0.1];
points = [square; square; square; square; thin; ...
          0.525,0.8,0.475,0.8; thin; ...
          0.9,0.15,0.5,0.1; 0.5,0.1,0.1,0.15; ...
          0.8,0.9,0.15,0.3; 0.85,0.3,0.2,0.9; ...
          0.5,0.85,0.03,0.06; 0.7,0.2,0.3,0.2; ...
          square; square; 1,0.5,0,0.5];
seeds = repmat(struct('lo',[],'hi',[],'path','','seed',0),16,1);
filename = [tempname '.jsonl'];
for item = 1:16
    point = points(item,:);
    [discard,~] = certainly_inadmissible(point,point,false);
    assert(~discard,'Fixture %d is inadmissible.',item);
    lo = point-1e-6;
    hi = point+1e-6;
    [proved,checked,residual] = geometric_exclusion(lo,hi,theta,true,item);
    assert(proved && checked==item,'Rule %d failed its positive fixture.',item);
    assert(sup(residual)<=0);
    seeds(item).lo = lo;
    seeds(item).hi = hi;
    seeds(item).seed = item;
    exact = evaluate_fences(point,point,'natural');
    if item<16
        assert(sup(exact.minimum)<inf(intval(theta)));
    else
        assert(sup(exact.items(5))<inf(exact.items(6)));
    end
end
assert(~geometric_exclusion(points(16,:),points(16,:),theta,false,16));
% Singular pair denominators must not produce a rational certificate.
assert(~geometric_exclusion(zeros(1,4),zeros(1,4),theta,true,[14,15,16]));
% Range extrema must propagate NaN corners, not silently omit them during
% min/max reductions. These finite inputs deliberately cause overflow.
assert(~geometric_exclusion(zeros(1,4),1e200*ones(1,4),theta,false,[1,10,11]));
% The angle-dichotomy rules require their proved threshold range.
assert(~geometric_exclusion(thin,thin,'0.5',true,5:13));

writer = open_certificate(filename,options,seeds);
cleanup = onCleanup(@() delete(filename));
for item = 16:-1:1
    leaf = seeds(item);
    leaf.item = item;
    leaf.status = 'geometric_low';
    if item==16, leaf.status = 'rational_pair_incompatible'; end
    [~,~,leaf.enclosure] = geometric_exclusion(leaf.lo,leaf.hi,theta,true,item);
    write_certificate_leaf(writer,leaf);
end
close_certificate(writer);
report = verify_seeded_certificate(filename,seeds);
assert(report.valid && report.counts.geometric_low==15 && ...
       report.counts.rational_pair_incompatible==1);
original = fileread(filename);
expect_rejected(filename,seeds,strrep(original,'"geometric_cert":true', ...
                                              '"geometric_cert":false'));
expect_rejected(filename,seeds,strrep(original,'"pair_eq_cert":true', ...
                                              '"pair_eq_cert":false'));
expect_rejected(filename,seeds,strrep(original,'"item":16','"item":1'));
expect_rejected(filename,seeds,strrep(original,'"theta":"1.0496"', ...
                                              '"theta":"1.025"'));
expect_rejected(filename,seeds,strrep(original,'"schema":8','"schema":5'));

% Old schemas must not silently enable new predicates. A root survivor is a
% complete historical cover; refine that schema-5 file with current options.
options.overwrite = true;
options.geometricCertificates = false;
options.p2v0Certificate = false;
options.form = 'natural'; % schema 4 predates the hybrid form
options.shortEdgeEpsilon = '0.122';
options.widthFloor = 1;
options.progressEvery = 0;
options.outputFile = filename;
run_search(options);
current = fileread(filename);
for schema = [4,5]
    historical = strrep(current,'"schema":8',sprintf('"schema":%d',schema));
    historical = strrep(historical,'"p2v0_cert":false,','');
    historical = strrep(historical,'"geometric_cert":false,','');
    if schema==4
        historical = strrep(historical,'"short_edge_cert":true,','');
        historical = strrep(historical,'"short_edge_epsilon":"0.122",','');
        historical = strrep(historical,'"hybrid_width_trigger":0.03125,','');
        historical = strrep(historical,'"source_certificate":"",','');
    end
    write_text(filename,historical);
    oldReport = verify_certificate(filename);
    assert(oldReport.valid);
end
refinementFile = [tempname '.jsonl'];
cleanupRefinement = onCleanup(@() delete(refinementFile));
options = default_options();
options.widthFloor = 0.5;
options.progressEvery = 0;
options.sourceCertificate = filename;
options.outputFile = refinementFile;
run_search(options,oldReport.survivors);
chain = verify_refinement_chain({filename,refinementFile});
assert(all([chain.valid]));
fprintf('=== ALL GEOMETRIC CERTIFICATE TESTS PASSED ===\n');
end

function expect_rejected(filename,seeds,contents)
write_text(filename,contents);
rejected = false;
try
    verify_seeded_certificate(filename,seeds);
catch exception
    rejected = startsWith(exception.identifier,'fence:');
end
assert(rejected,'Tampered certificate was accepted.');
end

function write_text(filename,contents)
fid = fopen(filename,'w');
assert(fid>=0);
cleanup = onCleanup(@() fclose(fid));
fprintf(fid,'%s',contents);
end
