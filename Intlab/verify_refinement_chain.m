function reports = verify_refinement_chain(files, intlabRoot, progressEvery)
%VERIFY_REFINEMENT_CHAIN Verify a base certificate and all refinements.
%
%   reports = verify_refinement_chain({base, refine1, refine2})
%
% Each refinement must cover exactly the survivor boxes of its predecessor.
% Prints stage numbers and, by default, progress every 100 records per file.
% Optional progressEvery=0 disables periodic output (not final summaries).

if nargin < 2 || isempty(intlabRoot)
    intlabRoot = '/home/beni/INTLAB/Intlab_V13';
end
if nargin<3 || isempty(progressEvery), progressEvery = 100; end
validateattributes(progressEvery,{'numeric'}, ...
    {'real','scalar','finite','integer','nonnegative'});
if ischar(files) || (isstring(files) && isscalar(files))
    files = cellstr(files);
elseif isstring(files)
    files = cellstr(files(:));
end
if ~iscell(files) || isempty(files)
    error('fence:badChain', 'files must be a nonempty cell array of filenames.');
end

started = tic;
fprintf('Verifying stage 1/%d\n',numel(files));
firstReport = verify_certificate(files{1}, intlabRoot, progressEvery);
reports = repmat(firstReport, 1, numel(files));
reports(1) = firstReport;
sourceSurvivors = reports(1).survivors;

for k = 2:numel(files)
    if isempty(sourceSurvivors)
        error('fence:badChain', ...
              'A refinement follows a stage with no survivors.');
    end
    fprintf('Verifying stage %d/%d; chain elapsed=%.1fs\n', ...
            k,numel(files),toc(started));
    reports(k) = verify_seeded_certificate( ...
        files{k}, sourceSurvivors, intlabRoot, true, progressEvery);
    if logical(reports(k).metadata.full_root)
        error('fence:badChain', 'Stage %d is marked as a full-root certificate.', k);
    end
    previousMetadata = reports(k - 1).metadata;
    currentMetadata = reports(k).metadata;
    if ~strcmp(char(currentMetadata.theta), char(previousMetadata.theta)) || ...
       logical(currentMetadata.half) ~= logical(previousMetadata.half)
        error('fence:badChain', ...
              'Stage %d changes the threshold or symmetry domain.', k);
    end
    if ~(double(currentMetadata.width_floor) < ...
         double(previousMetadata.width_floor))
        error('fence:badChain', ...
              'Stage %d does not decrease the width floor.', k);
    end
    if isfield(reports(k).metadata, 'source_certificate')
        recordedSource = char(reports(k).metadata.source_certificate);
        if ~isempty(recordedSource) && ~strcmp(recordedSource, char(files{k-1}))
            error('fence:badChain', ...
                  'Stage %d records a different source certificate.', k);
        end
    end
    sourceSurvivors = reports(k).survivors;
end

fprintf('Verified the complete %d-file chain; final survivors: %d.\n', ...
        numel(files), numel(sourceSurvivors));
fprintf('Total verification-chain elapsed: %.1fs.\n',toc(started));
end
