function plexsecthist( event, trial )
% PLEXSECTHIST PLEXMON trial handler that prepares tagged section histograms. 
%
%  PLEXMON is a Matlab GUI that monitors the event stream from the Plexon MAP 
%  server as trials are run in Maestro.  When it has collected a trial's worth 
%  of data, it invokes a trial handler function (specified by the user) that 
%  processes the trial data supplied by PLEXMON.
%
%  PLEXSECTHIST is a PLEXMON trial handler which accumulates and displays 
%  "tagged section" histograms for all neural "units" monitored by PLEXMON. The 
%  PLEXSECTHIST figure window is apportioned into an NxM grid of histograms, 
%  where N is the number of distinct tagged sections encountered thus far and 
%  M is the number of different neural units being monitored on the Plexon.  At 
%  the top of each column of histograms is a label reflecting the name of the 
%  unit as it appears on the Plexon (eg, 'sig001a'), and to the left of each 
%  row is a label reflecting the name of the tagged section represented by the 
%  histograms in that row.  Assuming each tagged section in the Maestro trial 
%  corresponds to a distinct test condition, this format makes it easy to 
%  compare one unit's responses to all the different test conditions, or 
%  compare the responses of all the units to a single test condition.  Sections 
%  are ordered from top to bottom alphabetically by section tag name -- keep 
%  this in mind when naming the tagged sections in your Maestro trial set.
%
%  All histograms include a "prolog" and "epilog" -- both 50ms long -- that 
%  bracket the extent of the tagged section.  The x-axis for all histograms is 
%  divided into P bins, where P is the # of bins in the longest tagged section 
%  encountered. Bin size is fixed at 10ms.  A blue calibration bar drawn below 
%  the bottom histogram in each column spans the duration of the longest 
%  section. Also, a thin white line in each histogram marks the beginning of 
%  the tagged section.\
%
%  Note that, while tagged sections need not be the same length, it makes sense 
%  to design them that way for ease of comparison.  The epilog and prolog are 
%  provided to measure the baseline firing rate -- so it is important that the 
%  trials be designed so that there is no stimulus present during the 50ms on 
%  either side of any tagged section.
%
%  For all histograms in a given column -- representing responses of a unit to 
%  different test conditions --, the y-axis range is [0..maxHz], where maxHz is 
%  the maximum observed firing rate of that unit across all tagged sections. 
%  Typically, maxHz will be different for different units.  The current maxHz
%  for each unit is written just above the top histogram in each column.
%
%  To learn more about "tagged  sections", consult the Maestro User's Guide. 
%  For more on the Plexon, see the appropriate user manuals.
%
%  Arguments:
%    event     The PLEXMON event ID: 'start', 'stop', or 'next'.  If it is 
%              anything else, PLEXSECTHIST displays an error message in the 
%              Matlab command window.
%    trial     The trial data structure supplied by PLEXMON.  Ignored for all 
%              events except 'next'.
%
%  Note that, between invocations, PLEXSECTHIST maintains accumulated data and 
%  status information in the global variable 'psh_glob'.  Do not mess with 
%  this variable!
% 
%  PLEXSECTHIST('start', []) clears out any previously accumulated histograms 
%  and starts over.  The PLEXSECTHIST figure window is emptied.
%
%  PLEXSECTHIST('stop', []) has no effect -- the figure window is left intact 
%  for viewing until the next 'start' event is received.
%
%  PLEXSECTHIST('next', T) processes the trial data in T and updates the 
%  contents of the figure window after every 5th trial is processed.
%  *** IMPORTANT: Tests showed that updating the figure window took a 
%  disproportionately large amount of time, quickly causing PLEXMON to drag 
%  well behind the data collection timeline. To alleviate this problem, the 
%  figure window is now only updated every 5 trials.
%
%  NOTE that the user may close the PLEXSECTHIST figure window at any time. 
%  This has the effect of obliterating any accumulated histogram data (global 
%  variable psh_glob is cleared from the workspace), so do not close the 
%  window until you're finished with it!
%
% See also: PLEXMON
%
% 19mar2019: Removed 'EraseMode' property from the textbox and line annotations
% generated in the plotHistograms() function. Support for that property was
% removed as of R2014b. 

% The global structure in which we maintain stuff across invocations of the function.
global psh_glob;

% Check arguments
if( nargin ~= 2 )
   help plexsecthist;
   return;
end

