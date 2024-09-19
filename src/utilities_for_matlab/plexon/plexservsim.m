function [n, ts] = plexservsim( request )
% PLEXSERVSIM Generate a simulated PLEXON event stream for testing the PLEXMON GUI.
% 
%  PLEXMON is a Matlab GUI that monitors the event stream from the Plexon MAP server as 
%  trials are run in Maestro.  When it has collected a trial's worth of data, it invokes 
%  a trial handler function (specified by the user) that processes the trial data 
%  supplied by PLEXMON.
%
%  PLEXSERVSIM was created to provide simulated output from the Plexon to debug/test 
%  the PLEXMON user interface as well as any trial handler that might be developed for 
%  use with PLEXMON -- particularly a handler that processes Maestro tagged sections.
%  It simulates the repeated presentation of 8 different Maestro trials containing 4 
%  of a total possible 8 tagged sections labelled 'a_sect' thru 'h_sect'.  Each trial 
%  is 1800ms long and starts with a 200ms untagged segment, followed by each of 4 
%  different tagged sections with intervening 100ms untagged segments, and a 100ms 
%  untagged segment at the end.  PLEXSERVSIM includes all the character data and sync 
%  pulses sent to the Plexon by Maestro during a trial: 'startCode', 'trial name', 
%  'trial data file', startSync(XS2), ..., stopSync(XS2), [resultCodes], 'stopCode'.
%  See the Maestro User's Guide for a complete discussion of the information Maestro 
%  sends to the Plexon.
%
%  Between the startSync and stopSync pulses on the Plexon's XS2 input, PLEXSERVSIM 
%  will generate artificial spike trains for 4 different "neural units".  The firing 
%  rate of each unit is set to a baseline rate during untagged portions of a 
%  simulated trial, and it is modulated in different ways during different tagged 
%  sections -- to simulate the unit's responses to different stimuli.  In addition, 
%  PLEXSERVSIM will post an event on Plexon TTL input channel 3 at the start of each 
%  'c_sect' tagged section, and on channel 4 at the start of each 'h_sect'.
%
%  PLEXSERVSIM is designed to be used with Maestro running and saving trial data. 
%  Maestro must be set up to run the following 8 trials in 'Ordered NOFIX' mode:
%     trial1 :  a_sect h_sect e_sect c_sect
%     trial2 :  d_sect f_sect b_sect g_sect
%     trial3 :  c_sect g_sect h_sect d_sect
%     trial4 :  b_sect a_sect f_sect e_sect
%     trial5 :  e_sect c_sect a_sect f_sect
%     trial6 :  g_sect d_sect b_sect h_sect
%     trial7 :  f_sect a_sect e_sect h_sect
%     trial8 :  d_sect g_sect c_sect b_sect
%  The trials must be given the names shown, contain the tagged sections in the order 
%  listed, and have exactly the structure described earlier.  Otherwise, what the 
%  trials actually DO matters not!  Also, when running the trials, save the data 
%  files to plextest.0001, plextest.0002, and so on.  These are the file names 
%  provided by PLEXSERVSIM, so PLEXMON will look for those files -- just be sure that 
%  PLEXMON's 'Maestro Dir" is set correctly. 
%
%  The simulation's state is maintained in a global variable called 'pss_glob'.  Do 
%  NOT mess with this variable!
%
%  PLEXSERVSIM('start') starts or restarts the simulation from scratch.  The next 
%  "trial" presented will be 'trial1', and the corresponding data file will be
%  'plextest.0001'.  No timestamp data returned.
%
%  PLEXSERVSIM('stop') stops the simulation.  The global pss_glob is cleared from 
%  the Matlab workspace.  No timestamp data returned.
%
%  PLEXSERVSIM('poll') polls the simulation for more Plexon event timestamp data. 
%  The simulator will only provide data every 0.5 sec approximately.  If you call it 
%  more often than that, you will get nothing back. 
%
% Arguments: 
%  request     Request ID: 'start', 'stop', or 'poll'.
%
% Returns:
%  n           Scalar number of timestamps returned.
%  ts          nx4 timestamp matrix as would be returned by the Plexon MAP server.
%
% See Also:  PLEXMON, PLEXSECTHIST.

