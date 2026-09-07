function run_tests()
%RUN_TESTS Bounded regression suite; never run the paper-resolution search.
directory = fileparts(mfilename('fullpath'));
addpath(directory,fileparts(directory));
test_intlab_version;
test_centered_default;
test_active_pair_vertex_certificate;
test_geometric_certificates;
test_verification_progress;
test_certification_process;
% Includes a complete but deliberately coarse root search.
test_p2v0_certificate;
fprintf('=== ALL PRODUCTION INTLAB TESTS PASSED ===\n');
end
