function setup_intlab(intlabRoot)
%SETUP_INTLAB Add INTLAB to the path and select fail-closed exception handling.
%
%   setup_intlab
%   setup_intlab('/path/to/Intlab_V13')
%
% The default is the installation used for this repository.  INTLAB may create
% a small, release-specific cache file in its installation directory the first
% time startintlab is called.

if nargin < 1 || isempty(intlabRoot)
    intlabRoot = '/home/beni/INTLAB/Intlab_V13';
end

if ~isfolder(intlabRoot)
    error('fence:intlabNotFound', 'INTLAB directory not found: %s', intlabRoot);
end

addpath(intlabRoot);

global INTLAB_CONST
if isempty(INTLAB_CONST)
    previousDirectory = pwd;
    restoreDirectory = onCleanup(@() cd(previousDirectory));
    cd(intlabRoot);
    startintlab;
    clear restoreDirectory
end

% Domain errors must produce NaN intervals.  A NaN is always treated as
% undecided by the fence code and can therefore never certify a box.
intvalinit('RealStdFctsExcptnNaN', 0);
end
