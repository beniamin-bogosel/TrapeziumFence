function result = run_search(options, seedBoxes)
%RUN_SEARCH Serial validated branch-and-bound search.
%
%   result = run_search(default_options())
%   result = run_search(options, seedBoxes)
%
% Search choices use ordinary binary64 arithmetic, but they are only
% heuristics.  Every terminal claim is decided from INTLAB enclosures.  The
% common split endpoint in split_box makes the collection of children an exact
% cover of the stored parent box.

if nargin < 1 || isempty(options)
    options = default_options();
end
if ~isfield(options, 'intlabRoot')
    defaults = default_options();
    options.intlabRoot = defaults.intlabRoot;
end
setup_intlab(options.intlabRoot);
options = complete_options(options);

if nargin < 2 || isempty(seedBoxes)
    seedBoxes = root_box();
end
seedBoxes = normalize_seeds(seedBoxes);

writer = [];
if ~isempty(options.outputFile)
    writer = open_certificate(options.outputFile, options, seedBoxes);
end
closeWriter = onCleanup(@() close_certificate(writer));

% A struct-array stack repeatedly reallocates and copies fields in MATLAB.
% Keep the hot work queue in four simple arrays with spare capacity, while
% retaining the readable box struct at the classifier boundary.
stackCapacity = max(1024, 2 * numel(seedBoxes));
stackLo = zeros(stackCapacity, 4);
stackHi = zeros(stackCapacity, 4);
stackPath = cell(stackCapacity, 1);
stackSeed = zeros(stackCapacity, 1);
stackTop = numel(seedBoxes);
for k = 1:stackTop
    stackLo(k,:) = seedBoxes(k).lo;
    stackHi(k,:) = seedBoxes(k).hi;
    stackPath{k} = seedBoxes(k).path;
    stackSeed(k) = seedBoxes(k).seed;
end
stats = empty_stats();
survivorCapacity = 1024;
survivorCount = 0;
survivorLo = zeros(survivorCapacity, 4);
survivorHi = zeros(survivorCapacity, 4);
survivorPath = cell(survivorCapacity, 1);
survivorSeed = zeros(survivorCapacity, 1);
keptLeaves = struct('lo', {}, 'hi', {}, 'path', {}, 'seed', {}, ...
                    'status', {}, 'item', {}, 'reason', {}, 'enclosure', {});

started = tic;
while stackTop > 0
    box.lo = stackLo(stackTop,:);
    box.hi = stackHi(stackTop,:);
    box.path = stackPath{stackTop};
    box.seed = stackSeed(stackTop);
    stackTop = stackTop - 1;
    category = categorize_box(box.lo, box.hi, options);

    if strcmp(category.status, 'survive')
        scaledWidth = scaled_box_width(box.lo, box.hi);
        if scaledWidth > options.widthFloor
            [leftChild, rightChild, didSplit] = split_box(box);
            if didSplit
                stats.internal = stats.internal + 1;
                % LIFO order chosen so the certificate is depth-first, left first.
                if stackTop + 2 > stackCapacity
                    oldCapacity = stackCapacity;
                    stackCapacity = 2 * stackCapacity;
                    stackLo(stackCapacity,4) = 0;
                    stackHi(stackCapacity,4) = 0;
                    stackPath{stackCapacity,1} = [];
                    stackSeed(stackCapacity,1) = 0;
                    assert(stackCapacity > oldCapacity);
                end
                stackTop = stackTop + 1;
                stackLo(stackTop,:) = rightChild.lo;
                stackHi(stackTop,:) = rightChild.hi;
                stackPath{stackTop} = rightChild.path;
                stackSeed(stackTop) = rightChild.seed;
                stackTop = stackTop + 1;
                stackLo(stackTop,:) = leftChild.lo;
                stackHi(stackTop,:) = leftChild.hi;
                stackPath{stackTop} = leftChild.path;
                stackSeed(stackTop) = leftChild.seed;
                continue
            end
        end
        category.status = 'survivor';
        category.reason = 'unresolved at the requested width floor';
    end

    stats = count_leaf(stats, category.status);
    leaf = make_leaf(box, category);
    if options.keepSurvivors && strcmp(leaf.status, 'survivor')
        survivorCount = survivorCount + 1;
        if survivorCount > survivorCapacity
            survivorCapacity = 2 * survivorCapacity;
            survivorLo(survivorCapacity,4) = 0;
            survivorHi(survivorCapacity,4) = 0;
            survivorPath{survivorCapacity,1} = [];
            survivorSeed(survivorCapacity,1) = 0;
        end
        survivorLo(survivorCount,:) = box.lo;
        survivorHi(survivorCount,:) = box.hi;
        survivorPath{survivorCount} = box.path;
        survivorSeed(survivorCount) = box.seed;
    end
    if options.keepLeaves
        keptLeaves(end + 1) = leaf; %#ok<AGROW>
    end
    if ~isempty(writer)
        write_certificate_leaf(writer, leaf);
    end

    if options.progressEvery > 0 && mod(stats.leaves, options.progressEvery) == 0
        if ~isempty(writer)
            % MATLAB has no fflush function.  A zero-distance seek is the
            % documented file-position operation and flushes pending output,
            % making a long-running certificate visible to another process.
            if fseek(writer.fileId, 0, 'cof') ~= 0
                error('fence:certificateFlush', ...
                      'Could not flush progress to the certificate file.');
            end
        end
        classified = stats.internal + stats.leaves;
        fprintf(['leaves=%d, classified=%d, pending=%d, survivors=%d, ' ...
                 'p2v0=%d, pair_vertex=%d, elapsed=%.1fs\n'], ...
                stats.leaves, classified, stackTop, stats.survivor, ...
                stats.p2v0_nonoptimal, stats.active_pair_vertex_incompatible, toc(started));
    end
    if stats.leaves >= options.maxLeaves && stackTop > 0
        stats.truncated = true;
        break
    end
