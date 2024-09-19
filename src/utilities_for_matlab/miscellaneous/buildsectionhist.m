function res = buildsectionhist(pathspec, trialnames, params)
% Construct spike time histograms for tagged sections culled from a set of Maestro trial data files.
% BUILDSECTIONHIST(PATHSPEC, TRIALNAMES, PARAMS) culls unit response data 
% (ie, "spike events" on digital input channel 0) for any tagged sections 
% defined in a set of Maestro trial data files, then prepares spike time 
% histograms for each tagged section found.  It uses the MEX function 
% READCXDATA() to read in the contents of each file satisfying the path 
% specification.
%
% Arguments (all are required):
%   PATHSPEC    A MATLAB string specifying a pathname, relative to the current 
%               working directory, that identifies one or more Maestro data 
%               files.  Typically, the '*' wildcard will be used to specify a 
%               bunch of files to be examined.  Note that buildsectionhist() 
%               will call the MATLAB function dir() with this argument to 
%               obtain a list of files to process.  If PATHSPEC is an empty 
%               string, then the function will check all files in the current 
%               working directory.
%
%   TRIALNAMES  A MATLAB cell array of strings, eg: {'trial1'; 'trial2'}.  This 
%               argument provides a means of narrowing the set of trial data 
%               files from which histogram data is collected.  If it is not 
%               empty, the function will only include those data files that 
%               satisfy the path specification AND contain data for a Maestro 
%               trial that is named in this cell array.
%
%   PARAMS      A 1x5 MATLAB double array [BIN PROLOG EPILOG PLOT? VERBOSE?] 
%               that specifies all other parameters for the function.
%
%               The first three values set the bin size, prolog length, and 
%               epilog length for each tagged section histogram constructed 
%               by the function.  All are specified in milliseconds.  Bin 
%               size is restricted to [1..100] ms.  The prolog and epilog 
%               refer to portions of the data immediately before and after 
%               a tagged section that are included in the section histogram; 
%               they are restricted to the range [0..500] ms.
%
%               If some histograms were successfully constructed and the PLOT?
%               flag is nonzero, then a plot is made of the results.  If the 
%               VERBOSE? flag is nonzero, the function will print detailed 
%               progress messages to the console (for debug purposes). 
%
% Returns:
% A 1xM MATLAB array of structures, where M is the # of unique tagged sections 
% culled from the data files.  Each structure includes three fields: 
%
%   tag        A MATLAB string holding the section name.
%   hist       A Nx1 double matrix representing the histogram constructed for 
%              that section.  Histogram data are in Hz (spikes per second). 
%   nreps      A 1x2 array listing the minimum and maximum #samples obtained 
%              across all bins of the histogra.  If min(nreps) != max(nreps), 
%              then some bins were undersampled.
%
% Any file satisfying the path specification will be ignored by BUILDSECTIONHIST 
% if it is NOT a Maestro trial data file, if the corresponding trial name is not 
% found among the elements of arg TRIALNAMES, if it contains no well-defined 
% tagged sections, or if it has been "marked" (ie, the field "marked" is nonzero
% in the structure returned by READCXDATA()), it is ignored.  A tagged section
% is "well-defined" only if READCXDATA() is able to determine its duration and 
% start time relative to when Maestro's event timer began timestamping events 
% during the trial.  For example, trials that include a "skip on saccade"
% operation are ignored because READCXDATA() cannot reliably determine the start
% time of any tagged section after the "skip" occurs.
%
% The INTENT behind the tagged sections feature in Maestro is that each distinct
% tagged section in a trial set will have the same duration in any trial in
% which it appears, and that the recorded data will cover the entire section as 
% well as the prolog and epilog.  In this case, accumulating the results across
% any number of trial reps is straightforward.  However, Maestro cannot enforce
% this intent; the user can easily create a set of trials in which a named
% section has different lengths in different trials.  In addition, if the user
% chooses a prolog and epilog that extend beyond the recorded portion of a trial,
% or if in some trials a tagged section begins before data recording is enabled,
% then some bins in the histogram will be undersampled because nothing was
% recorded in those bins for some fraction (or all) of the instances of the 
% tagged section culled from the data files.  For these reasons, as it
% accumulates histogram data, BUILDSECTIONHIST keeps track of how many samples
% are accumulated in each bin.  The total spike count in each bin is then
% normalized by the #samples obtained for THAT bin, NOT by the # of times the 
% tagged section was presented across all data files examined.  The NREPS field
% can be checked to see if any portion of a histogram was undersampled: if 
% min(nreps) does not equal max(nreps), then undersampling occurred.

% some Maestro-related constants...
CXHF_ISCONTINUOUS = bitshift(1,0);                       % header record flags
CXHF_HASTAGSECTS = bitshift(1,7);

% do we have the required # of input arguments and output arguments?
if( nargin ~= 3 | nargout ~= 1 )
   error( 'BUILDSECTIONHIST requires 3 inputs and 1 output!' );
end

