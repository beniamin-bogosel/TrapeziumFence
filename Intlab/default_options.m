function options = default_options()
%DEFAULT_OPTIONS Parameters for the readable MATLAB/INTLAB search.

options.theta = '1.0496';
options.widthFloor = 0.025;
options.form = 'centered';      % natural pass, then center all six candidates as needed
options.hybridWidthTrigger = 0.03125; % used only when form='hybrid'
options.half = false;
options.shortEdgeCertificate = true;
options.shortEdgeEpsilon = '0.212';
options.geometricCertificates = true; % chord, angle, and rational pair tests
options.p2v0Certificate = true; % exclude all four vertex fences being inactive
options.activePairVertexCertificate = true; % J=I5 intersect I6 must meet a vertex
options.shortEdgeSquaredLimit = []; % populated once by run_search
options.flatAreaCertificate = true;
options.pairEqualityCertificate = true;
options.outputFile = '';
options.overwrite = false;
options.sourceCertificate = '';
options.sourceVerified = false; % opt-in for externally verified C/fragments
options.keepLeaves = false;
options.keepSurvivors = true;
options.maxLeaves = inf;         % debugging limit; truncates the cover
options.progressEvery = 100;
options.intlabRoot = '/home/beni/INTLAB/Intlab_V13';
end
