function res = findtrialchains(dirspec, base)
% Find all trial chains in a set of Maestro trial data files sequenced using the "chained" trial sequencer mode.
% FINDTRIALCHAINS(DIRSPEC, BASE) scans all Maestro data files with the given base name in the directory specified and 
% finds all "trial chains" among those files. A "chain" is a sequence of consecutive presentations of the same trial 
% during Maestro's "chained" sequencer mode. A header flag in the Maestro data file identifies a trial as part of a 
% chained sequence, and a second flag marks those files which record the first trial in a chain.
%
% This script uses the MEX function READCXDATA() to examine the header for each data file. If this MEX function is not
% available on your Matlab command path, the script will fail.
%
% NOTE: Maestro v3.1.0 had a "bug" which caused it to mark all "Chained" trials as successful, even if the trial file
% was not saved. As a result, if the subject failed the first trial in a new chain, the "chain start" flag would be
% missed, and FINDTRIALCHAINS would miss that chain. This bug was fixed in Maestro 3.1.1, but we also modified 
% FINDTRIALCHAINS so that it would detect a new chain even if the "chain start" flag was missing -- by checking for a
% change in the name of the trial presented (vs the previous trial).
%
% Arguments (all are required):
%   DIRSPEC: A MATLAB string specifying the pathname, relative to the current working directory, of the directory 
% containing the Maestro trial data files to be scanned. If DIRSPEC is an empty string, then the function will check all
% files in the current working directory that have base name specified.
%
%   BASE: A MATLAB string specifying the common base name of the trial data files to be scanned. During a typical 
% experiment, all the data files share a common base name and differ only in the four digit numeric extension: 
% "yo07oct2013a.0001, yo07oct2013a.0002, ...."
%
% Returns:
% A Nx2 cell matrix, where N is the # of distinct trials participating in the chained sequence. The first column holds 
% the trial name, while the second column holds a 1x2M numeric array listing the M trial chains found for that trial. 
% Each pair of numbers (EXT, LEN) in this array indicate the numeric extension of the data file for the first trial in a
% chain, followed by the chain's length LEN. Of course, a chain of length L also includes shorter chains of length 1, 
% 2, ... , L-1.
%
% Any file satisfying the path specification will be ignored if it is NOT a Maestro trial data file or if is not part of
% a chained trial sequence.
%
% saruffner, 07oct2013

% some Maestro header record flags
CXHF_ISCONTINUOUS = bitshift(1,0); 
THF_CHAINED = bitshift(1,20);
THF_CHAINSTART = bitshift(1,21);

% do we have the required # of input arguments?
if( nargin < 2 )
   error('Missing one or both input arguments!');
end;

% make sure DIRSPEC identifies an existing directory
if(~isdir(dirspec))
   error('Arg DIRSPEC does not identify as existing directory!');
end;

% make sure BASE is a non-empty character string
if(isempty(base) || ~ischar(base))
   error('Arg BASE must be a non-empty character string!');
end;

% initially, output is a 0x2 cell array -- in case we find no chains!
res = cell(0,2);

% get the directory's file listing and count how many files we need to 
% examine. The file name starts with BASE, followed by '.' and a 4-digit
% numeric extension. If there are no such files, we're done!
files = dir(dirspec);
if(isempty(files))
   return;
end;

basePlusDot = strcat(base, '.');
fileCount = 0;
for i=1:numel(files)
   if(files(i).isdir == 0)
      idx = strfind(files(i).name, basePlusDot);
      if((~isempty(idx)) && isscalar(idx) && (idx == 1))
         fileCount = fileCount + 1;
      end;
   end;
end;
if(fileCount == 0)
   fprintf('Found no files in %s starting with %s', dirspec, basePlusDot);
   return;
end;

% process files 'base.nnnn' until we're done, accumulating chains as we go...
chainTrialName = '';
chainStartIdx = 0;   % if > 0, then this is index of first trial in a chain in progress
chainLen = 0;

% a temporary structure for accumulating chains
chains = struct('name', {}, 'n', {}, 'list', {});

