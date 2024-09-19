function res = extractsectiondata(pspec, tnames, prolog, epilog, numunits, skipOnMarked, sparseOut, verbose)
% Extract recorded data for tagged sections culled from a set of Maestro trial data files.
%
% EXTRACTSECTIONDATA(PSPEC,TNAMES,PROLOG,EPILOG,NUMUNITS,SKIPONMARKED,SPARSEOUT,VERBOSE)
% extracts eye trajector and neural unit response data for any tagged
% sections defined in a specified set of Maestro trial data files,
% which may or may not contain "sorted spike trains" as the result of
% post-processing in XWORK or merging with the data stream simultaneously
% collected on the Plexon Multichannel Acquisition system (eg, via the
% PLEXSYNCHRONIZE M-function).  It uses the MEX function READCXDATA() to
% read in the contents of each file satisfying the path specification.
%
% This function handles the mundane task of collecting the relevant
% recorded data for each and every repetition of a unique tagged
% section across all trials processed -- the idea being that each
% such section represents a distinct experimental condition.
%
% Arguments (all are required):
%   PATHSPEC:  A MATLAB string specifying a pathname, absolute or relative to
%     the current working directory, that identifies one or more Maestro data
%     files. The '*' wildcard will likely be used to specify a bunch of files
%     to examine. This argument is passed to the MATLAB function DIR to obtain
%     a list of files to process.  If PATHSPEC is an empty string, then the
%     function will check all files in the current working directory.
%
%   TRIALNAMES:  A MATLAB cell array of strings, eg: {'trial1'; 'trial2'}.
%     This argument provides a means of narrowing the set of trial data files
%     from which tagged section data is collected.  If it is not empty, the
%     function only processes data files that satisfy the path specification
%     AND contain data for a Maestro trial that is named in this cell array.
%
%   PROLOG, EPILOG:  These scalars specify the length, in milliseconds, of the
%     section prolog and epilog, ie, the portions of recorded data to include
%     before the section started and after it ended, respectively. Allowed
%     range is [0..500].
%
%   NUMUNITS: The # of "sorted spike trains" expected in each data file.  The
%     assumption here is that one monitored a set number of units during the
%     time the trials were presented.  Some units may have been lost or
%     acquired during that time, but it must be true that an identified unit
%     is assigned to the same spike train channel in all data files.
%     EXTRACTSECTIONDATA will only look at the first NUMUNITS spike train
%     channels in the data file.  If NUMUNITS is 0, it will ignore these
%     channels entirely.  Allowed range is [0..13].
%
%   SKIPONMARKED:  If nonzero, the function will ignore any data files for
%     which the 'marked' flag is set in the structure returend by READCXDATA.
%
%   SPARSEOUT:  This flag selects two alternative formats for the unit response
%     data. If nonzero, sparse matrices are used -- these are easier to
%     process but involve a loss of precision in spike times, which are
%     effectively rounded up to the nearest millisecond.  If zero, a single
%     instance of one unit's response trace is a vector of actual spike times;
%     these vectors are collected in cell arrays because they won't all be the
%     same length.  The spike times (relative to the start of the prolog) are
%     in milliseconds, but preserved with their original recorded precision.
%
%   VERBOSE:  If nonzero, the function will print out lots of progress
%     messages.  This is intended for debugging purposes and will generally
%     be set to zero.
%
% Returns:
% A 1xM MATLAB array of structures, where M is the # of unique tagged sections
% culled from the data files.  Each structure includes the following fields:
%
%   tag:  A MATLAB string holding the section name (as defined in Maestro).
%   dur:  The duration of the section, including prolog and epilog, in ms.
%   nreps:  Number of *valid* repetitions of the section found across all
%     trials examined.  See notes below on what is considered a "valid"
%     repetition.
%
%   hgpos, vepos, hevel, vevel, hdvel: Behavioral response data recorded by
%     Maestro (on channels HGPOS, VEPOS, etc.).  Eye position is reported in
%     degrees, eye velocity in deg/sec.  Each field is a P x dur matrix, where
%     each row is the channel data recorded over the course of a particular
%     repetition of the tagged section (including prolog and epilog). Normally,
%     P=nreps, but it could be less if the channel was not recorded during some
%     or all repetitions of the section.
%
%   unit0:  Spike times recorded by Maestro on its digital input channel 0.
%     The format of this field depends upon the SPARSEOUT flag.
%
%     If SPARSEOUT != 0, the field is an nreps x dur sparse matrix, S, where
%     S(i,j) = 1 if a spike occurred during the j-th millisecond of the i-th
%     repetition of the tagged section.  Note that this form involves a loss in
%     the original precision with which spikes are timestamped in Maestro.
%
%     If SPARSEOUT == 0, the field is an nreps x 1 cell array, C, where C{i} is
%     an n-vector of the n spike times recorded during the i-th repetition of
%     the tagged section (of course, n is likely to vary with each repetition!).
%     Each spike timestamp is in milliseconds, but with 10-microsecond resolution.
%
%   sortedUnits:  Spike times from one or more "sorted spike train" channels
%      stored in the Maestro data file as the result of post-processing. We
%      know of two ways to augment the Maestro data file this way.  XWORK can
%      cull additional units from the 25KHz spike waveform optionally recorded
%      by Maestro. More recently, David Schoppik developed PLEXSYNCHRONIZE to
%      merge the unit data recorded on the Plexon Multichannel Acquisition
%      Processor with the relevant Maestro files.  The field is an Nx1 cell
%      array, where N = NUMUNITS.  Each individual cell in this array has
%      the same format as the unit0 field, as described above.
%
% DETAILS:
% 1) The files processed by EXTRACTSECTIONDATA will meet the following criteria:
%    -- File satisfies the path specification provided in PSPEC.
%    -- File is a valid Maestro trial data file.
%    -- If TNAMES is not empty, the trial name stored in the file header must
%       match one of the strings in TNAMES.
%    -- If SKIPONMARKED is nonzero, the file has NOT been marked (ie, the 'marked'
%       field is zero in the structure returned by READCXDATA).
%    -- Trial does NOT include a "skip on saccade" operation -- because READCXDATA
%       cannot reliably determine the start time of any tagged section after the
%       "skip" occurs.
%    -- File contains at least one *valid* repetition of a tagged section.
%
% 2) The INTENT behind the tagged sections feature in Maestro is that each distinct
% tagged section in a trial set will have the same duration in any trial in which
% it appears, and that the recorded data will cover the entire section as well as
% the prolog and epilog.  In this case, accumulating the results across any number
% of trial reps is straightforward.  However, Maestro cannot enforce this intent;
% the user can easily create a set of trials in which a named section has different
% lengths in different trials.  In addition, the user could choose a prolog and
% epilog that extend beyond the recorded portion of a trial, or define some trials
% so that a tagged section (or a portion of the prolog) begins before data
% recording is engaged.  THE USER MUST DESIGN TRIALS TO AVOID THESE SITUATIONS!!
% EXTRACTSECTIONDATA **requires** that all VALID repetitions of a tagged section
% (including prolog and epilog) have the same recorded duration as the very first
% instance encountered of that particular tagged section. In verbose mode, the
% function spits out a warning message each time it encounters an invalid
% repetition.  Only valid reps are included in the results structure.
%
% 3) CAVEAT for Plexon users.  If a Plexon-monitored unit fails to respond at all
% during a trial, the PLEXSYNCHRONIZE function will set the corresponding "sorted
% spike train" channel in the Maestro data file to [-1] to indicate that the unit
% really did not respond or [-99] to indicate that the unit was not held during
% the time of the trial.  EXTRACTSECTIONDATA does NOT pass on these special codes;
% the unit is treated as if it simply did not fire during any of the tagged
% sections culled from the trial.