% Here's the global structure in which we maintain the simulator's state.  If it does not yet exist, it will be 
% initialized to an empty matrix.
global pss_glob;

% Initialize return variables: no timestamps returned.
n = 0;
ts = zeros(0,4);

% Respond to 'stop' request:  clear simulation data from workspace and return.
if( strcmp('stop', request) )
   disp('PLEXSERVSIM:  Plexon simulator stopped.');
   clear global pss_glob;
   return;
end

% Respond to 'start' request: reset simulator, set 'Plexon time' to 0, and begin simulation.
if( strcmp('start', request) )
   disp('PLEXSERVSIM:  Plexon simulator started.');

   % Get rid of any old stuff and initialize simulator state
   pss_glob = [];
   pss_glob.lastPoll = clock;
   pss_glob.nTrials = 0;  
   pss_glob.plexTime = 0;
   pss_glob.trialTime = 0;

   % This 4x2 matrix holds the channel# and unit# (in cols 1 and 2, resp) of the Plexon "units" we're simulating.
   pss_glob.unitIDs = [1 3; 2 1; 2 2; 3 1];

   % For each "unit", define baseline probability of firing, along with a modulated firing probability during each of the 
   % eight different 300ms tagged sections.
   t = [0:299];
   pss_glob.unit(1).baseRate = 0.1;
   pss_glob.unit(1).sectRate(1,:) = [ones(1,50)*0.1 (0.7-t(1:250)/500)];
   pss_glob.unit(1).sectRate(2,:) = [(0.1-t(1:50)/500) zeros(1,250)];
   pss_glob.unit(1).sectRate(3,:) = ones(1,300)*0.1;
   pss_glob.unit(1).sectRate(4,:) = [(0.1+t(1:50)/500) ones(1,250)*0.2];
   pss_glob.unit(1).sectRate(5,:) = [(0.2-t(1:150)/750) zeros(1,150)];
   pss_glob.unit(1).sectRate(6,:) = [(0.2+t(1:150)/215) ones(1,150)*0.9];
   pss_glob.unit(1).sectRate(7,:) = [(0.2+t(1:150)/500) ones(1,150)*0.5];
   pss_glob.unit(1).sectRate(8,:) = ones(1,300)*0.1;

   pss_glob.unit(2).baseRate = 0.3;
   pss_glob.unit(2).sectRate(1,:) = [ones(1,50)*0.1 (0.1 + t(1:250)/500)];
   pss_glob.unit(2).sectRate(2,:) = 0.3 - t/1000;
   pss_glob.unit(2).sectRate(3,:) = ones(1,300)*0.35;
   pss_glob.unit(2).sectRate(4,:) = ones(1,300)*0.35;
   pss_glob.unit(2).sectRate(5,:) = ones(1,300)*0.35;
   pss_glob.unit(2).sectRate(6,:) = 0.3 + t/500;
   pss_glob.unit(2).sectRate(7,:) = [(0.3 + t(1:150)/300) (0.8-t(1:150)/300)];
   pss_glob.unit(2).sectRate(8,:) = [(0.3 + t(1:100)/180) (0.8-t(1:200)/300)];

   pss_glob.unit(3).baseRate = 0.05;
   pss_glob.unit(3).sectRate(1,:) = ones(1,300)*0.05;
   pss_glob.unit(3).sectRate(2,:) = ones(1,300)*0.05;
   pss_glob.unit(3).sectRate(3,:) = 0.05 + t/450;
   pss_glob.unit(3).sectRate(4,:) = [(0.05 + t(1:150)/600) ones(1,150)*0.3];
   pss_glob.unit(3).sectRate(5,:) = ones(1,300)*0.05;
   pss_glob.unit(3).sectRate(6,:) = [(0.05 + t(1:50)/200) (0.3+t(1:250)/750)];
   pss_glob.unit(3).sectRate(7,:) = 0.1 + t/400;
   pss_glob.unit(3).sectRate(8,:) = ones(1,300)*0.05;

   pss_glob.unit(4).baseRate = 0.6;
   pss_glob.unit(4).sectRate(1,:) = ones(1,300)*0.6;
   pss_glob.unit(4).sectRate(2,:) = ones(1,300)*0.55;
   pss_glob.unit(4).sectRate(3,:) = 0.6 + t/1500;
   pss_glob.unit(4).sectRate(4,:) = 0.6 - t/1200;
   pss_glob.unit(4).sectRate(5,:) = [(0.6 - t(1:100)/200) (0.1+t(1:200)/1000)];
   pss_glob.unit(4).sectRate(6,:) = [(0.6 - t(1:50)/100) (0.1+t(1:250)/1400)];
   pss_glob.unit(4).sectRate(7,:) = ones(1,300)*0.65;
   pss_glob.unit(4).sectRate(8,:) = [(0.6 - t(1:150)/500) (0.3+t(1:150)/500)];

   % Order of tagged sections in each of the 8 simulated trials.  Four sections per trial.
   pss_glob.trialSects(1,:) = [1 8 5 3];        % a_sect h_sect e_sect c_sect
   pss_glob.trialSects(2,:) = [4 6 2 7];        % d_sect f_sect b_sect g_sect
   pss_glob.trialSects(3,:) = [3 7 8 4];        % c_sect g_sect h_sect d_sect
   pss_glob.trialSects(4,:) = [2 1 6 5];        % b_sect a_sect f_sect e_sect
   pss_glob.trialSects(5,:) = [5 3 1 6];        % e_sect c_sect a_sect f_sect
   pss_glob.trialSects(6,:) = [7 4 2 8];        % g_sect d_sect b_sect h_sect
   pss_glob.trialSects(7,:) = [6 1 5 8];        % f_sect a_sect e_sect h_sect
   pss_glob.trialSects(8,:) = [4 7 3 2];        % d_sect g_sect c_sect b_sect

   % Prepare the first 2 seconds of timestamps: an 1800ms trial w/100ms on either side
   pss_glob.timestamps = zeros(0,4);
   prepareTimestamps;
   return;
