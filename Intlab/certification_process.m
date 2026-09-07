function summary = certification_process(outputDirectory, mode, widthFloors, intlabRoot)
%CERTIFICATION_PROCESS Serial base search, six refinements, and full verification.
%
%   summary = certification_process;                 % run/resume and verify
%   summary = certification_process('my_run');       % separate output directory
%   summary = certification_process([], 'compute');  % defer mathematical replay
%   summary = certification_process([], 'verify');   % verify existing stages only
%
% Optional third argument: a decreasing width schedule, e.g. [1,0.5] for a
% short smoke test. Custom schedules have distinct filenames and are NOT the
% paper computation. Optional fourth argument: the INTLAB installation path.
%
% Completed stage files are never overwritten. Every reused cover is checked.
% An interrupted *.partial stage is archived and restarted from its seeds.
% Run only one process per output directory. Timings/reports are diagnostics;
% only successful full claim and coverage replay establishes verification.

codeDirectory = fileparts(mfilename('fullpath'));
addpath(codeDirectory);
if nargin<1 || isempty(outputDirectory), outputDirectory = codeDirectory; end
if nargin<2 || isempty(mode), mode = 'all'; end
paperFloors = 0.025 ./ 2.^(0:6);
if nargin<3 || isempty(widthFloors), widthFloors = paperFloors; end
defaults = default_options();
if nargin<4 || isempty(intlabRoot), intlabRoot = defaults.intlabRoot; end
mode = validatestring(mode,{'all','compute','verify'});
validateattributes(widthFloors,{'double'}, ...
    {'real','vector','finite','positive','nonempty'});
widthFloors = reshape(widthFloors,1,[]);
if any(diff(widthFloors)>=0)
    error('fence:badSchedule','Width floors must strictly decrease.');
end
outputDirectory = char(outputDirectory);
if ~isfolder(outputDirectory)
    if strcmp(mode,'verify')
        error('fence:missingOutputDirectory','No output directory: %s',outputDirectory);
    end
    [ok,message] = mkdir(outputDirectory);
    if ~ok, error('fence:outputDirectory','%s',message); end
end
[found,installation] = fileattrib(intlabRoot);
if ~found, error('fence:intlabNotFound','INTLAB directory not found: %s',intlabRoot); end
intlabRoot = installation.Name;
setup_intlab(intlabRoot);
% Record absolute paths before changing directory. Relative source filenames
% in the certificates then remain portable and match the historical chain.
previousDirectory = pwd;
restoreDirectory = onCleanup(@() cd(previousDirectory));
cd(outputDirectory);
outputDirectory = pwd;

paperSchedule = isequal(widthFloors,paperFloors);
if paperSchedule
    suffixes = {'0025','00125','000625','0003125','00015625','000078125','0000390625'};
    files = cellfun(@(s) ['intlab_centered_pair_vertex_w' s '.jsonl'], ...
                    suffixes,'UniformOutput',false);
    expectedMinutes = [187,130,56,49,56,76,104];
    verificationMinutes = 9851.8/60;
else
    files = arrayfun(@(k) sprintf('intlab_centered_custom_stage%02d.jsonl',k), ...
                    1:numel(widthFloors),'UniformOutput',false);
    expectedMinutes = NaN(size(widthFloors));
    verificationMinutes = NaN;
end
options = defaults;
options.form = 'centered';
options.intlabRoot = intlabRoot;
options.progressEvery = 100;
options.keepSurvivors = false; % recovered by the independent coverage check
summary.mode = mode;
summary.outputDirectory = outputDirectory;
summary.paperSchedule = paperSchedule;
summary.machine = machine_information();
summary.intlabRoot = char(intlabRoot);
summary.files = files;
summary.stages = struct([]);
summary.computationComplete = false;
summary.verified = false;
summary.verificationSeconds = NaN;
summary.started = char(datetime('now','Format','yyyy-MM-dd HH:mm:ss'));
started = tic;

fprintf('Serial centered INTLAB certification: %s\n',outputDirectory);
fprintf('Current machine: %s; %.1f GiB RAM; MATLAB %s; %s\n', ...
    summary.machine.cpu,summary.machine.memoryGiB,summary.machine.matlab,summary.machine.os);
fprintf('Reference: Intel i7-9750H 2.60 GHz, 6 cores/12 threads, 32 GB RAM, Linux Mint 22.1.\n');
fprintf('No parallel workers. Estimates use the reference machine, not a CPU-speed scaling model.\n');
if ~paperSchedule, fprintf('CUSTOM schedule: this is not the seven-stage paper computation.\n'); end

% Refuse incompatible old files before starting any expensive new stage.
for k=1:numel(files)
    if isfile(files{k})
        check_settings(read_metadata(files{k}),k,files,widthFloors,options);
    elseif strcmp(mode,'verify')
        error('fence:missingStage','Stage %d has not completed: %s',k,files{k});
    end
end
print_expected_completion(files,expectedMinutes,verificationMinutes,mode);
summary.elapsedSeconds = toc(started);
save('certification_summary.mat','summary');