% some Maestro-related constants...
CXHF_ISCONTINUOUS = bitshift(1,0);                       % header record flags
CXHF_HASTAGSECTS = bitshift(1,7);
CH_HGPOS = 0;                                            % AI channel #s
CH_VEPOS = 1;
CH_HEVEL = 2;
CH_VEVEL = 3;
CH_HDVEL = 8;
POS_AIRAW2DEG = 0.025;
VEL_AIRAW2DPS = 0.0918892;

% do we have the required # of input arguments and output arguments?
if( nargin ~= 8 | nargout ~= 1 )
   error( 'EXTRACTSECTIONDATA requires 8 inputs and 1 output!' );
end

% make sure PSPEC arg is a string vector
bOk = ischar(pspec) & (ndims(pspec)==2);
temp = size(pspec);
if( bOk )
   bOk = (temp(1)==1 | temp(2)==1);
end
if( ~bOk )
   error( 'Arg PSPEC must be a string vector!' );
end

% make sure TNAMES arg is a 1xM or Mx1 cell array of string vectors only
if( ~isempty(tnames) )
   bOk = iscell(tnames) & (ndims(tnames)==2);
   temp = size(tnames);
   if( bOk )
      bOk = (temp(1)==1 | temp(2)==1);
   end
   if( bOk )
      for( m=1:length(tnames) )
         name = tnames{m};
         bOk = ischar(name) & (ndims(name)==2);
         temp = size(name);
         if( bOk )
            bOk = (temp(1)==1 | temp(2)==1);
         end
         if( ~bOk )
            break;
         end
      end
   end
   if( ~bOk )
      error( 'Arg TNAMES must be a 1xN or Nx1 cell array of string vectors!' );
   end
