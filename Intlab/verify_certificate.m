function report = verify_certificate(filename, intlabRoot, progressEvery)
%VERIFY_CERTIFICATE Recompute claims and the complete full-root cover.

if nargin < 2 || isempty(intlabRoot)
    intlabRoot = '/home/beni/INTLAB/Intlab_V13';
end
if nargin<3 || isempty(progressEvery), progressEvery = 100; end
report = verify_seeded_certificate(filename, root_box(), intlabRoot, true, progressEvery);
if ~logical(report.metadata.full_root) || report.metadata.seed_count ~= 1
    error('fence:notFullCover', ...
          'Expected a certificate generated from the single full root box.');
end
fprintf('Verified the complete gap-free root certificate.\n');
end