if ~strcmp(mode,'verify')
    seeds = root_box();
    for k=1:numel(files)
        options.widthFloor = widthFloors(k);
        options.sourceCertificate = '';
        if k>1, options.sourceCertificate = files{k-1}; end
        reused = isfile(files{k});
        fprintf('\n=== Stage %d/%d: widthFloor %.12g ===\n',k,numel(files),widthFloors(k));
        if reused
            fprintf('Reusing completed file; checking coverage only: %s\n',files{k});
            report = verify_seeded_certificate(files{k},seeds,intlabRoot,false,100);
            computationSeconds = previous_timing(files{k},report.leaves);
        else
            partial = [files{k} '.partial'];
            archive_interrupted(partial);
            options.outputFile = partial;
            if isfinite(expectedMinutes(k))
                fprintf('Reference computation estimate: %d minutes (not a deadline).\n',expectedMinutes(k));
            end
            result = run_search(options,seeds);
            if ~result.stats.complete
                error('fence:incompleteStage','Stage %d did not finish; partial output retained.',k);
            end
            report = verify_seeded_certificate(partial,seeds,intlabRoot,false,100);
            check_settings(report.metadata,k,files,widthFloors,options);
            if isfile(files{k})
                error('fence:certificateExists','Refusing to replace %s.',files{k});
            end
            [ok,message] = movefile(partial,files{k});
            if ~ok, error('fence:promoteStage','%s',message); end
            computationSeconds = result.stats.elapsedSeconds;
            timing.stats = result.stats;
            timing.file = files{k};
            details = dir(files{k});
            timing.fileBytes = details.bytes;
            timing.machine = summary.machine;
            save([files{k}(1:end-6) '_stats.mat'],'timing');
            clear result timing
        end
        stage = stage_summary(report,files{k},reused,computationSeconds);
        if k==1
            summary.stages = stage;
        else
            summary.stages(k) = stage;
        end
        seeds = report.survivors;
        fprintf('Stage complete: %d leaves, %d survivors.\n',report.leaves,numel(seeds));
        print_hulls(summary.stages(k));
        summary.elapsedSeconds = toc(started);
        save('certification_summary.mat','summary');
        print_expected_completion(files,expectedMinutes,verificationMinutes,mode);
        if isempty(seeds) && k<numel(files)
            error('fence:noSurvivors', ...
                'No survivors remain; inspect this result before extending the reference schedule.');
        end
    end
    summary.computationComplete = true;
end

if ~strcmp(mode,'compute')
    % Never trust a cached success flag. Explicit verification replays every
    % mathematical claim and checks links/coverage across the entire chain.
    verificationStarted = tic;
    reports = verify_refinement_chain(files,intlabRoot,100);
    summary.verificationSeconds = toc(verificationStarted);
    for k=1:numel(files)
        if strcmp(mode,'verify')
            stage = stage_summary(reports(k),files{k},true, ...
                                  previous_timing(files{k},reports(k).leaves));
            if k==1
                summary.stages = stage;
            else
                summary.stages(k) = stage;
            end
        end
        summary.stages(k).verificationSeconds = reports(k).elapsedSeconds;
    end
    summary.verified = true;
    summary.computationComplete = true;
    save('verification_reports.mat','reports');
else
    fprintf('\nCOMPUTATION ONLY: covers checked, mathematical claim replay is still pending.\n');
end
summary.elapsedSeconds = toc(started);
summary.finished = char(datetime('now','Format','yyyy-MM-dd HH:mm:ss'));
save('certification_summary.mat','summary');
fprintf('\nCompleted %d stages; final survivors: %d; fully verified: %d.\n', ...
    numel(files),summary.stages(end).survivors,summary.verified);
print_hulls(summary.stages(end));
fprintf('Invocation elapsed: %.0f minutes. Summary: %s\n', ...
    summary.elapsedSeconds/60,fullfile(outputDirectory,'certification_summary.mat'));
end

function metadata = read_metadata(filename)
fid = fopen(filename,'r');
if fid<0, error('fence:certificateOpen','Cannot open %s.',filename); end
closeFile = onCleanup(@() fclose(fid));
line = fgetl(fid);
if ~ischar(line), error('fence:badCertificate','Empty certificate: %s',filename); end
metadata = jsondecode(line);
end

function check_settings(metadata,k,files,floors,options)
% Proof-flag matching prevents silently mixing a legacy experiment into the
% reproduction pipeline. The general verifier still supports old schemas.
names = {'schema','theta','form','half','width_floor','full_root', ...
    'source_certificate','short_edge_cert','short_edge_epsilon', ...
    'geometric_cert','p2v0_cert','active_pair_vertex_cert', ...
    'active_pair_vertex_reduction','pair_eq_cert','flat_area_cert'};
if ~all(isfield(metadata,names))
    error('fence:wrongStageSettings','Incomplete metadata in %s.',files{k});
