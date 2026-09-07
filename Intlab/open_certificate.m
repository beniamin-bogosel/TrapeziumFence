function writer = open_certificate(filename, options, seedBoxes)
%OPEN_CERTIFICATE Open a JSONL certificate and write its metadata record.

if isfile(filename) && ~options.overwrite
    error('fence:certificateExists', ...
          ['Refusing to overwrite existing certificate %s. Choose a new ' ...
           'filename or set options.overwrite=true explicitly.'], filename);
end
[fileId, message] = fopen(filename, 'w');
if fileId < 0
    error('fence:certificateOpen', 'Cannot open %s: %s', filename, message);
end
writer.fileId = fileId;
writer.filename = filename;

isFullRoot = isempty(options.sourceCertificate) && isscalar(seedBoxes) && ...
             isequal(seedBoxes(1).lo, root_box().lo) && ...
             isequal(seedBoxes(1).hi, root_box().hi);
metadata.type = 'meta';
metadata.schema = 8;
metadata.engine = 'MATLAB-INTLAB';
metadata.theta = options.theta;
metadata.form = lower(options.form);
metadata.hybrid_width_trigger = options.hybridWidthTrigger;
metadata.half = logical(options.half);
metadata.short_edge_cert = logical(options.shortEdgeCertificate);
metadata.short_edge_epsilon = options.shortEdgeEpsilon;
metadata.geometric_cert = logical(options.geometricCertificates);
metadata.p2v0_cert = logical(options.p2v0Certificate);
metadata.active_pair_vertex_cert = logical(options.activePairVertexCertificate);
metadata.active_pair_vertex_reduction = 'P0_P1_P2V0_sections4_5_remark19_v1';
metadata.pair_eq_cert = logical(options.pairEqualityCertificate);
metadata.flat_area_cert = logical(options.flatAreaCertificate);
metadata.width_floor = options.widthFloor;
metadata.seed_count = numel(seedBoxes);
metadata.full_root = isFullRoot;
metadata.source_certificate = char(options.sourceCertificate);
metadata.root = [0,2; 0,1; -1,1; 0,1];
metadata.split_spans = [2,1,2,1];
fprintf(fileId, '%s\n', jsonencode(metadata));
end