end

% all other args must be numeric scalars!  We don't use isscalar() b/c it is not supported in Matlab 6.x.
bOk = isnumeric(prolog) & (ndims(prolog)==2);
if( bOk )
   bOk = (size(prolog,1)==1) & (size(prolog,2)==1); 
end
if( ~bOk )
   error( 'Arg PROLOG must be a numeric scalar!' )
end

bOk = isnumeric(epilog) & (ndims(epilog)==2);
if( bOk )
   bOk = (size(epilog,1)==1) & (size(epilog,2)==1); 
end
if( ~bOk )
   error( 'Arg EPILOG must be a numeric scalar!' )
end

bOk = isnumeric(numunits) & (ndims(numunits)==2);
if( bOk )
   bOk = (size(numunits,1)==1) & (size(numunits,2)==1); 
end
if( ~bOk )
   error( 'Arg NUMUNITS must be a numeric scalar!' )
end

bOk = isnumeric(skipOnMarked) & (ndims(skipOnMarked)==2);
if( bOk )
   bOk = (size(skipOnMarked,1)==1) & (size(skipOnMarked,2)==1); 
end
if( ~bOk )
   error( 'Arg SKIPONMARKED must be a numeric scalar!' )
end

bOk = isnumeric(sparseOut) & (ndims(sparseOut)==2);
if( bOk )
   bOk = (size(sparseOut,1)==1) & (size(sparseOut,2)==1); 
end
if( ~bOk )
   error( 'Arg SPARSEOUT must be a numeric scalar!' )
end

bOk = isnumeric(verbose) & (ndims(verbose)==2);
if( bOk )
   bOk = (size(verbose,1)==1) & (size(verbose,2)==1); 
end
if( ~bOk )
   error( 'Arg VERBOSE must be a numeric scalar!' )
end


% restrict prolog and epilog durations to the range [0..500]
prolog = round(prolog);
if( prolog < 0 )
   prolog = 0;
elseif( prolog > 500 )
   prolog = 500;
end

epilog = round(epilog);
if( epilog < 0 )
   epilog = 0;
elseif( epilog > 500 )
   epilog = 500;
end

% restrict numunits to the range [0..13]
numunits = round(numunits);
if( numunits < 0 )
   numunits = 0;
elseif( numunits > 13 )
   numunits = 13;
end

% start with a 0x0 structure with the expected fields defined
res = struct('tag', {}, 'dur', {}, 'nreps', {}, 'hgpos', {}, 'vepos', {}, 'hevel', {}, 'vevel', {}, 'hdvel', {}, ...
             'unit0', {}, 'sortedUnits', {});

