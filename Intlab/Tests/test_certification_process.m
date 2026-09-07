function test_certification_process()
%TEST_CERTIFICATION_PROCESS Cheap end-to-end, restart, and rejection tests.
directory = tempname;
mkdir(directory);
cleanup = onCleanup(@() rmdir(directory,'s'));
startingDirectory = pwd;

% This deliberately coarse custom schedule must never impersonate the paper
% computation. It exercises the very same driver with tiny complete covers.
first = certification_process(directory,'compute',[1,0.5]);
assert(first.computationComplete && ~first.verified && ~first.paperSchedule);
assert(isequal([first.stages.widthFloor],[1,0.5]));
assert(~any([first.stages.reused]));
assert(all(isfinite([first.stages.computationSeconds])));
assert(strcmp(pwd,startingDirectory));
finalFile = fullfile(directory,first.files{end});
original = fileread(finalFile);
assert(isfile(fullfile(directory,'certification_summary.mat')));
assert(~isfile(fullfile(directory,'verification_reports.mat')));

second = certification_process(directory,'all',[1,0.5]);
assert(second.verified && all([second.stages.reused]));
assert(isequal([first.stages.computationSeconds],[second.stages.computationSeconds]));
assert(strcmp(original,fileread(finalFile))); % immutable certificates
saved = load(fullfile(directory,'verification_reports.mat'));
assert(all([saved.reports.valid]) && all([saved.reports.claimsRechecked]));
third = certification_process(directory,'verify',[1,0.5]);
assert(third.verified && strcmp(original,fileread(finalFile)));

% Wrong settings or partial final files are rejected, never silently reused
% or overwritten. Restore only our own temporary test file after each check.
write_text(finalFile,strrep(original,'"form":"centered"','"form":"hybrid"'));
expect_failure(@() certification_process(directory,'compute',[1,0.5]), ...
               'fence:wrongStageSettings');
write_text(finalFile,original);
newlines = strfind(original,newline);
truncated = original(1:newlines(end-1));
write_text(finalFile,truncated);
expect_failure(@() certification_process(directory,'compute',[1,0.5]), ...
               'fence:coverageGap');
assert(strcmp(truncated,fileread(finalFile)));
write_text(finalFile,original);
assert(strcmp(pwd,startingDirectory));

% An interrupted .partial file is retained recoverably, not accepted as a
% complete stage. The restarted stage gets fresh output and full replay.
interrupted = fullfile(directory,'interrupted');
mkdir(interrupted);
partial = fullfile(interrupted,'intlab_centered_custom_stage01.jsonl.partial');
write_text(partial,'deliberately incomplete test output');
restarted = certification_process(interrupted,'all',1);
assert(restarted.verified && ~restarted.stages.reused && ~isfile(partial));
archived = dir(fullfile(interrupted,'Archive','Interrupted','*.partial.*'));
assert(isscalar(archived));
assert(strcmp(fileread(fullfile(archived.folder,archived.name)), ...
              'deliberately incomplete test output'));
expect_failure(@() certification_process(interrupted,'verify',[1,0.5]), ...
               'fence:missingStage');
assert(strcmp(pwd,startingDirectory));
fprintf('=== ALL CERTIFICATION PROCESS TESTS PASSED ===\n');
end

function expect_failure(operation,identifier)
failed = false;
try
    operation();
catch exception
    failed = strcmp(exception.identifier,identifier);
    if ~failed, rethrow(exception); end
end
assert(failed,'Expected rejection did not occur.');
end

function write_text(filename,text)
fid = fopen(filename,'w');
assert(fid>=0);
cleanup = onCleanup(@() fclose(fid));
fprintf(fid,'%s',text);
end
