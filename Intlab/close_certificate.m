function close_certificate(writer)
%CLOSE_CERTIFICATE Close a writer returned by open_certificate.
if ~isempty(writer) && isstruct(writer) && isfield(writer, 'fileId') && writer.fileId > 0
    fclose(writer.fileId);
end
end
