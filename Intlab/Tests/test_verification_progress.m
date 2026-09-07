function test_verification_progress()
%TEST_VERIFICATION_PROGRESS Progress is diagnostic; proof failures still fail.
filename = [tempname '.jsonl'];
write_text(filename,'');
cleanup = onCleanup(@() delete(filename));
texts = {'', 'meta', sprintf('meta\n'), sprintf('meta\nleaf'), ...
         sprintf('meta\nleaf\n'), sprintf('meta\r\nleaf\r\n')};
expected = [0,0,0,1,1,1];
for k=1:numel(texts)
    write_text(filename,texts{k});
    assert(certificate_record_count(filename)==expected(k));
end
% Cross the 1 MiB counting boundary; retain bounded scan memory.
write_text(filename,[sprintf('meta\n'), ...
                    repmat([repmat('x',1,1024),newline],1,1100)]);
assert(certificate_record_count(filename)==1100);

options = default_options();
options.outputFile = filename;
options.overwrite = true;
options.widthFloor = 0.5;
options.progressEvery = 0;
result = run_search(options);
assert(result.stats.complete && result.stats.leaves==16);
original = fileread(filename);
output = evalc('report = verify_certificate(filename,[],3);');
assert(report.valid && report.elapsedSeconds>=0);
assert(contains(output,'checked=0/16'));
assert(contains(output,'checked=3/16 (18.75%)'));
assert(contains(output,'checked=16/16 (100.00%)'));
assert(contains(output,'elapsed=') && contains(output,'ETA(this file)='));
quiet = evalc('quietReport = verify_certificate(filename,[],0);');
assert(~contains(quiet,'verify: checked='));
assert(quietReport.valid && isequal(report.counts,quietReport.counts));
structural = evalc('coverage = verify_seeded_certificate(filename,root_box(),[],false,3);');
assert(contains(structural,'claims NOT rechecked'));
assert(coverage.valid && ~coverage.claimsRechecked);

% Missing final newline is permitted; missing final leaf must still fail.
write_text(filename,original(1:end-1));
output = evalc('report = verify_certificate(filename,[],3);');
assert(report.valid && contains(output,'checked=16/16'));
newlines = strfind(original,newline);
write_text(filename,original(1:newlines(end-1)));
rejected = false;
try
    evalc('verify_certificate(filename,[],3);');
catch exception
    rejected = strcmp(exception.identifier,'fence:coverageGap');
end
assert(rejected);

% All wrappers must propagate the interval and identify their chain stage.
options.widthFloor = 1;
base = run_search(options);
refinementFile = [tempname '.jsonl'];
options.widthFloor = 0.5;
options.sourceCertificate = filename;
options.outputFile = refinementFile;
run_search(options,base.survivors);
cleanupRefinement = onCleanup(@() delete(refinementFile));
output = evalc('reports = verify_refinement_chain({filename,refinementFile},[],3);');
assert(all([reports.valid]));
assert(contains(output,'Verifying stage 1/2'));
assert(contains(output,'Verifying stage 2/2'));
assert(contains(output,'checked=3/16'));
assert(contains(output,'Total verification-chain elapsed:'));
fprintf('=== ALL VERIFICATION PROGRESS TESTS PASSED ===\n');
end

function write_text(filename,contents)
fid = fopen(filename,'wb');
assert(fid>=0);
cleanup = onCleanup(@() fclose(fid));
fwrite(fid,contents,'char');
end