end

stats.pending = stackTop;
stats.classified = stats.internal + stats.leaves;
stats.elapsedSeconds = toc(started);
stats.complete = stackTop == 0 && ~stats.truncated;
result.options = options;
result.stats = stats;
result.survivors = stack_boxes(survivorLo, survivorHi, survivorPath, ...
                               survivorSeed, survivorCount);
result.leaves = keptLeaves;
result.seedBoxes = seedBoxes;
result.pendingBoxes = stack_boxes(stackLo, stackHi, stackPath, stackSeed, stackTop);
clear closeWriter
end

function options = complete_options(options)
defaults = default_options();
names = fieldnames(defaults);
for k = 1:numel(names)
    if ~isfield(options, names{k})
        options.(names{k}) = defaults.(names{k});
    end
end
if ~(ischar(options.theta) || (isstring(options.theta) && isscalar(options.theta)))
    error('fence:badTheta', 'options.theta must be an exact decimal string.');
end
options.theta = char(options.theta);
decimalPattern = '^\+?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$';
if isempty(regexp(options.theta, decimalPattern, 'once'))
    error('fence:badTheta', 'theta must be a positive decimal literal.');
end
if ~(isscalar(options.shortEdgeCertificate) && ...
     (islogical(options.shortEdgeCertificate) || ...
      (isnumeric(options.shortEdgeCertificate) && ...
       isfinite(options.shortEdgeCertificate) && ...
       any(options.shortEdgeCertificate == [0,1]))))
    error('fence:badShortEdgeOption', ...
          'shortEdgeCertificate must be a logical scalar.');
end
options.shortEdgeCertificate = logical(options.shortEdgeCertificate);
if ~(ischar(options.shortEdgeEpsilon) || ...
     (isstring(options.shortEdgeEpsilon) && isscalar(options.shortEdgeEpsilon)))
    error('fence:badShortEdgeOption', ...
          'shortEdgeEpsilon must be an exact decimal string.');
end
options.shortEdgeEpsilon = char(options.shortEdgeEpsilon);
logicalNames = {'half','flatAreaCertificate','pairEqualityCertificate', ...
                'keepLeaves','keepSurvivors','overwrite','sourceVerified', ...
                'geometricCertificates','p2v0Certificate','activePairVertexCertificate'};
for k = 1:numel(logicalNames)
    value = options.(logicalNames{k});
    if ~(isscalar(value) && (islogical(value) || ...
         (isnumeric(value) && isfinite(value) && any(value == [0,1]))))
        error('fence:badLogicalOption', ...
              '%s must be a logical scalar.', logicalNames{k});
    end
    options.(logicalNames{k}) = logical(value);
