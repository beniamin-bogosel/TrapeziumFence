function report = verify_seeded_certificate(filename, seedBoxes, intlabRoot, recheckClaims, progressEvery)
%VERIFY_SEEDED_CERTIFICATE Verify claims and the cover of specified seed boxes.
%
% The certificate traversal is depth first and processes the final seed first,
% exactly as run_search does with its LIFO stack. Every stored endpoint is
% reconstructed from the seed and binary path; volumes are not a cover proof.
% Progress defaults to every 100 records; progressEvery=0 disables it.
% Percentages and ETA refer to this file, not the whole refinement chain.

if nargin < 3 || isempty(intlabRoot)
    intlabRoot = '/home/beni/INTLAB/Intlab_V13';
end
if nargin < 4 || isempty(recheckClaims)
    recheckClaims = true;
end
if nargin<5 || isempty(progressEvery), progressEvery = 100; end
validateattributes(progressEvery,{'numeric'}, ...
    {'real','scalar','finite','integer','nonnegative'});
totalRecords = 0;
if progressEvery>0
    mode = 'claims and coverage';
    if ~recheckClaims, mode = 'coverage only (claims NOT rechecked)'; end
    fprintf('Starting verification (%s): %s\n',mode,filename);
    totalRecords = certificate_record_count(filename);
    fprintf('  %d records; progress every %d; ETA is for this file only.\n', ...
            totalRecords,progressEvery);
end
setup_intlab(intlabRoot);
seeds = numbered_seeds(seedBoxes);

[fileId, message] = fopen(filename, 'r');
if fileId < 0
    error('fence:certificateOpen', 'Cannot open %s: %s', filename, message);
end
closeFile = onCleanup(@() fclose(fileId));

firstLine = fgetl(fileId);
if ~ischar(firstLine)
    error('fence:badCertificate', 'Certificate is empty.');
end
metadata = jsondecode(firstLine);
validate_metadata(metadata, numel(seeds));
options = options_from_metadata(metadata, intlabRoot);
theta = intval(options.theta);
thetaLower = inf(theta);
flatAreaLimit = inf(theta.^2 ./ 4);

seedOrder = numel(seeds):-1:1;
seedPosition = 1;
expected = seeds(seedOrder(seedPosition));
counts = empty_counts();
survivorCapacity = 1024;
survivorCount = 0;
survivorLo = zeros(survivorCapacity, 4);
survivorHi = zeros(survivorCapacity, 4);
checked = 0;
started = tic; % setup and the lightweight line-count pass are excluded
if progressEvery>0, print_progress(checked,totalRecords,started); end