i = 0;
while((i < 10000) && (i < fileCount))
   fname = fullfile(dirspec, sprintf('%s.%04d', base, i+1));
   i = i+1;

   isChained = 0;
   isChainStart = 0;
   if(exist(fname, 'file'))
      fData = readcxdata(fname);
      if(~isempty(fData.key))
         isChained = (bitand(fData.key.flags, CXHF_ISCONTINUOUS) == 0);
         isChained = isChained && (bitand(fData.key.dwTrialFlags, THF_CHAINED) ~= 0);
         isChained = isChained && ischar(fData.trialname) && ~isempty(fData.trialname);
         isChainStart = isChained && (bitand(fData.key.dwTrialFlags, THF_CHAINSTART) ~= 0);

         % NOTE: This handles the possibility that the trial identity has changed, yet the THF_CHAINSTART flag is
         % missing. For trials recorded under Maestro v3.1.0, this could happen due to a bug. Bug was fixed in v3.1.1.
         if(isChained && (~isChainStart))
            isChainStart = ~strcmp(chainTrialName, fData.trialname);
         end;
      end;
   end;
   
   if(chainStartIdx > 0)
      % note the special conditions for ending the current chain in progress. The most common reason to end a chain is 
      % the start of a new chain!
      isChainEnd = isChainStart || (~isChained) || (~strcmp(chainTrialName, fData.trialname));
     
      % if we're at chain's end, store file index (the numeric extension) and the length of the chain; otherwise, 
      % increment the current chain's length
      if(isChainEnd)
         if(isempty(chains))
            chains(1).name = chainTrialName;
            chains(1).n = 1;
            chains(1).list = zeros(1,100);
            chains(1).list(1) = chainStartIdx;
            chains(1).list(2) = chainLen;
         else
            idx = 0;
            for j=1:length(chains)
               if(strcmp(chains(j).name, chainTrialName))
                  idx = j;
                  break;
               end;
            end;
            if(idx == 0) 
               idx = length(chains) + 1;
               chains(idx).name = chainTrialName;
               chains(idx).n = 1;
               chains(idx).list = zeros(1,100);
               chains(idx).list(1) = chainStartIdx;
               chains(idx).list(2) = chainLen;
            else
               n = chains(idx).n;
               chains(idx).list(2*n + 1) = chainStartIdx;
               chains(idx).list(2*n + 2) = chainLen;
               chains(idx).n = n + 1;
               if(2*chains(idx).n == length(chains(idx).list))
                  chains(idx).list = cat(2, chains(idx).list, zeros(1,100));
               end;
            end;
         end;

         chainTrialName = '';
         chainStartIdx = 0;
         chainLen = 0;
      else
         chainLen = chainLen + 1;
      end;
   end;

   % if a new chain is starting, remember the trial name and the file index of the first trial in the chain.
   if(isChainStart)
      chainStartIdx = i;
      chainLen = 1;
      chainTrialName = fData.trialname;
   end;
end;

% make sure you account for the last chain that terminated with the last trial data file processed
if(chainStartIdx > 0)
   if(isempty(chains))
      chains(1).name = chainTrialName;
      chains(1).n = 1;
      chains(1).list = zeros(1,100);
      chains(1).list(1) = chainStartIdx;
      chains(1).list(2) = chainLen;
   else
      idx = 0;
      for j=1:length(chains)
         if(strcmp(chains(j).name, chainTrialName))
            idx = j;
            break;
         end;
      end;
      if(idx == 0) 
         idx = length(chains) + 1;
         chains(idx).name = chainTrialName;
         chains(idx).n = 1;
         chains(idx).list = zeros(1,100);
         chains(idx).list(1) = chainStartIdx;
         chains(idx).list(2) = chainLen;
      else
         n = chains(idx).n;
         chains(idx).list(2*n + 1) = chainStartIdx;
         chains(idx).list(2*n + 2) = chainLen;
         chains(idx).n = n + 1;
         if(2*chains(idx).n == length(chains(idx).list))
            chains(idx).list = cat(2, chains(idx).list, zeros(1,100));
         end;
      end;
   end;
end;

% prepare cell array containing the trial chains found
res = cell(length(chains), 2);
for i=1:length(chains)
   res{i,1} = chains(i).name;
   n = chains(i).n;
   res{i,2} = chains(i).list(1:2*n);
end;

