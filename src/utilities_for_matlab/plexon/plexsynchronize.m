%
%          by: david schoppik
%        date: February 25, 2005
%     purpose: to write spike times to a set of files
%              that represent an experiment's worth of
%              trials
%       usage: result = plex_synchronize(experiment,directory,verbose)
%
%        reqs: 1. "experiment" must be a single letter
%              2. the script must be refer to a directory
%                 that contains the trial files in the format
%                 xxYYMMDD/xxYYMMDDe where "e" is the experiment2
%              3. Plexon data must be in the same directory, named
%                 xxYYMMDD.e.nex
%
%       notes: this program replaces "sychronize" for all rigs that use
%              the new device which sends character names as strobed words
%              to the Plexon

function [result,plexvals] = plexsynchronize(experiment,directory,verbose)
if nargin == 2
  verbose = 1;
end

if nargin == 1
  directory = pwd;
  verbose = 1;
end

if nargin < 1
  help plexsynchronize
  return
end

result = -1;
threshold = 1; % in ms

% set up the names
% modify if you want a different naming convention
trialbasename = [getlastdir(pwd) experiment];
plexonfilename = [getlastdir(pwd) '.' experiment '.nex'];

% get the number of trials in a particular experiment
% modify if you want a different naming convention
numtrials = 0;
files=dir(directory);
for i=1:length(files)
  if max(findstr(files(i).name,experiment) == 9) & files(i).name(2:3) ~= 'DS'
    numtrials = numtrials +1;
  end
end

if verbose
  disp(['Number of trials = ' num2str(numtrials)])
end

% get the spike times from the plexon file
[names, types] = nex_info(plexonfilename);
unitmarkers = find(~types);
numunits = length(unitmarkers);

if verbose
  disp(['Number of units: ',num2str(numunits)])
end

for i = 1:length(unitmarkers)
  spiketimesCell{i} = nex_ts(plexonfilename,names(unitmarkers(i),1:7));
  if verbose
    disp(['Number of spikes in unit ' num2str(i) ' = ' num2str(length(spiketimesCell{i}))])
  end
end



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% parse the characters that came in from Maestro %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% get the full data set of Mestro-passed values
[strobed_inputs ts] = nex_marker(plexonfilename,'Strobed');

% I'm not sure what the best way to do this is: the data is read in as
% chars; but the numbers passed are actually ASCII codes, so to convert them
% I just take what nex_marker gives me, scale and convert them
% appropriately.  This is really for legibility.

% 48 is ASCII 0 -- to convert this to the proper integers
for i = 0:9,strobed_inputs(find(strobed_inputs == 48+i)) = i; end
% scale them appropriately
strobed_inputs(:,3) = 100*strobed_inputs(:,3);
strobed_inputs(:,4) = 10*strobed_inputs(:,4);
% add them to get the actual values of the characters
strobed_inputs = sum(strobed_inputs(:,3:5),2);
% tack on the timestamps of the characters
strobed_inputs(:,2) = ts;
% this is the pulse that should come in on channel 2 from XS2/DO11

XS2pulses = nex_ts(plexonfilename,'Event002');
DO2pulses = nex_ts(plexonfilename,'Event004');

% find the trial start and stop values, discarding aborted trials.
% the trial_ts is a cell array, in case there are different length trial
% names.

startcode = 2; % Maestro "trial start"
stopcode = 3; % Maestro "trial stop"
abortcode = 15; % Maestro " trial stop"
lostfixcode = 14; % End of trial b/c monkey lost fix
nullcode = 0; % Maestro spacer between names
filesavedcode = 6; % Was there a saved file associated with this trial?

start = find(strobed_inputs(:,1) == startcode);
stop = find(strobed_inputs(:,1) == stopcode);

% since this is for synchronization purposes, we can get rid of all the
% trials where there wasn't an associated saved file....
% this should work for files using Plexon codes after 3-10-05, when Scott
% updated the trial codes

