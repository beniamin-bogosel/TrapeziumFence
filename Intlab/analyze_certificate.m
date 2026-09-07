function summary = analyze_certificate(filename)
%ANALYZE_CERTIFICATE Stream a JSONL certificate and summarize its leaves.
%
% Volumes and distances are ordinary binary64 diagnostics.  Proof claims must
% be checked by rerunning the interval predicates, not by trusting this report.

[fileId, message] = fopen(filename, 'r');
if fileId < 0
    error('fence:certificateOpen', 'Cannot open %s: %s', filename, message);
end
closeFile = onCleanup(@() fclose(fileId));

names = {'discarded','short_edge','flat_area','pair_incompatible', ...
         'active_pair_vertex_incompatible', ...
         'geometric_low','rational_pair_incompatible','p2v0_nonoptimal', ...
         'certified','survivor'};
counts = cell2struct(num2cell(zeros(size(names))), names, 2);
volumes = counts;
survivorLo = inf(1,4);
survivorHi = -inf(1,4);
metadata = struct();

while true
    line = fgetl(fileId);
    if ~ischar(line)
        break
    end
    record = jsondecode(line);
    if isfield(record, 'type') && strcmp(record.type, 'meta')
        metadata = record;
        continue
    end
    status = char(record.status);
    if strcmp(status, 'nonoptimal')
        status = 'pair_incompatible';
    elseif strcmp(status, 'low')
        status = 'certified';
    end
    if ~isfield(counts, status)
        error('fence:badCertificate', 'Unknown status %s.', status);
    end
    box = double(record.box);
    lo = box(:,1).';
    hi = box(:,2).';
    volume = prod(hi - lo);
    counts.(status) = counts.(status) + 1;
    volumes.(status) = volumes.(status) + volume;
    if strcmp(status, 'survivor')
        survivorLo = min(survivorLo, lo);
        survivorHi = max(survivorHi, hi);
    end
end
clear closeFile

summary.metadata = metadata;
summary.counts = counts;
summary.volumes = volumes;
summary.totalLeaves = sum(structfun(@double, counts));
summary.totalVolume = sum(structfun(@double, volumes));
summary.survivorBox.lo = survivorLo;
summary.survivorBox.hi = survivorHi;

fprintf('Leaves: %d, diagnostic total volume: %.17g\n', ...
        summary.totalLeaves, summary.totalVolume);
for k = 1:numel(names)
    fprintf('  %-19s %9d   volume %.12g\n', names{k}, ...
            counts.(names{k}), volumes.(names{k}));
end
if counts.survivor > 0
    fprintf('Survivor outer rectangle (paper notation):\n');
    fprintf('  A: [%.17g, %.17g] x [%.17g, %.17g]\n', ...
            survivorLo(3), survivorHi(3), survivorLo(4), survivorHi(4));
    fprintf('  B: [%.17g, %.17g] x [%.17g, %.17g]\n', ...
            survivorLo(1), survivorHi(1), survivorLo(2), survivorHi(2));
end
fprintf('Volumes above are diagnostics, not interval certificates.\n');
end
