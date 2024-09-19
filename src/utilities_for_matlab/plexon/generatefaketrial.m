function trial = generatefaketrial( n )
% GENERATEFAKETRIAL Generates fake output from PLEXMON for testing PLEXSECTHIST.
%  n = trial index

% check for invalid trial index
if( n<=0 | n>9999 )
   disp('GENERATEFAKETRIAL: n must be [1..9999]');
   trial = [];
   return;
end

% Probability of firing of unit #11 in 4 different 300ms tagged sections 'secA' - 'secD'
t = [0:299];
unit11_base = 0.1;
unit11 = zeros(4,300);
unit11(1,:) = [ones(1,50)*0.1 (0.7-0.7*t((51:end)-50)/300)];
unit11(2,:) = [(0.1-t(1:50)/500) zeros(1,250)];
unit11(3,:) = [ones(1,100)*0.1 ones(1,200)*0.9];
unit11(4,:) = [(0.1+t(1:50)/500) ones(1,250)*0.2];

% Similarly for unit #23
unit23_base = 0.2;
unit23 = zeros(4,300);
unit23(1,:) = [(0.2-t(1:150)/750) zeros(1,150)];
unit23(2,:) = [(0.2+t(1:150)/215) ones(1,150)*0.9];
unit23(3,:) = [(0.2+t(1:150)/500) ones(1,150)*0.5];
unit23(4,:) = 0.2 + t/900;

% Randomly choose order of the 4 sections.
sectionOrder = randperm(4);
sectionTags = {'sectA', 'sectB', 'sectC', 'sectD'};

% Prepare some fields in the return structure
trial.name = sprintf( 'faketrial%d', sectionOrder(1) );
trial.maestroPath = sprintf( 'test.%04d', n );
trial.savedFile = 1;
trial.lostFix = 0;
trial.aborted = 0;
trial.len = 1.8;
trial.unitIDs = [11; 23];
trial.markers = [2 0.000; 2 1.8];
trial.maestroData.targets.tRecordOn = 0;

% Prepare the tagged section info for the trial
for( i=1:length(sectionOrder) )
   m = sectionOrder(i);
   trial.maestroData.tagSections(i) = struct('tag', sectionTags{m}, 'firstSeg', i*2 - 1, 'lastSeg', i*2 - 1, ...
      'tStart', 200 + (i-1)*400, 'tLen', 300);
end

% Trial format: [base200 sec1 base100 sec2 base100 sec3 base100 sec4 base100], where base200 is a segment 
% during which units fire at baseline levels for 200ms, and base100 is a 100ms segment at base firing rates.
% secN refers to a tagged section in the order randomly chosen above.  Total trial time is 1.8seconds.
%
% Prepare 1x1800 vectors representing instantaneous firing rate for each unit.
%
base100 = ones(1,100);
unit11rate = ones(1,200)*unit11_base;
for( i=1:length(sectionOrder) )
   m = sectionOrder(i);
   unit11rate = horzcat( unit11rate, [unit11(m,:) base100*unit11_base] );
end
unit23rate = ones(1,200)*unit23_base;
for( i=1:length(sectionOrder) )
   m = sectionOrder(i);
   unit23rate = horzcat( unit23rate, [unit23(m,:) base100*unit23_base] );
end

% Prepare Plexon timestamp matrix of spikes.  Every 2ms in trial starting at t=1ms, choose a random # to decide if a 
% spike is generated on each unit IAW the current instantaneous firing rate.  If a spike "occurs", add the appropriate 
% timestamp to the Nx3 timestamp matrix.  Remember that Plexon timestamps here are in seconds relative to trial start.
trial.plexunits = zeros(0,3);
for( i=2:2:1800 )
   chance = rand(1,2);
   if( chance(1) < unit11rate(i) )
      trial.plexunits(end+1,:) = [1 1 (i-1)/1000];
   end
   if( chance(2) < unit23rate(i) )
      trial.plexunits(end+1,:) = [2 3 (i-1)/1000];
   end
end