end


% If we get here, request ID should be 'poll' and our global state should be initialized. If not, report error.
if( ~strcmp('poll', request) )
   disp('PLEXSERVSIM: Unrecognized request!');
   return;
end
if( isempty(pss_glob) )
   disp('PLEXSERVSIM: Simulator state uninitialized? Call with ''start'' request first.');
   return;
end

% We return the next 0.5 seconds' worth of timestamps IF 0.5 seconds have elapsed since we were last polled.  If not, 
% return an empty timestamp matrix (initialized above).
pollTime = clock;
if( etime(pollTime, pss_glob.lastPoll) < 0.5 )
   return;
end
pss_glob.lastPoll = pollTime;

currT = pss_glob.plexTime;
indices = find( (pss_glob.timestamps(:,4) >= currT) & (pss_glob.timestamps(:,4) < currT+0.5) );
if( ~isempty(indices) )
   n = length(indices);
   ts = pss_glob.timestamps(indices,:);
end

% Advance timeline by 0.5 seconds.
pss_glob.plexTime = pss_glob.plexTime + 0.5;
pss_glob.trialTime = pss_glob.trialTime + 0.5;

% If we've reached the end of the current trial, it's time to prepare timestamps for the next one.
if( pss_glob.trialTime >= 2 )
   pss_glob.trialTime = 0;
   pss_glob.nTrials = pss_glob.nTrials + 1;
   if( pss_glob.nTrials >= 9999 )                     % start over when data file ext reaches 9999
      pss_glob.nTrials = 0;
   end
   prepareTimestamps;
end



function prepareTimestamps()
% Private function that generates the Plexon Nx4 timestamp matrix for the next 1800ms trial in the simulation, plus 
% 100ms of intertrial time on either side.
%
% The four columns in the matrix are, in order: type (1=unit, 4=ext event), channel#, unit#, and timestamp in seconds.
% For strobed character events, channel#=257 and unit# is the ASCII character.  XS2 sync pulses are on channel# 2.

global pss_glob;

% If our global state is empty, something bad has happened!
if( isempty(pss_glob) )
   disp('PLEXSERVSIM:  Internal error! Simulation state lost.');
   return;