while true
    line = fgetl(fileId);
    if ~ischar(line)
        break
    end
    while isempty(expected) && seedPosition < numel(seedOrder)
        seedPosition = seedPosition + 1;
        expected = seeds(seedOrder(seedPosition));
    end
    if isempty(expected)
        error('fence:overlappingCover', ...
              'Certificate has leaves after all seed boxes were covered.');
    end

    record = jsondecode(line);
    require_leaf_fields(record);
    currentSeed = seedOrder(seedPosition);
    if record.seed ~= currentSeed
        error('fence:seedOrder', 'Expected seed %d but found seed %d.', ...
              currentSeed, record.seed);
    end
    [current, expected] = reconstruct_next_box(expected, char(record.path));

    endpoints = double(record.box);
    if ~isequal(size(endpoints), [4,2]) || ...
       ~isequal(endpoints(:,1).', current.lo) || ...
       ~isequal(endpoints(:,2).', current.hi)
        error('fence:boxPathMismatch', ...
              'Stored endpoints do not match seed %d path %s.', ...
              currentSeed, char(record.path));
    end

    status = canonical_certificate_status(char(record.status));
    if strcmp(status,'active_pair_vertex_incompatible') && ...
       ~options.activePairVertexCertificate
        error('fence:falseClaim','Active pair/vertex rule is disabled in metadata.');
    end
    if strcmp(status,'p2v0_nonoptimal') && ~options.p2v0Certificate
        error('fence:falseClaim','P2V0 rule is disabled in metadata.');
    end
    if any(strcmp(status,{'geometric_low','rational_pair_incompatible'})) && ...
       ~options.geometricCertificates
        error('fence:falseClaim','Geometric rules are disabled in metadata.');
    end
    item = double(record.item);
    if ~(isscalar(item) && isfinite(item) && item == floor(item))
        error('fence:badCertificate', 'Leaf item must be a finite integer.');
    end
    if recheckClaims
        verify_claim(status, item, current.lo, current.hi, options, ...
                     thetaLower, flatAreaLimit);
    elseif strcmp(status, 'survivor') && ...
           scaled_box_width(current.lo, current.hi) > options.widthFloor
        error('fence:prematureSurvivor', ...
              'Seed %d path %s is wider than the recorded floor.', ...
              currentSeed, char(record.path));
    end

    counts.(status) = counts.(status) + 1;
    if strcmp(status, 'survivor')
        survivorCount = survivorCount + 1;
        if survivorCount > survivorCapacity
            survivorCapacity = 2 * survivorCapacity;
            survivorLo(survivorCapacity,4) = 0;
            survivorHi(survivorCapacity,4) = 0;
        end
        survivorLo(survivorCount,:) = current.lo;
        survivorHi(survivorCount,:) = current.hi;
    end
    checked = checked + 1;
    if progressEvery>0 && mod(checked,progressEvery)==0
        print_progress(checked,totalRecords,started);
    end
end
clear closeFile

if ~isempty(expected) || seedPosition ~= numel(seedOrder)
    error('fence:coverageGap', ...
          'Certificate ended before all seed boxes were covered.');
end

report.valid = true;
report.claimsRechecked = logical(recheckClaims);
report.leaves = checked;
report.counts = counts;
report.survivors = endpoint_boxes(survivorLo, survivorHi, survivorCount);
report.metadata = metadata;
report.elapsedSeconds = toc(started);
if progressEvery>0 && (checked==0 || mod(checked,progressEvery)~=0)
    print_progress(checked,totalRecords,started);
end
fprintf('Verified %d leaves and the complete cover of %d seed box(es).\n', ...
        checked, numel(seeds));
end

function print_progress(checked,total,started)
elapsed = toc(started);
percent = 100*checked/max(1,total);
eta = 'estimating';
if checked>0
    remaining = ceil(elapsed*max(0,total-checked)/checked);
    eta = sprintf('%02d:%02d:%02d',floor(remaining/3600), ...
                  floor(mod(remaining,3600)/60),mod(remaining,60));
end
fprintf('verify: checked=%d/%d (%.2f%%), elapsed=%.1fs, ETA(this file)=%s\n', ...
        checked,total,percent,elapsed,eta);
end

function boxes = endpoint_boxes(lo, hi, count)
boxes = struct('lo', {}, 'hi', {});
if count == 0
    return
end
boxes(count) = struct('lo', [], 'hi', []);
for k = 1:count
    boxes(k).lo = lo(k,:);
    boxes(k).hi = hi(k,:);
end
end

function seeds = numbered_seeds(input)
if isempty(input)
    error('fence:badSeeds', 'At least one seed box is required.');
end
seeds = input(:);
for k = 1:numel(seeds)
    if ~isfield(seeds(k), 'lo') || ~isfield(seeds(k), 'hi')
        error('fence:badSeeds', 'Every seed needs lo and hi fields.');
    end
    seeds(k).lo = reshape(double(seeds(k).lo), 1, 4);
    seeds(k).hi = reshape(double(seeds(k).hi), 1, 4);
    if any(~isfinite(seeds(k).lo)) || any(~isfinite(seeds(k).hi)) || ...
       any(seeds(k).lo > seeds(k).hi)
        error('fence:badSeeds', 'Seed endpoints must be finite proper boxes.');
    end
    seeds(k).path = '';
    seeds(k).seed = k;
end
end

function validate_metadata(metadata, seedCount)
required = {'type','schema','engine','theta','form','half','pair_eq_cert', ...
            'flat_area_cert','width_floor','seed_count','full_root','root', ...
            'split_spans'};
if ~all(isfield(metadata, required)) || ~strcmp(metadata.type, 'meta') || ...
   ~strcmp(metadata.engine, 'MATLAB-INTLAB') || ...
   ~any(double(metadata.schema) == [4,5,6,7,8])
    error('fence:badCertificate', 'Missing or unsupported INTLAB metadata.');
end
if metadata.seed_count ~= seedCount
    error('fence:badCertificate', ...
          'Metadata records %d seeds but %d were supplied.', ...
          metadata.seed_count, seedCount);
end
hasShortFlag = isfield(metadata, 'short_edge_cert');
if metadata.schema>=8
    if ~isfield(metadata,'active_pair_vertex_cert') || ...
       ~(isscalar(metadata.active_pair_vertex_cert) && ...
         (islogical(metadata.active_pair_vertex_cert) || ...
          (isnumeric(metadata.active_pair_vertex_cert) && ...
           any(metadata.active_pair_vertex_cert==[0,1])))) || ...
       ~isfield(metadata,'active_pair_vertex_reduction') || ...
       ~strcmp(metadata.active_pair_vertex_reduction, ...
               'P0_P1_P2V0_sections4_5_remark19_v1')
        error('fence:badCertificate','Schema 8 requires the active-set flag and reduction.');
    end
end
if metadata.schema>=7
    if ~isfield(metadata,'p2v0_cert') || ...
       ~(isscalar(metadata.p2v0_cert) && ...
         (islogical(metadata.p2v0_cert) || ...
          (isnumeric(metadata.p2v0_cert) && any(metadata.p2v0_cert==[0,1]))))
        error('fence:badCertificate','Schema 7 requires logical p2v0_cert.');
    end
end
if metadata.schema >= 6
    if ~isfield(metadata,'geometric_cert') || ...
       ~(isscalar(metadata.geometric_cert) && ...
         (islogical(metadata.geometric_cert) || ...
          (isnumeric(metadata.geometric_cert) && ...
           any(metadata.geometric_cert == [0,1]))))
        error('fence:badCertificate','Schema 6 requires logical geometric_cert.');
    end
end
hasShortEpsilon = isfield(metadata, 'short_edge_epsilon');
if xor(hasShortFlag, hasShortEpsilon) || ...
   (metadata.schema >= 5 && ~(hasShortFlag && hasShortEpsilon))
    error('fence:badCertificate', 'Incomplete short-edge metadata.');
end
if metadata.schema >= 5 && ~isfield(metadata, 'source_certificate')
    error('fence:badCertificate', 'Schema 5 requires source provenance.');
end
if metadata.schema >= 5 && ~isfield(metadata, 'hybrid_width_trigger')
    error('fence:badCertificate', 'Schema 5 requires the hybrid width trigger.');
end
if metadata.schema >= 5 && ...
   ~(isscalar(metadata.hybrid_width_trigger) && ...
     isfinite(metadata.hybrid_width_trigger) && ...
     metadata.hybrid_width_trigger >= 0)
    error('fence:badCertificate', 'Invalid hybrid width trigger.');
end
thetaText = char(metadata.theta);
decimalPattern = '^\+?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$';
if isempty(regexp(thetaText, decimalPattern, 'once'))
    error('fence:badCertificate', 'Metadata theta is not a decimal literal.');
end
theta = intval(thetaText);
allowedForms = {'natural','centered'};
if metadata.schema >= 5
    allowedForms{end + 1} = 'hybrid';
end
if ~isfinite(inf(theta)) || ~isfinite(sup(theta)) || inf(theta) <= 0 || ...
   ~(isfinite(metadata.width_floor) && metadata.width_floor > 0) || ...
   ~any(strcmp(char(metadata.form), allowedForms))
    error('fence:badCertificate', 'Invalid proof parameters in metadata.');
end
expectedRoot = root_box();
storedRoot = double(metadata.root);
if ~isequal(size(storedRoot), [4,2]) || ...
   ~isequal(storedRoot(:,1).', expectedRoot.lo) || ...
   ~isequal(storedRoot(:,2).', expectedRoot.hi) || ...
   ~isequal(reshape(double(metadata.split_spans),1,[]), [2,1,2,1])
    error('fence:badCertificate', ...
          'Root box or split spans do not match the implementation.');
end
end

function options = options_from_metadata(metadata, intlabRoot)
options = default_options();
options.activePairVertexCertificate = metadata.schema>=8 && ...
                                     logical(metadata.active_pair_vertex_cert);
options.p2v0Certificate = metadata.schema>=7 && logical(metadata.p2v0_cert);
% Never reinterpret an older certificate as enabling new geometric statuses.
options.geometricCertificates = metadata.schema >= 6 && ...
                                logical(metadata.geometric_cert);
options.theta = char(metadata.theta);
options.form = char(metadata.form);
if isfield(metadata, 'hybrid_width_trigger')
    options.hybridWidthTrigger = double(metadata.hybrid_width_trigger);
end
options.half = logical(metadata.half);
if isfield(metadata, 'short_edge_cert')
    options.shortEdgeCertificate = logical(metadata.short_edge_cert);
    options.shortEdgeEpsilon = char(metadata.short_edge_epsilon);
    if options.shortEdgeCertificate
        [~, ~, epsilonSquared] = short_edge_parameters( ...
            options.theta, options.shortEdgeEpsilon);
        options.shortEdgeSquaredLimit = inf(epsilonSquared);
    else
        options.shortEdgeSquaredLimit = [];
    end
else
    options.shortEdgeCertificate = false;
    options.shortEdgeSquaredLimit = [];
end
options.pairEqualityCertificate = logical(metadata.pair_eq_cert);
options.flatAreaCertificate = logical(metadata.flat_area_cert);
options.widthFloor = double(metadata.width_floor);
options.intlabRoot = intlabRoot;
end

function require_leaf_fields(record)
required = {'box','status','item','seed','path'};
if ~all(isfield(record, required))
    error('fence:badCertificate', 'A leaf record is missing required fields.');
end
end

function [current, expected] = reconstruct_next_box(expected, desiredPath)
current = expected(end);
expected(end) = [];
if ~startsWith(desiredPath, current.path)
    error('fence:coverageGap', 'Unexpected path %s after %s.', ...
          desiredPath, current.path);
end
while length(current.path) < length(desiredPath)
    nextBit = desiredPath(length(current.path) + 1);
    [left, right, didSplit] = split_box(current);
    if ~didSplit
        error('fence:unsplittablePath', ...
              'Path %s subdivides an unsplittable box.', desiredPath);
    end
    if nextBit ~= '0'
        error('fence:coverageGap', 'Left subtree %s is missing.', left.path);
    end
    expected(end + 1) = right; %#ok<AGROW>
    current = left;
end
end

function verify_claim(status, item, lo, hi, options, thetaLower, flatAreaLimit)
switch status
    case 'active_pair_vertex_incompatible'
        if ~options.activePairVertexCertificate || item~=0
            error('fence:falseClaim','Invalid or disabled active pair/vertex claim.');
        end
        requested = [5,6];
        if strcmp(options.form,'centered'), requested = 1:6; end
        evaluation = evaluate_fences(lo,hi,options.form,requested,[],false,[],[],true);
        if ~evaluation.activePairVertexExcluded
            error('fence:falseClaim','Active pair/vertex incompatibility was not proved.');
        end

    case 'p2v0_nonoptimal'
        if ~options.p2v0Certificate || ~any(item==[5,6])
            error('fence:falseClaim','Invalid or disabled P2V0 claim.');
        end
        [proved,checkedItem] = p2v0_exclusion(lo,hi,item);
        if ~proved || checkedItem~=item
            error('fence:falseClaim','Vertex inactivity was not proved.');
        end

    case {'geometric_low','rational_pair_incompatible'}
        if strcmp(status,'geometric_low')
            validItem = item>=1 && item<=15;
        else
            validItem = item==16 && options.pairEqualityCertificate;
        end
        if ~options.geometricCertificates || ~validItem
            error('fence:falseClaim','Invalid or disabled geometric claim.');
        end
        [proved, checkedItem] = geometric_exclusion( ...
            lo, hi, options.theta, options.pairEqualityCertificate, item);
        if ~proved || checkedItem ~= item
            error('fence:falseClaim','Recorded geometric inequality failed.');
        end

    case 'discarded'
        if ~certainly_inadmissible(lo, hi, options.half)
            error('fence:falseClaim', 'Recorded discarded box is not excluded.');
        end

    case 'short_edge'
        if ~options.shortEdgeCertificate
            error('fence:falseClaim', 'Short-edge rule is disabled in metadata.');
        end
        [~, ~, sideSquared] = certainly_inadmissible(lo, hi, options.half);
        if isempty(sideSquared)
            error('fence:falseClaim', 'Short-edge box has no side enclosure.');
        end
        [proved, edge] = short_edge_certifies_nonoptimal( ...
            sideSquared, options.theta, options.shortEdgeEpsilon, ...
            options.shortEdgeSquaredLimit);
        if ~proved || item ~= edge
            error('fence:falseClaim', 'Recorded short-edge claim is invalid.');
        end

    case 'flat_area'
        if ~options.flatAreaCertificate
            error('fence:falseClaim', 'Flat-area rule is disabled in metadata.');
        end
        x = infsup(lo, hi);
        area = quadrilateral_area(x);
        if ~finite_interval(area) || sup(area) > flatAreaLimit
            error('fence:falseClaim', 'Recorded flat-area claim is invalid.');
        end

    case 'pair_incompatible'
        if ~options.pairEqualityCertificate
            error('fence:falseClaim', 'Pair-equality rule is disabled in metadata.');
        end
        x = infsup(lo, hi);
        area = quadrilateral_area(x);
        evaluation = evaluate_fences(lo, hi, options.form, [5,6], [], ...
                                     false, area, x);
        pair1 = evaluation.items(5);
        pair2 = evaluation.items(6);
        if ~finite_interval(pair1) || ~finite_interval(pair2) || ...
           ~(sup(pair1) < inf(pair2) || sup(pair2) < inf(pair1))
            error('fence:falseClaim', ...
                  'Recorded opposite-pair intervals are not disjoint.');
        end

    case 'certified'
        if item < 1 || item > 6
            error('fence:falseClaim', 'Certified fence item must be 1 through 6.');
        end
        x = infsup(lo, hi);
        area = quadrilateral_area(x);
        evaluation = evaluate_fences(lo, hi, options.form, item, ...
                                     thetaLower, false, area, x);
        fence = evaluation.items(item);
        if ~finite_interval(fence) || sup(fence) > thetaLower
            error('fence:falseClaim', 'Recorded explicit fence is not certified.');
        end

    case 'survivor'
        if scaled_box_width(lo, hi) > options.widthFloor
            error('fence:prematureSurvivor', ...
                  'Recorded survivor is wider than the requested floor.');
        end
end
end

function yes = finite_interval(value)
yes = ~any(isnan(inf(value))) && ~any(isnan(sup(value))) && ...
      all(isfinite(inf(value))) && all(isfinite(sup(value)));
end

function counts = empty_counts()
names = {'discarded','short_edge','flat_area','pair_incompatible', ...
         'active_pair_vertex_incompatible', ...
         'geometric_low','rational_pair_incompatible','p2v0_nonoptimal', ...
         'certified','survivor'};
counts = cell2struct(num2cell(zeros(size(names))), names, 2);
end