% make sure PATHSPEC arg is a string vector
bOk = ischar(pathspec) & (ndims(pathspec)==2);
temp = size(pathspec);
if( bOk )
   bOk = (temp(1)==1 | temp(2)==1);
end
if( ~bOk )
   error( 'Arg PATHSPEC must be a string vector!' );
end

% make sure TRIALNAMES arg is a 1xM or Mx1 cell array of string vectors only
if( ~isempty(trialnames) )
   bOk = iscell(trialnames) & (ndims(trialnames)==2);
   temp = size(trialnames);
   if( bOk )
      bOk = (temp(1)==1 | temp(2)==1);
   end
   if( bOk )
      for( m=1:length(trialnames) )
         name = trialnames{m};
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
      error( 'Arg TRIALNAMES must be a cell vector of string vectors!' );
   end
end


% check PARAMS argument.  Range limit bin size, prolog, and epilog.
bOk = isreal(params) & (ndims(params)==2);
temp=size(params);
if( bOk ) 
   bOk = (length(params)==5) & (temp(1)==1 | temp(2)==1);
end
if( ~bOk )
   error( 'Arg PARAMS must be real-valued double vector of length 5!' );
end

bin = round(params(1));
if( bin < 1 ) 
   bin = 1;
elseif( bin > 100 )
   bin = 100;
end

prolog = round(params(2));
if( prolog < 0 )
   prolog = 0;
elseif( prolog > 500 )
   prolog = 500;
end

epilog = round(params(3));
if( epilog < 0 )
   epilog = 0;
elseif( epilog > 500 )
   epilog = 500;
end

bPlot = (params(4) ~= 0);
bVerbose = (params(5) ~= 0);


% extract directory (path relative to current working dir) from path spec, and get the list of files that satisfy the 
% path spec.  If there are no files to process, abort.  In this case, the results structure is empty.
fileDir = fileparts(pathspec);
files = dir(pathspec);
if( isempty(files) )
   if( bVerbose ) 
      disp( 'WARNING: Found no files satisfying path specification!' );
   end
   res = struct('tag', {}, 'hist', {}, 'nreps', {});
   return;
end

% process each file in our file list...
histdata = struct([]);                                                  % histogram data accumulated here
existingTags = {};                                                      % tags of sections encountered thus far

for( m=1:numel(files) )
   if( files(m).isdir ~= 0 )                                            % skip directories!
      continue;
   end

   filename = fullfile(fileDir, files(m).name);                         % attempt to read file as a Maestro data file 
   trialdata = readcxdata(filename);
   if( isempty(trialdata.key) )                                         % file was probably NOT a Maestro file!
      if( bVerbose ) 
         disp([filename ' SKIPPED -- NOT a Maestro file']);
      end
      continue;
   end

   if( bitand(trialdata.key.flags, CXHF_ISCONTINUOUS) ~= 0 )            % we ignore Continuous-mode Maestro files!
      if( bVerbose ) 
         disp([filename ' SKIPPED -- Continuous-mode file']);
      end
      continue;
   end

   if( ~isempty(trialnames) )                                           % if trial names were supplied, ignore trial if 
      if( isempty(strmatch(trialdata.trialname, trialnames, 'exact')) ) % its name is not in the supplied list.
         if( bVerbose )
            disp([filename ' SKIPPED -- not one of named trials']);
         end;
         continue;
      end
   end

   marked = trialdata.marked;                                           % we ignore files that have been "marked"
   if( (~isempty(marked)) & marked(1) ~= 0 )
      if( bVerbose ) 
         disp([filename ' SKIPPED -- File has been marked']);
      end
      continue;
   end

   if( bitand(trialdata.key.flags, CXHF_HASTAGSECTS) == 0 )             % this trial had no tagged sections
      if( bVerbose ) 
         disp([filename ' SKIPPED -- Has no tagged sections']);
      end
      continue;
   end

   spikes = trialdata.spikes;                                           % spike arrival times recorded for this trial

   nSects = length(trialdata.tagSections);
   if( bVerbose )
      disp([filename ' -- Processing ' int2str(nSects) ' tagged sections']);
   end

   for( n=1:nSects )                                                    % BEGIN: process each tagged section in trial
      section = trialdata.tagSections(n);
      if( section.tLen < 0 )                                            % skip it -- section not well-defined
         continue;
      end

      tBegin = double(section.tStart) - prolog;                         % compute time interval spanning section plus
      tLen = double(section.tLen) + prolog + epilog;                    % prolog & epilog, in ms rel to record start
      tEnd = tBegin + tLen;                            

      nBins = floor(tLen/bin);                                          % number of COMPLETE bins spanning the interval 
      tLen = bin*nBins;                                                 % section length adjusted to integer # of bins
      tEnd = tBegin + tLen;

      tRecordedLen = 0;                                                 % compute length of section actually recorded
      if( tEnd>0 & tBegin<trialdata.key.nScansSaved )
         tRecordedLen = min(tEnd, double(trialdata.key.nScansSaved)) - max(tBegin, 0);
      end
      
      if( tRecordedLen < bin )                                          % did not record any COMPLETE bins in section
         continue;
      end

      if( isempty(histdata) )                                           % adding first element to our intermediate  
         histdata(1).tag = section.tag;                                 % results structure
         histdata(1).bins = zeros(1,nBins);
         histdata(1).nsamples = zeros(1,nBins);
         existingTags = {histdata(1:length(histdata)).tag}; 
      else                                                              % else, see if we've already encounted section 
         index = strmatch(section.tag, existingTags, 'exact');
         if( isempty(index) )                                           % if not, append a new element to intermediate 
            newindex = length(histdata) + 1;                            % results to hold results for the section
            histdata(newindex).tag = section.tag;
            histdata(newindex).bins = zeros(1,nBins);
            histdata(newindex).nsamples = zeros(1,nBins);
            existingTags = {histdata(1:length(histdata)).tag};
         elseif( length(histdata(index).bins) < nBins )                 % if so, grow bin buffer if nec -- a section 
            currLen = length(histdata(index).bins);                     % could have different lengths in diff trials! 
            extra = nBins-currLen;
            histdata(index).bins(currLen+1:nBins) = zeros(1,extra);
            histdata(index).nsamples(currLen+1:nBins) = zeros(1,extra);
         end
      end

      iFirstBin = 1;                                                    % determine range of COMPLETE bins in section 
      if( tBegin < 0 )                                                  % that were recorded during trial
         iFirstBin = ceil(abs(tBegin)/bin);
      end

      nRecordedBins = floor(tRecordedLen/bin);
      iLastBin = iFirstBin + nRecordedBins - 1;
      
      t0 = bin*(iFirstBin-1);                                           % time interval covered by this range of bins, 
      t1 = bin*(iLastBin);                                              % in ms RELATIVE TO START OF SECTION

      index = strmatch(section.tag, existingTags, 'exact');             % incr #samples in each bin COMPLETELY covered 
                                                                        % by recorded portion of trial
      histdata(index).nsamples(iFirstBin:iLastBin) = histdata(index).nsamples(iFirstBin:iLastBin) + 1;

      if( isempty(spikes) )                                             % there were no spikes recorded, so move on 
         continue;
      end
      spks = spikes - tBegin;                                           % spike times RELATIVE TO START OF SECTION
      hits = spks( find(spks>=t0 & spks<t1) );                          % spikes in recorded portion of section  
      hits = floor(hits/bin) + 1;                                       % corres bin #s -- note that the same bin 
                                                                        % could appear more than once here!
      for( p=1:length(hits) )                                           % increment the affected bins 
         histdata(index).bins(hits(p)) = histdata(index).bins(hits(p)) + 1;
      end

   end                                                                  % END: process each tagged section in trial
