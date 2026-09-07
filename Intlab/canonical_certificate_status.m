function status = canonical_certificate_status(status)
%CANONICAL_CERTIFICATE_STATUS Normalize legacy certificate status names.

if strcmp(status, 'low')
    status = 'certified';
elseif strcmp(status, 'nonoptimal')
    status = 'pair_incompatible';
end
allowed = {'discarded','short_edge','flat_area','pair_incompatible', ...
           'active_pair_vertex_incompatible', ...
           'geometric_low','rational_pair_incompatible','p2v0_nonoptimal', ...
           'certified','survivor'};
if ~any(strcmp(status, allowed))
    error('fence:badCertificate', 'Unknown status %s.', status);
end
end