% The user could close the figure between any invocation of PLEXSECTHIST -- clearing our global structure so that it 
% is empty now.  Check for that condition and initialize accordingly.
if( isempty(psh_glob) )
   psh_glob.binSize = 10;                    % bin size in milliseconds
   psh_glob.prolog = 50;                     % prolog in milliseconds
   psh_glob.epilog = 50;                     % epilog in milliseconds
   psh_glob.fig = -1;                        % figure not created yet
   psh_glob.nTrials = 0;                     % no trials processed yet
   psh_glob.lastTrial = '<none>';  
   psh_glob.unitIDs = zeros(0,1);            % no units being monitored
   psh_glob.histData = [];                   % no section histogram data yet
   psh_glob.tMaxLenMS = 0; 
end

% If figure does not exist, create it.  Put name of this function in the title bar, and set the "close request" fcn 
% to clear our global data structure prior to deleting the figure.
if( ~ishandle(psh_glob.fig) )
   psh_glob.fig = figure;
   set(psh_glob.fig, 'CloseRequestFcn', 'closereq; clear global psh_glob;');
   set(psh_glob.fig, 'NumberTitle', 'off', 'Name', 'PLEXSECTHIST');
end

% Respond to one of three possible events...
if( strcmp(lower(event), 'start') )
   % Clear out data (if any) collected during a previous run.
   psh_glob.nTrials = 0; 
   psh_glob.lastTrial = '<none>'; 
   psh_glob.unitIDs = zeros(0,1); 
   psh_glob.histData = []; 
   psh_glob.tMaxLenMS = 0; 

   % Make sure figure is empty
   plotHistograms; 
   return;
elseif( strcmp(lower(event), 'stop') )
   % PLEXMON has stopped.  We don't do anything here.  We want to leave the figure up for viewing.
   return;
elseif( ~strcmp(lower(event), 'next') )
   % Programming error! Event not recognized.
   disp( sprintf('PLEXSECTHIST:  Unknown event %s', event) );
   return;
end

% If we get here, then we received the 'next' event -- time to process a trial's worth of data...
psh_glob.nTrials = psh_glob.nTrials + 1;
psh_glob.lastTrial = trial.name;

% Ignore trials that did not run to completion!
if( trial.aborted | trial.lostFix )
    disp( sprintf('PLEXSECTHIST:  Trial %s did not run to completion -- SKIPPED.', trial.name) );
    return;
end

% Check that trial data includes information about tagged sections that is recorded in the Maestro data file.  If 
% there is no such information, then this handler does not process the data!
if( isempty(trial.maestroData) )
   disp( sprintf('PLEXSECTHIST:  No Maestro-collected data available for trial %s', trial.name) );
   return;
end
if( isempty(trial.maestroData.tagSections) )
   disp( sprintf('PLEXSECTHIST:  Trial %s lacks any tagged sections.', trial.name) );
   return;
end

% If trial data includes timestamps for spikes from neural units that we have not encountered yet, then 
% append those units to our list.  If there are no unit timestamps in the trial, then there's nothing to do.
if( isempty(trial.unitIDs) )
   return;
end
for( n=1:length(trial.unitIDs) )
   if( isempty( find(psh_glob.unitIDs == trial.unitIDs(n)) ) )
       psh_glob.unitIDs(end+1) = trial.unitIDs(n);
   end
end

% make sure we have a histogram bin buffer for all Plexon units encountered thus far!
nUnits = length(psh_glob.unitIDs);
for( n=1:length(psh_glob.histData) )
   binbufsz = size(psh_glob.histData(n).bins);
   if( nUnits > binbufsz(1) )
      extra = nUnits - binbufsz(1);
      psh_glob.histData(n).bins = vertcat(psh_glob.histData(n).bins, zeros(extra,binbufsz(2)));
   end
end

% If we've already accumulated section histogram data, construct cell array of tagged section names that 
% exist thus far.
existingTags = {};
if( ~isempty(psh_glob.histData) )
   existingTags = {psh_glob.histData(1:end).tag};
end