end

% Which trial are we running next?  Prepare name of trial and data file name accordingly.
trialNum = mod(pss_glob.nTrials, 8) + 1;
trialName = sprintf('trial%d', trialNum);
trialFile = sprintf('plextest.%04d', pss_glob.nTrials + 1);       % data file ext starts at .0001, not .0000!

% Prepare character stream timestamps: start, trial name, trial file, dataSaved, stop.
pss_glob.timestamps = zeros(0,4);
pss_glob.timestamps(1,:) = [4 257 2 pss_glob.plexTime + 0.05];    % startCode
pss_glob.timestamps(2,:) = [4 257 6 pss_glob.plexTime + 1.95];    % dataFileSavedCode
pss_glob.timestamps(3,:) = [4 257 3 pss_glob.plexTime + 1.96];    % stopCode 

trialInfo = [double(trialName) 0 double(trialFile) 0];            % trial name and filename converted to ASCII numeric 
trialInfo = trialInfo';                                           % codes, with terminating nulls.
n = length(trialInfo);
infoTimes = [1:n]' * 0.00025 + 0.05 + pss_glob.plexTime;          % trial name and filename chars follow startCode
tstamps = horzcat(ones(n,1)*4, ones(n,1)*257, trialInfo, infoTimes);
pss_glob.timestamps = vertcat(pss_glob.timestamps, tstamps);

% Prepare the two XS2 sync pulse timestamps that bracket the simulated Maestro trial timeline of 1800ms.
pss_glob.timestamps(end+1,:) = [4 2 0 pss_glob.plexTime + 0.1];
pss_glob.timestamps(end+1,:) = [4 2 0 pss_glob.plexTime + 1.9];

% If trial contains a 'c_sect' tagged section, deliver a timestamp on Plexon channel 3 at the start of that section.
sectIndex = find(pss_glob.trialSects(trialNum,:) == 3);
if( ~isempty(sectIndex) )
   tSectMS = 200 + (sectIndex(1)-1)*400;
   pss_glob.timestamps(end+1,:) = [4 3 0 (pss_glob.plexTime + 0.1 + tSectMS/1000)];
end

% If trial contains a 'h_sect' tagged section, deliver a timestamp on Plexon channel 4 at the start of that section.
sectIndex = find(pss_glob.trialSects(trialNum,:) == 8);
if( ~isempty(sectIndex) )
   tSectMS = 200 + (sectIndex(1)-1)*400;
   pss_glob.timestamps(end+1,:) = [4 4 0 (pss_glob.plexTime + 0.1 + tSectMS/1000)];
end

% Now add timestamps for any spikes occurring on any of the 4 simulated units.
for( i=1:4 )
   % First, calculate probability of firing during each millisec of the 2sec timeline.  Outside tagged sections, the 
   % probability of firing is set to the baseline probability.
   unitRate = ones(1,2000)*pss_glob.unit(i).baseRate;
   for( j=1:4 )
      sectNum = pss_glob.trialSects(trialNum,j);
      tSectStart = 100 + 200 + (j-1)*400 + 1;
      unitRate(tSectStart:tSectStart+299) = pss_glob.unit(i).sectRate(sectNum,:);
   end

   % Now decide whether or not the unit fired a spike during EVERY OTHER millisecond (so max firing rate is 500Hz)
   chance = rand(1,2000);
   for( k=2:2:2000)
      chance(k) = 2;
   end
   fired = find( chance < unitRate );

   % Compute spike times in seconds
   fired = pss_glob.plexTime + (fired-1)/1000;

   % Add spike timestamps for units to our timestamp matrix.
   fired = fired';
   nStamps = length(fired);
   tStamps = horzcat(1*ones(nStamps,1), pss_glob.unitIDs(i,1)*ones(nStamps,1), ...
      pss_glob.unitIDs(i,2)*ones(nStamps,1), fired);
   pss_glob.timestamps = vertcat(pss_glob.timestamps, tStamps);
end

% Finally, sort all timestamps in chronological order!
pss_glob.timestamps = sortrows(pss_glob.timestamps, 4);
