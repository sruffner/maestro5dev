function res = readcxdata_saccut(pathspec, verbose, nchans)
% Read in Maestro datafile and apply any saccade cuts to analog data therein.
% READCXDATA_SACCUT(PATHSPEC, [VERBOSE, NCHANS]) calls the MEX function 
% READCXDATA() to read in the contents of a Maestro data file, then applies 
% any saccade cuts specified in that file as XWork actions (ACTION_CUTIT). 
% If the function finds a problem with any saccade cut action specified in the 
% READCXDATA output, it displays a warning and skips that action.
%
% The arguments are the same arguments passed to READCXDATA(). Make sure that 
% your MATLAB path includes the correct version of READCXDATA() for your 
% operating system and MATLAB version.
%
% Arguments:
%   PATHSPEC    (required) A MATLAB string containing the pathname for the 
%   Maestro (or Cntrlx) data file to be processed. This can be an absolute 
%   path or a path relative to the current working directory.
%
%   VERBOSE     (optional) If nonzero, detailed progress messages are written 
%   to stdout. Typically used only for debugging purposes. Default = 0.
%
%   NCHANS      (optional) For headerless Continuous-mode data files (generated 
%   by Cntrlx versions dated before 29 Jan 2002), READCXDATA() must know how 
%   many analog data channels were recorded to properly parse the compressed 
%   analog data stored in the file. If an incorrect value is specified, then 
%   the decompressed analog data will be invalid. If not specified and the 
%   data file is headerless, the function will fail.
%
% Returns:
% The same MATLAB structure returned by READCXDATA(), except that the analog 
% data stored in the 'data' field has been altered to account for any XWork 
% saccade-cut actions also stored in the file. If the file contains no such 
% actions, then the output is identical to that returned by READCXDATA() itself.

% check # of input arguments, then invoke READCXDATA accordingly
if(nargin == 1)
   res = readcxdata(pathspec);
elseif(nargin == 2)
   res = readcxdata(pathspec, verbose);
elseif(nargin == 3)
   res = readcxdata(pathspec, verbose, nchans);
else
   error( 'READCXDATA_SACCUT requires 1-3 arguments' );
end

% if there are no saccade cut actions or no analog data, we're done! 
% Also check for bad field.
if(isempty(res.cut) | isempty(res.data))
   return;
end
if((size(res.cut,1) < 1) | (size(res.cut,2) ~= 3))
   error( 'Bad field (cut) in structure prepared by READCXDATA!');
end

% process each saccade cut in turn
nchans = size(res.data, 1);
nsamps = size(res.data, 2);
for(i = 1:size(res.cut,1))
   chan = res.cut(i,3);
   t0 = res.cut(i,1);
   t1 = res.cut(i,2);
   if(chan < 0 | chan >= nchans)
      disp( ['WARNING: Entry no. ' num2str(i) ' in cut field has bad channel #'] );
   elseif(t0 < 0 | t0 >= nsamps)
      disp( ['WARNING: Entry no. ' num2str(i) ' in cut field has bad start time'] );
   elseif(t1 <= t0 | t1 >= nsamps)
      disp( ['WARNING: Entry no. ' num2str(i) ' in cut field has bad end time'] );
   else
      slope = (res.data(chan+1, t1+1) - res.data(chan+1,t0+1)) / (t1 - t0 + 1);
      for(j = t0+1 : t1)
         res.data(chan+1, j) = res.data(chan+1, t0+1) + slope * (j - t0 - 1);
      end
   end
end