%
% BEGIN: process each tagged section in trial
%
for( n=1:length(trial.maestroData.tagSections) ) 
   section = trial.maestroData.tagSections(n);

   % skip section -- it is not well-defined!
   if( section.tLen < 0 )  
      continue;
   end

   % keep track of the longest section length (NOT including prolog and epilog!) across all tagged sections 
   % encountered (this is for plotting purposes)
   if( double(section.tLen) > psh_glob.tMaxLenMS )
      psh_glob.tMaxLenMS = double(section.tLen);
   end

   % compute time interval spanning section plus prolog and epilog, in msecs RELATIVE TO START OF TRIAL. Note that the 
   % section start time provided in trial.maestroData.taggedSections is relative to the start of recording!
   tBegin = double(trial.maestroData.targets.tRecordOn) + double(section.tStart) - psh_glob.prolog; 
   tLen = double(section.tLen) + psh_glob.prolog + psh_glob.epilog; 
   tEnd = tBegin + tLen;                            

   % compute number of COMPLETE bins spanning the section -- section length adjusted to integer # of bins
   nBins = floor(tLen/psh_glob.binSize); 
   tLen = psh_glob.binSize*nBins; 
   tEnd = tBegin + tLen;

   % compute length of section actually recorded by Plexon; skip if did not record any COMPLETE bins in section 
   trialLenMS = trial.len * 1000;
   tRecordedLen = 0; 
   if( tEnd>0 & tBegin < trialLenMS )
      tRecordedLen = min(tEnd, trialLenMS) - max(tBegin, 0);
   end
   if( tRecordedLen < psh_glob.binSize ) 
      continue;
   end

   % find the histogram buffer for this tagged section...
   if( isempty(psh_glob.histData) ) 
      % we're adding our very first tagged section!
      psh_glob.histData(1).tag = section.tag; 
      psh_glob.histData(1).bins = zeros(length(psh_glob.unitIDs),nBins);
      psh_glob.histData(1).nsamples = zeros(1,nBins);
      existingTags = {psh_glob.histData(1:end).tag}; 
   else 
      % else, see if we've already encounted section 
      index = strmatch(section.tag, existingTags, 'exact');
      if( isempty(index) )
         % if not, append a new buffer to hold results for the new section
         psh_glob.histData(end+1).tag = section.tag; 
         psh_glob.histData(end).bins = zeros(length(psh_glob.unitIDs),nBins);
         psh_glob.histData(end).nsamples = zeros(1,nBins);
         existingTags = {psh_glob.histData(1:end).tag};
      elseif( size(psh_glob.histData(index).bins, 2) < nBins )
         % otherwise, grow bin buffer if necessary -- a section could have different lengths in different trials! 
         currLen = size(psh_glob.histData(index).bins, 2); 
         extra = nBins-currLen;
         psh_glob.histData(index).bins = horzcat(psh_glob.histData(index).bins, zeros(length(psh_glob.unitIDs),extra));
         psh_glob.histData(index).nsamples = horzcat(psh_glob.histData(index).nsamples, zeros(1,extra));
      end
   end
   index = strmatch(section.tag, existingTags, 'exact');

   % determine range of COMPLETE bins in section that were recorded by Plexon during trial
   iFirstBin = 1;
   if( tBegin < 0 )
      iFirstBin = 1 + ceil(abs(tBegin)/psh_glob.binSize);
   end
   nRecordedBins = floor(tRecordedLen/psh_glob.binSize);
   iLastBin = iFirstBin + nRecordedBins - 1;
   
   % compute time interval covered by this range of bins, in msecs RELATIVE TO START OF SECTION
   t0 = psh_glob.binSize*(iFirstBin-1); 
   t1 = psh_glob.binSize*(iLastBin); 

   % incr #samples in each bin COMPLETELY covered by recorded portion of Plexon trial
   psh_glob.histData(index).nsamples(iFirstBin:iLastBin) = psh_glob.histData(index).nsamples(iFirstBin:iLastBin) + 1;

   % accumulate histogram data for all Plexon "units" being monitored
   for( m=1:length(psh_glob.unitIDs) ) 
      % from ID# get Plexon ch# and unit# : ID = 10*ch# + unit#
      chan = floor(psh_glob.unitIDs(m) / 10); 
      unit = psh_glob.unitIDs(m) - 10*chan;

      % get unit's recorded spike times in MILLISECONDS relative to start of section.  If no spikes, move on.
      spikes = trial.plexunits( find( trial.plexunits(:,1)==chan & trial.plexunits(:,2)==unit ), 3 ) * 1000;
      if( isempty( spikes ) )
         continue;
      end
      spikes = spikes - tBegin;

      % get bin #s of spikes falling in recorded portion of the section.  Note that the same bin could appear more 
      % than once in this array!
      hits = spikes( find(spikes>=t0 & spikes<t1) );
      if( isempty( hits ) )
         continue;
      end
      hits = floor( hits/psh_glob.binSize ) + 1;

      % increment the affected bins in the unit's section histogram
      for( p=1:length(hits) )
         psh_glob.histData(index).bins(m, hits(p)) = psh_glob.histData(index).bins(m, hits(p)) + 1;
      end
   end
