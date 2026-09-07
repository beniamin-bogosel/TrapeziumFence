function records = certificate_record_count(filename)
%CERTIFICATE_RECORD_COUNT Fast line count for progress reporting, not proof.
% JSONL has one metadata line followed by one record per line. Count bytes
% in bounded chunks without parsing JSON or retaining the certificate.
[fileId,message] = fopen(filename,'rb');
if fileId<0
    error('fence:certificateOpen','Cannot open %s: %s',filename,message);
end
cleanup = onCleanup(@() fclose(fileId));
lines = 0;
lastByte = [];
while true
    bytes = fread(fileId,1024*1024,'*uint8');
    if isempty(bytes), break; end
    lines = lines+nnz(bytes==10);
    lastByte = bytes(end);
end
[message,errorNumber] = ferror(fileId);
% MATLAB can report normal end-of-file through ferror as well as feof.
if errorNumber~=0 && ~feof(fileId)
    error('fence:certificateRead','Cannot count %s: %s',filename,message);
end
if ~isempty(lastByte) && lastByte~=10
    lines = lines+1; % also count a final record with no newline
end
records = max(0,lines-1); % exclude the metadata line
end