end


% prepare the output structure...
if( isempty(histdata) )
   res = struct('tag', {}, 'hist', {}, 'nreps', {});
else
   for( m=1:length(histdata) )
      res(m).tag = histdata(m).tag;

      nreps = histdata(m).nsamples;                                     % normalize each bin by #times that bin was 
      noSampIndices = find(nreps==0);                                   % recorded across all files processed -- and 
      nreps(noSampIndices) = nreps(noSampIndices) + 1;                  % avoid divide-by-0 error!!
      res(m).hist = histdata(m).bins ./ nreps;
      res(m).hist = res(m).hist / (bin*0.001);                          % convert histogram units to Hz

      res(m).nreps = [min(histdata(m).nsamples) max(histdata(m).nsamples)];
   end
end

clear histdata;                                                         % we're done with intermediate results

% if plot flag set, prepare a figure that plots the first 32 histograms
if( bPlot & ~isempty(res) )
   if( bVerbose ) 
      disp( 'Preparing figure showing all histograms...' );
   end

   nHists = min(32, length(res));                                       % we'll plot up to 32 section histograms
   nCols = floor((nHists-1)/8) + 1;                                     % arranged in columns of 8 rows each
   nRows = min(nHists, 8);

   maxHz = 0;                                                           % find max bin value over all histograms
   for( m=1:nHists )                                                    % plotted.  All y-axes will range from 0 to 
      maxHz = max(maxHz, max(res(m).hist) );                            % this max value.
   end

   maxNBins = 0;                                                        % find max #bins over all histograms plotted. 
   for( m=1:nHists )                                                    % All x-axes will range from 0 to this value. 
      maxNBins = max(maxNBins, length(res(m).hist));
   end

   theFig = figure;                                                     % create new figure and make it invisible 
   set(theFig, 'Visible', 'off');                                       % while we populate it w/ histograms

   for( m=1:nHists )
      subplot(nRows,nCols,m);
      title( res(m).tag );
      axis([0 maxNBins 0 maxHz]);                                       % all axes are scaled the same
      hold on;
      bar( res(m).hist );
      hold off;
   end

   figTitle= ['Histograms in Hz, bin=' int2str(bin) 'ms'];              % set figure title and make fig visible
   set(theFig, 'Name', figTitle);
   set(theFig, 'Visible', 'on');
end