end 
%
% END: process each tagged section in trial

% now update the figure to reflect new state of tagged section histograms
% WARNING: plotHistograms IS VERY SLOW, so we only update the figure after 
% every 5 trials processed. 
if(rem(psh_glob.nTrials, 5) == 0)
   plotHistograms;
end

function plotHistograms()

% The histogram data is stored in this global.
global psh_glob;

% Make our figure the current one and clear it (clf also removes annotation objects).
figure(psh_glob.fig);
clf(psh_glob.fig);

% If there's no data yet, there's nothing to do!
if( isempty(psh_glob.histData) )
   return;
end

% Write a status line along the bottom of the figure.  Note:  annotation() not available until Matlab 7.0.
%hStatus = uicontrol('Style', 'text', 'Units', 'pixels', 'Position', [0 0 1000 50], 'FontSize', 8, ...
%   'FontUnits', 'points', 'FontWeight', 'bold', 'HorizontalAlignment', 'left', ...
%   'String', sprintf( 'Status: Processing trial %s. N=%d', psh_glob.lastTrial, psh_glob.nTrials ) );
%extStatus = get(hStatus, 'Extent');
%set(hStatus, 'Position', [0 0 extStatus(3) extStatus(4)]);
%set(hStatus, 'BackgroundColor', get(psh_glob.fig, 'Color'));
hStatus = annotation('textbox', [0.01 0 0.99 0.01], 'FitHeightToText', 'on', 'BackgroundColor', 'none', 'Margin', 3, ...
   'FontSize', 10, 'FontUnits', 'points', 'FontWeight', 'bold', 'HorizontalAlignment', 'left', ...
   'VerticalAlignment', 'bottom', 'LineStyle', 'none', 'Interpreter', 'none', ...
   'String', sprintf( 'Status: Processing trial %s. N=%d', psh_glob.lastTrial, psh_glob.nTrials ) );
pos = get(hStatus, 'Position');
pos(2) = 0.01;
set(hStatus, 'Position', pos);


% Construct unit labels: sig{ch}{unit}, where {ch} is zero-padded 3-digit channel# and {unit} is 'a', 'b', 'c', or 
% 'd' depending on the unit#.
unitLtrs = {'a', 'b', 'c', 'd'};
unitLabels = cell(1,length(psh_glob.unitIDs));
for( i=1:length(psh_glob.unitIDs) )
   % from ID# get Plexon ch# and unit# : ID = 10*ch# + unit#
   chan = floor(psh_glob.unitIDs(i) / 10); 
   unit = psh_glob.unitIDs(i) - 10*chan;

   unitLabels{i} = sprintf('sig%03d%s', chan, unitLtrs{unit} );
end

% Reorder indices of section histogram buffers so that section tags are in alphabetical order.
sectionTags = {psh_glob.histData(1:end).tag};
orderedTags = sort(sectionTags);
n = length(orderedTags);
orderedSectIndices = zeros(1,n);
for( i=1:n )
    orderedSectIndices(i) = strmatch( orderedTags{i}, sectionTags, 'exact' );
end