start = start(find(strobed_inputs(stop-1) == filesavedcode));
stop = stop(find(strobed_inputs(stop-1) == filesavedcode));
offset = [];
for i = 1:length(stop)
  strobed_inputs(start(i)+1:stop(i)-1);
  relevant_trialcodes = strobed_inputs(start(i)+1:stop(i)-1);
  nulls = find(relevant_trialcodes == nullcode);
  synch_data{i,1} = char(relevant_trialcodes(nulls(1)+1:nulls(2)));
  synch_data{i,2} = XS2pulses(find(XS2pulses > ...
    strobed_inputs(start(i),2) & ...
    XS2pulses < strobed_inputs(stop(i),2))); % the true beginning and end of each trial
  % occasionally, the XS2pulse will not come through.  In this case, we can
  % still go to the pulses on DO2 that should(!) mark the beginning and end
  % of every trial.  
  if length(synch_data{i,2}) ~= 2
    disp(['For trial ' synch_data{i,1} ' ' num2str(length(synch_data{i,2})) ' pulse(s) recorded'])
    synch_data{i,2} = DO2pulses(find(DO2pulses > ...
      strobed_inputs(start(i),2) & ...
      DO2pulses < strobed_inputs(stop(i),2)));
    
    if length(synch_data{i,2}) ~= 2
      disp('ERROR: Cannot find appropriate marker pulses')
    else
      if isempty(offset), offset = inputdlg('What is the length of your last segment (in seconds)?'); end
      synch_data{i,2}(2) = synch_data{i,2}(2) + str2num(offset{1});
    end
  end
end

if verbose
  disp(['Number of trials in the Plexon file is ' num2str(length(synch_data))])
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% put the spike times into the Maestro files %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% now put the spike times into the files
for i=1:length(synch_data)
  trialdata = readcxdata(synch_data{i,1});
  triallength = length(trialdata.data(1,:));
  % clear existing spike times -- this is just precautionary, to
  % avoid problems with multiple resortings of the spike files
  for k=1:13
    trialdata.sortedSpikes{k} = [];
  end

  % for legibility, let's do:
  trial_start = synch_data{i,2}(1);
  trial_end = synch_data{i,2}(2);
  
  
  if abs((1000*(trial_end-trial_start)) - length(trialdata.data)) > threshold
    disp([synch_data{i,1} ' has a temporal mismatch greater than ' num2str(threshold) ' ms'])
    disp(['it is ' num2str((1000*(trial_end-trial_start)) - length(trialdata.data)) 'ms'])
  end
      
  for j=1:numunits
    % the "raw" spiketimes are all the spikes that fall between the stop
    % and start
    rawspiketimes = spiketimesCell{j}(find(spiketimesCell{j} > trial_start  & ...
      spiketimesCell{j} < trial_end));
    trialdata.sortedSpikes{j} = double((rawspiketimes - trial_start) * 1000);
    if max(trialdata.sortedSpikes{j}) > length(trialdata.data)
      disp(['Spike time for trial ' num2str(i) ' exceeds trial' ...
        ' length by ' num2str(round(max(trialdata.sortedSpikes{j}) - length(trialdata.data))) ' ms'])
    end

    % we need to to some special things if the field is empty to
    % distinguish whether it is empty because the unit didn't
    % respond (we'll put a -1 in) or because the unit wasn't held
    % during the time of the trial (that's a -99)

    % if a neuron is lost and recaptured, this can be problematic.  but
    % that should really be recorded as a separate file anyway...
    
    if isempty(trialdata.sortedSpikes{j})
      if (trial_start > spiketimesCell{j}(1) && ...
          trial_start+triallength < spiketimesCell{j}(end))
        trialdata.sortedSpikes{j} = -1;
      else
        trialdata.sortedSpikes{j} = -99;
      end
    end

  end

  if editcxdata(synch_data{i,1},trialdata)
    disp(['Could not write to trial number ' num2str(i)])
    return;
  end
end

result = 0;