end
expectedSource = '';
if k>1, expectedSource = files{k-1}; end
flags = {'short_edge_cert','geometric_cert','p2v0_cert', ...
         'active_pair_vertex_cert','pair_eq_cert','flat_area_cert'};
flagsOn = all(cellfun(@(f) isequal(metadata.(f),true) || ...
                          isequal(metadata.(f),1),flags));
if ~isequal(metadata.schema,8) || ~strcmp(metadata.theta,options.theta) || ...
   ~strcmp(metadata.form,'centered') || ~isequal(metadata.half,false) || ...
   ~isequal(metadata.width_floor,floors(k)) || ...
   ~isequal(metadata.full_root,k==1) || ...
   ~strcmp(metadata.source_certificate,expectedSource) || ~flagsOn || ...
   ~strcmp(metadata.short_edge_epsilon,options.shortEdgeEpsilon) || ...
   ~strcmp(metadata.active_pair_vertex_reduction,'P0_P1_P2V0_sections4_5_remark19_v1')
    error('fence:wrongStageSettings', ...
        'Settings/source mismatch in %s. Use a separate output directory.',files{k});
end
end

function stage = stage_summary(report,filename,reused,seconds)
stage.file = filename;
stage.widthFloor = report.metadata.width_floor;
stage.reused = reused;
stage.leaves = report.leaves;
stage.survivors = numel(report.survivors);
stage.counts = report.counts;
stage.hullLo = NaN(1,4);
stage.hullHi = NaN(1,4);
if stage.survivors>0
    stage.hullLo = min(vertcat(report.survivors.lo),[],1);
    stage.hullHi = max(vertcat(report.survivors.hi),[],1);
end
stage.computationSeconds = seconds; % NaN means no measured timing is available
stage.verificationSeconds = NaN;
end

function print_hulls(stage)
if stage.survivors==0, fprintf('No survivor hull.\n'); return; end
lo = stage.hullLo; hi = stage.hullHi;
fprintf('A: [%.17g, %.17g] x [%.17g, %.17g]\n',lo(3),hi(3),lo(4),hi(4));
fprintf('B: [%.17g, %.17g] x [%.17g, %.17g]\n',lo(1),hi(1),lo(2),hi(2));
end

function print_expected_completion(files,minutesPerStage,verificationMinutes,mode)
remaining = 0;
if ~strcmp(mode,'verify')
    remaining = sum(minutesPerStage(~cellfun(@isfile,files)));
end
if ~strcmp(mode,'compute'), remaining = remaining+verificationMinutes; end
if isfinite(remaining)
    finish = datetime('now')+minutes(remaining);
    finish.Format = 'yyyy-MM-dd HH:mm';
    fprintf('Reference-based remaining estimate: %.0f minutes; expected finish around %s (local time).\n', ...
        remaining,char(finish));
    fprintf('Approximate; excludes coverage/setup overhead and depends on machine load.\n');
else
    fprintf('No reference completion-time estimate for this custom schedule.\n');
end
end

function seconds = previous_timing(filename,leaves)
seconds = NaN;
sidecar = [filename(1:end-6) '_stats.mat'];
if ~isfile(sidecar), return; end
try
    saved = load(sidecar,'timing');
    timing = saved.timing;
    details = dir(filename);
    if strcmp(timing.file,filename) && timing.fileBytes==details.bytes && ...
       timing.stats.complete && timing.stats.leaves==leaves && ...
       isscalar(timing.stats.elapsedSeconds) && isfinite(timing.stats.elapsedSeconds) && ...
       timing.stats.elapsedSeconds>=0
        seconds = timing.stats.elapsedSeconds;
    end
catch
    % A missing/corrupt timing file must never affect proof acceptance.
    warning('fence:timingUnavailable','Ignoring unreadable timing sidecar %s.',sidecar);
end
end

function archive_interrupted(filename)
if ~isfile(filename), return; end
archive = fullfile('Archive','Interrupted');
if ~isfolder(archive), mkdir(archive); end
[~,uniqueName] = fileparts(tempname);
destination = fullfile(archive,[filename '.' uniqueName]);
[ok,message] = movefile(filename,destination);
if ~ok, error('fence:archivePartial','%s',message); end
fprintf('Interrupted stage retained at %s; restarting that stage from its seeds.\n',destination);
end

function machine = machine_information()
machine.cpu = getenv('PROCESSOR_IDENTIFIER');
if isempty(machine.cpu), machine.cpu = computer; end
machine.memoryGiB = NaN;
machine.matlab = version;
machine.os = computer;
if isfile('/proc/cpuinfo')
    match = regexp(fileread('/proc/cpuinfo'),'model name\s*:\s*([^\r\n]+)','tokens','once');
    if ~isempty(match), machine.cpu = strtrim(match{1}); end
end
if isfile('/proc/meminfo')
    match = regexp(fileread('/proc/meminfo'),'MemTotal:\s*(\d+)','tokens','once');
    if ~isempty(match), machine.memoryGiB = str2double(match{1})/1024^2; end
end
if isunix
    [status,os] = system('uname -sr');
    if status==0, machine.os = strtrim(os); end
end
end