% We break up figure into NxM subplots, where N is the # of tagged sections and M is the # of units monitored 
n = length(psh_glob.histData);
m = length(psh_glob.unitIDs);
for( i=1:m )
   % FOR EACH UNIT MONITORED...

   % For each tagged section, compute firing rate per bin.  Since sections can have different lengths, we must save 
   % each histogram vector in a cell array.  Also compute max firing rate and max #bins over all sections. 
   sectionHists = cell(1,n);
   maxHz = 1;                                            % don't permit a zero max firing rate
   maxNBins = 0;
   for( j=1:n )
      nreps = psh_glob.histData(j).nsamples;
      noSampIndices = find(nreps==0);                                   % avoid divide-by-0 error!!
      nreps(noSampIndices) = nreps(noSampIndices) + 1; 
      sectionHists{j} = psh_glob.histData(j).bins(i,:) ./ nreps;
      sectionHists{j} = sectionHists{j} / (psh_glob.binSize * 0.001);   % converts to firing rate in Hz
      maxHz = max(maxHz, max(sectionHists{j}) ); 
      maxNBins = max(maxNBins, length(sectionHists{j}));
   end

   % Now plot firing rate vs bin# for all tagged sections for the current unit.  Each subplot will have the same 
   % axes: [0..maxHz] in Y, [1..maxNBins] in X.  All axes are hidden.  Tagged section subplots are drawn from top to 
   % bottom in alphabetical order of the section tags.
   for( j=1:n )
      subplot(n,m, (j-1)*m + i);
      sectionIndex = orderedSectIndices(j);
      set(gca, 'Visible', 'off');
      axis([1 maxNBins 0 maxHz]); 

      % To the left of each row, write the tag name of the section represented by the histograms in that row.
      % Center it vertically WRT the row of subplots, and center it horizontally in the space between the left edge of 
      % the figure and the left edge of the leftmost subplot.  Note that setting 'VerticalAlignment' to 'middle' 
      % centers the text within the textbox; we still have to center the textbox wrt the subplot.
      if( i==1 ) 
         pos = get(gca, 'Position');
         y = pos(2) + pos(4)/2;
         hText = annotation('textbox', [0 y pos(1) 0.01], 'FitHeightToText', 'on', ...
            'BackgroundColor', 'none', 'Margin', 1, 'FontSize', 8, 'FontUnits', 'points', 'FontWeight', 'bold', ...
            'HorizontalAlignment', 'center', 'VerticalAlignment', 'middle', 'LineStyle', 'none', ...
            'Interpreter', 'none', 'String', sectionTags{sectionIndex} );
         pos = get(hText, 'Position');
         pos(2) = y - pos(4)/2;
         set(hText, 'Position', pos);
      end

      % At the top of each column, put the name of the corresponding unit.  Center it horizontally above the column
      % and center it vertically  between the top edge of figure and the top edge of the topmost subplot.  Also, put a 
      % text label just above the topleft corner of the top subplot that reflects the max observed firing rate across 
      % all tagged sections for the corresponding unit.
      if( j==1 )
         pos = get(gca, 'Position');
         y = (1 + pos(2) + pos(4))/2;
         hText = annotation('textbox', [pos(1) y pos(3) 0.01], 'FitHeightToText', 'on', ...
            'BackgroundColor', 'none', 'Margin', 1, 'FontSize', 8, 'FontUnits', 'points', 'FontWeight', 'bold', ...
            'HorizontalAlignment', 'center', 'VerticalAlignment', 'middle', 'LineStyle', 'none', ...
            'Interpreter', 'none', 'String', unitLabels{i} );
         pos = get(hText, 'Position');
         pos(2) = y;
         set(hText, 'Position', pos);

         text( 0, 1, sprintf( '%dHz', round(maxHz) ), ...
            'Units', 'normalized', 'HorizontalAlignment', 'left', 'VerticalAlignment', 'bottom', ...
            'FontSize', 8, 'FontUnits', 'points', 'FontWeight', 'bold' );
      end

      % At the bottom of each column, draw a line representing the length of the longest tagged section.  The line 
      % should start at the end of the prolog, which is when each section actually begins.  Under this line, put a 
      % text label indicating the length of the longest section, in milliseconds.
      if( j==n )
         % Draw horizontal line below plot, from end of prolog to end of the longest tagged section.
         x(1) = round(psh_glob.prolog / psh_glob.binSize);
         x(2) = x(1) + round(psh_glob.tMaxLenMS / psh_glob.binSize);
         y(1) = -round(maxHz/5);
         y(2) = y(1);
         line(x,y, 'LineStyle', '-', 'LineWidth', 1.5, 'Color', 'b', 'Clipping', 'off');
         % Put text label "N ms" below the line.
         text( sum(x)/2, y(1), sprintf( '%d ms', psh_glob.tMaxLenMS ), ...
            'Units', 'data', 'HorizontalAlignment', 'center', 'VerticalAlignment', 'top', ...
            'FontSize', 8, 'FontUnits', 'points', 'FontWeight', 'bold');
      end

      % Draw the histogram -- dont' let the axis range change.  Also draw a white line marking the start of section
      hold on;
      bar( sectionHists{sectionIndex}, 1 );
      x(1) = round(psh_glob.prolog / psh_glob.binSize);
      x(2) = x(1);
      y(1) = 0;
      y(2) = maxHz;
      line(x,y, 'LineStyle', '-', 'LineWidth', 1, 'Color', 'w');
      hold off;
   end
end