end
if ~(ischar(options.sourceCertificate) || ...
     (isstring(options.sourceCertificate) && isscalar(options.sourceCertificate)))
    error('fence:badSourceCertificate', ...
          'sourceCertificate must be a character vector or scalar string.');
end
options.sourceCertificate = char(options.sourceCertificate);
if options.shortEdgeCertificate
    [~, ~, epsilonSquared] = short_edge_parameters( ...
        options.theta, options.shortEdgeEpsilon);
    options.shortEdgeSquaredLimit = inf(epsilonSquared);
else
    options.shortEdgeSquaredLimit = [];
end
thetaInterval = intval(options.theta);
if ~isfinite(inf(thetaInterval)) || ~isfinite(sup(thetaInterval)) || ...
   inf(thetaInterval) <= 0
    error('fence:badTheta', 'theta must be positive and finite.');
end
if ~(isnumeric(options.widthFloor) && isscalar(options.widthFloor) && ...
     isfinite(options.widthFloor) && options.widthFloor > 0)
    error('fence:badWidthFloor', 'widthFloor must be positive and finite.');
end
if ~(isnumeric(options.hybridWidthTrigger) && ...
     isscalar(options.hybridWidthTrigger) && ...
     isfinite(options.hybridWidthTrigger) && options.hybridWidthTrigger >= 0)
    error('fence:badHybridTrigger', ...
          'hybridWidthTrigger must be a nonnegative finite scalar.');
end
if ~any(strcmpi(options.form, {'natural','hybrid','centered'}))
    error('fence:badForm', ...
          'form must be ''natural'', ''hybrid'', or ''centered''.');
end
if ~(isscalar(options.maxLeaves) && ...
     (isinf(options.maxLeaves) || ...
      (options.maxLeaves >= 1 && options.maxLeaves == floor(options.maxLeaves))))
    error('fence:badMaxLeaves', 'maxLeaves must be a positive integer or Inf.');
end
if ~(isscalar(options.progressEvery) && isfinite(options.progressEvery) && ...
     options.progressEvery >= 0 && options.progressEvery == floor(options.progressEvery))
    error('fence:badProgress', 'progressEvery must be a nonnegative integer.');
end
options.form = lower(char(options.form));
end

function seeds = normalize_seeds(input)
required = {'lo','hi'};
for k = 1:numel(input)
    if ~all(isfield(input(k), required))
        error('fence:badSeeds', 'Every seed needs lo and hi fields.');
    end
    input(k).lo = reshape(input(k).lo, 1, 4);
    input(k).hi = reshape(input(k).hi, 1, 4);
    if any(~isfinite(input(k).lo)) || any(~isfinite(input(k).hi)) || ...
       any(input(k).lo > input(k).hi)
        error('fence:badSeeds', 'Seed endpoints must be finite proper boxes.');
    end
    input(k).path = '';
    input(k).seed = k;
end
seeds = input;
end

function stats = empty_stats()
stats = struct('internal',0, 'leaves',0, 'discarded',0, 'short_edge',0, ...
               'flat_area',0, 'p2v0_nonoptimal',0, 'active_pair_vertex_incompatible',0, ...
               'geometric_low',0, 'rational_pair_incompatible',0, ...
               'pair_incompatible',0, 'certified',0, 'survivor',0, ...
               'pending',0, 'classified',0, 'truncated',false, 'complete',false, ...
               'elapsedSeconds',0);
end

function stats = count_leaf(stats, status)
stats.leaves = stats.leaves + 1;
stats.(status) = stats.(status) + 1;
end

function leaf = make_leaf(box, category)
leaf.lo = box.lo;
leaf.hi = box.hi;
leaf.path = box.path;
leaf.seed = box.seed;
leaf.status = category.status;
leaf.item = category.item;
leaf.reason = category.reason;
leaf.enclosure = category.enclosure;
end

function boxes = stack_boxes(lo, hi, paths, seeds, top)
boxes = struct('lo', {}, 'hi', {}, 'path', {}, 'seed', {});
if top == 0
    return
end
boxes(top) = struct('lo', [], 'hi', [], 'path', '', 'seed', 0);
for k = 1:top
    boxes(k).lo = lo(k,:);
    boxes(k).hi = hi(k,:);
    boxes(k).path = paths{k};
    boxes(k).seed = seeds(k);
end
end
