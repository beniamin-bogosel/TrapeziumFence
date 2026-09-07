function result = refine_survivors(sourceCertificate, options)
%REFINE_SURVIVORS Run the validator only on survivor boxes from a JSONL file.
%
% The source may be a C or MATLAB certificate.  The output covers exactly the
% source survivor boxes, not the original full root; retain the source file as
% part of a refinement chain.

if nargin < 2 || isempty(options)
    options = default_options();
end
defaults = default_options();
if ~isfield(options, 'intlabRoot')
    options.intlabRoot = defaults.intlabRoot;
end
if ~isfield(options, 'sourceVerified')
    options.sourceVerified = defaults.sourceVerified;
end

metadata = read_metadata(sourceCertificate);
nativeFullRoot = isfield(metadata, 'engine') && ...
                 strcmp(char(metadata.engine), 'MATLAB-INTLAB') && ...
                 isfield(metadata, 'full_root') && logical(metadata.full_root);
if nativeFullRoot
    % Prove that no pending subtree is silently omitted before extracting the
    % survivors. Claim recomputation remains a separate verification pass.
    sourceReport = verify_seeded_certificate( ...
        sourceCertificate, root_box(), options.intlabRoot, false);
    seeds = sourceReport.survivors;
elseif options.sourceVerified
    seeds = read_survivor_boxes(sourceCertificate);
else
    error('fence:unverifiedRefinementSource', ...
          ['The source is not a native full-root INTLAB certificate. Verify ' ...
           'its complete chain first, then set options.sourceVerified=true.']);
end
if isempty(seeds)
    error('fence:noSurvivors', 'No survivor boxes found in %s.', sourceCertificate);
end
fprintf('Refining %d survivor boxes from %s\n', numel(seeds), sourceCertificate);
options.sourceCertificate = char(sourceCertificate);
result = run_search(options, seeds);
result.sourceCertificate = sourceCertificate;
end

function metadata = read_metadata(filename)
[fileId, message] = fopen(filename, 'r');
if fileId < 0
    error('fence:certificateOpen', 'Cannot open %s: %s', filename, message);
end
closeFile = onCleanup(@() fclose(fileId));
line = fgetl(fileId);
if ~ischar(line)
    error('fence:badCertificate', 'Certificate is empty.');
end
metadata = jsondecode(line);
clear closeFile
end

function seeds = read_survivor_boxes(filename)
[fileId, message] = fopen(filename, 'r');
if fileId < 0
    error('fence:certificateOpen', 'Cannot open %s: %s', filename, message);
end
closeFile = onCleanup(@() fclose(fileId));
capacity = 1024;
count = 0;
lo = zeros(capacity, 4);
hi = zeros(capacity, 4);
while true
    line = fgetl(fileId);
    if ~ischar(line)
        break
    end
    if ~contains(line, '"status":"survivor"')
        continue
    end
    record = jsondecode(line);
    endpoints = double(record.box);
    if ~isequal(size(endpoints), [4,2])
        error('fence:badCertificate', ...
              'A survivor record does not contain a 4-by-2 box.');
    end
    count = count + 1;
    if count > capacity
        capacity = 2 * capacity;
        lo(capacity,4) = 0;
        hi(capacity,4) = 0;
    end
    lo(count,:) = endpoints(:,1).';
    hi(count,:) = endpoints(:,2).';
end
clear closeFile

seeds = struct('lo', {}, 'hi', {});
if count == 0
    return
end
seeds(count) = struct('lo', [], 'hi', []);
for k = 1:count
    seeds(k).lo = lo(k,:);
    seeds(k).hi = hi(k,:);
end
end