% extract directory (path relative to current working dir) from path spec, and get the list of files that satisfy the
% path spec.  If there are no files to process, abort.  In this case, the results structure is empty.
fileDir = fileparts(pspec);
files = dir(pspec);
if( isempty(files) )
   if( verbose )
      warning( 'Found no files satisfying path specification!' );
   end
   return;
end

% process each file in our file list...
existingTags = {};                                                      % tags of sections encountered thus far

for( m=1:numel(files) )
   if( files(m).isdir ~= 0 )                                            % skip directories!
      continue;
   end

   filename = fullfile(fileDir, files(m).name);                         % attempt to read file as a Maestro data file
   trialdata = readcxdata(filename);
   if( isempty(trialdata.key) )                                         % file was probably NOT a Maestro file!
      if( verbose )
         disp([filename ' SKIPPED -- NOT a Maestro file']);
      end
      continue;
   end

   if( bitand(trialdata.key.flags, CXHF_ISCONTINUOUS) ~= 0 )            % we ignore Continuous-mode Maestro files!
      if( verbose )
         disp([filename ' SKIPPED -- Continuous-mode file']);
      end
      continue;
   end

   if( ~isempty(tnames) )                                               % if trial names were supplied, ignore trial if
      if( isempty(strmatch(trialdata.trialname, tnames, 'exact')) )     % its name is not in the supplied list.
         if( verbose )
            disp([filename ' SKIPPED -- not one of named trials']);
         end;
         continue;
      end
   end

   if( skipOnMarked )                                                   % if skipOnMarked nonzero, ignore files that
      marked = trialdata.marked;                                        % have been "marked"
      if( (~isempty(marked)) & marked(1) ~= 0 )
         if( verbose )
            disp([filename ' SKIPPED -- File has been marked']);
         end
         continue;
      end
   end

   if( bitand(trialdata.key.flags, CXHF_HASTAGSECTS) == 0 )             % this trial had no tagged sections
      if( verbose )
         disp([filename ' SKIPPED -- Has no tagged sections']);
      end
      continue;
   end

   nSects = length(trialdata.tagSections);
   if( verbose )
      disp([filename ' -- Processing ' int2str(nSects) ' tagged sections']);
   end

   for( n=1:nSects )                                                    % BEGIN: process each tagged section in trial
      section = trialdata.tagSections(n);
      if( section.tLen < 0 )                                            % skip it -- section not well-defined
         continue;
      end

      tBegin = double(section.tStart) + 1 - prolog;                     % compute time interval spanning section plus
      if( tBegin < 1 )
         if( verbose )
            warning( 'Skipped %s section in %s -- not all of section was recorded!', section.tag, filename );
         end
         continue;
      end
      tLen = double(section.tLen) + prolog + epilog;                    % prolog & epilog, in ms rel to record start
      tEnd = tBegin + tLen - 1;

      tRecordedLen = 0;                                                 % compute length of section actually recorded
      if( tEnd>0 & tBegin<trialdata.key.nScansSaved )
         tRecordedLen = min(tEnd, double(trialdata.key.nScansSaved)) - max(tBegin, 1) + 1;
      end

      if( tRecordedLen ~= tLen )                                        % if entire section not recorded (incl prolog
         if( verbose )                                                  % and epilog), it's considered invalid
            warning( 'Skipped %s section in %s -- not all of section was recorded!', section.tag, filename );
         end
         continue;
      end

      if( isempty(existingTags) )                                       % first valid section to be processed
         res(1).tag = section.tag;                                      % results structure
         res(1).dur = tLen;
         res(1).nreps = 1;
         existingTags = {res(1:length(res)).tag};
      else                                                              % else, have we already encountered section?
         index = strmatch(section.tag, existingTags, 'exact');
         if( isempty(index) )                                           % if not, append a new element to the results
            newindex = length(res) + 1;
            res(newindex).tag = section.tag;
            res(newindex).dur = tLen;
            res(newindex).nreps = 1;
            existingTags = {res(1:length(res)).tag};
         elseif( res(index).dur ~= tLen )                               % if we've seen section before, but this rep
            if( verbose )                                               % has a different duration -- INVALID!
               warning( 'Skipped %s section in %s -- duration %d should be %d', section.tag, filename, ...
                  tLen, res(index).dur );
            end
            continue;
         else                                                           % else, this is a valid rep of the section
            res(index).nreps = res(index).nreps + 1;
         end
      end

      index = strmatch(section.tag, existingTags, 'exact');             % BEGIN: Add recorded data for this valid
                                                                        % repetition of the section to output struct

      nch = double(trialdata.key.nchans);
      chanIndex = find(trialdata.key.chlist(1:nch)==CH_HGPOS);          % horizontal eye position
      if( ~isempty(chanIndex) )
         nextRowIndex = size(res(index).hgpos,1) + 1;
         res(index).hgpos(nextRowIndex,:) = trialdata.data(chanIndex,[tBegin:tEnd]) * POS_AIRAW2DEG;
      end

      chanIndex = find(trialdata.key.chlist(1:nch)==CH_VEPOS);          % vertical eye position
      if( ~isempty(chanIndex) )
         nextRowIndex = size(res(index).vepos,1) + 1;
         res(index).vepos(nextRowIndex,:) = trialdata.data(chanIndex,[tBegin:tEnd]) * POS_AIRAW2DEG;
      end

      chanIndex = find(trialdata.key.chlist(1:nch)==CH_HEVEL);          % horizontal eye velocity
      if( ~isempty(chanIndex) )
         nextRowIndex = size(res(index).hevel,1) + 1;
         res(index).hevel(nextRowIndex,:) = trialdata.data(chanIndex,[tBegin:tEnd]) * VEL_AIRAW2DPS;
      end

      chanIndex = find(trialdata.key.chlist(1:nch)==CH_VEVEL);          % vertical eye velocity
      if( ~isempty(chanIndex) )
         nextRowIndex = size(res(index).vevel,1) + 1;
         res(index).vevel(nextRowIndex,:) = trialdata.data(chanIndex,[tBegin:tEnd]) * VEL_AIRAW2DPS;
      end

      chanIndex = find(trialdata.key.chlist(1:nch)==CH_HDVEL);          % horizontal eye velocity (high-pass)
      if( ~isempty(chanIndex) )
         nextRowIndex = size(res(index).hdvel,1) + 1;
         res(index).hdvel(nextRowIndex,:) = trialdata.data(chanIndex,[tBegin:tEnd]) * VEL_AIRAW2DPS;
      end


      nreps = res(index).nreps;                                         % Maestro-recorded spikes on DI0
      if( nreps == 1 )
         if( sparseOut )
            res(index).unit0 = sparse(1,tLen);
         else
            res(index).unit0 = cell(1,1);
         end
      end
      if( isempty(trialdata.spikes) )
         hits = [];
      else
         spikes = trialdata.spikes - tBegin;
         hits = spikes( find(spikes>0 & spikes<tLen) );
      end
      if( sparseOut )
         if( isempty(hits) )
            res(index).unit0(nreps,:) = 0;
         else
            res(index).unit0(nreps,ceil(hits)) = 1;
         end
      else
         res(index).unit0{nreps,1} = hits;
      end

      if( nreps == 1 )                                                  % the sorted spike train channels
         res(index).sortedUnits = cell(numunits,1);
         for(u=1:numunits)
            if( sparseOut )
               res(index).sortedUnits{u} = sparse(1,tLen);
            else
               res(index).sortedUnits{u} = cell(1,1);
            end
         end
      end
 
      for(u=1:numunits) 
         spikes = trialdata.sortedSpikes{u};
         if( isempty(spikes) )
            hits = [];
         elseif( spikes(1) < 0 )                                        % negative spike time is special code from
            hits = [];                                                  % PLEXSYNCHRONIZE  -- ignored
         else
            spikes = spikes - tBegin;
            hits = spikes( find(spikes>0 & spikes<tLen) );
         end

         if( sparseOut )
            if( isempty(hits) )
               res(index).sortedUnits{u}(nreps,:) = 0;
            else
               res(index).sortedUnits{u}(nreps,ceil(hits)) = 1;
            end
         else
            res(index).sortedUnits{u}{nreps,1} = hits;
         end
      end

   end                                                                  % END: process each tagged section in trial
end
