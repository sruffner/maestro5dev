function varargout = plexmon(varargin)
% PLEXMON A Plexon-Maestro data stream monitor.
%
%     This simple Matlab GUI provides a framework for monitoring data 
%     collected by both the Plexon Multichannel Acquisition Processor (MAP) 
%     and Maestro operating in trial mode.  It monitors the Plexon event 
%     stream until a complete trial's worth of data has been collected, 
%     uses READCXDATA to read in the corresponding Maestro trial data file, 
%     then passes a structure containing Plexon timestamps and Maestro 
%     data to a "trial handler" function.  The handler "consumes" the 
%     trial data in some manner, then returns control to PLEXMON.
%
%     Usage:
%     PLEXMON, entered in the Matlab command window, starts the GUI.  Only a 
%     single instance is allowed to run, so entering PLEXMON again has no 
%     effect except to reinitialize the existing singleton GUI.
%
%     The GUI includes Start and Stop buttons, plus text controls for entering 
%     (1) the directory in which PLEXMON should search for Maestro data files, 
%     and (2) the name of the "trial handler" function.  The Maestro directory 
%     is optional; if omitted or if the path does not exist, PLEXMON will not 
%     attempt to retrieve the Maestro data file associated with each trial 
%     culled from the Plexon data stream.  The trial handler, on the other 
%     hand, is required.  The user must enter a valid M-file or MEX function 
%     name that exists on the Matlab search path (as verified by the EXIST 
%     command).
%
%     When the user presses the Start button, PLEXMON verifies the existence 
%     of the trial handler function, notifies the handler that it is starting 
%     up, connects to the Plexon MAP server, and begins polling the server 
%     for event timestamps.  Status messages keep the user informed about 
%     what PLEXMON is doing.  PLEXMON will discard all timestamp data until 
%     it sees a "start event" (generated by Maestro and timestamped by the 
%     Plexon).  It then accumulates timestamps until the next "stop event" is 
%     detected.  These two events bracket the presentation of a single 
%     Maestro trial; see the next section for a more detailed description of 
%     the information Maestro sends to the Plexon before, during, and after 
%     a trial.  PLEXMON processes the Plexon timestamp data, reads in the 
%     corresponding Maestro data file (if possible), then passes all of the 
%     data to the handler function for processing.  When the handler returns, 
%     PLEXMON continues monitoring the Plexon event stream for the next 
%     trial's worth of data.  This process repeats indefinitely until the 
%     user presses the Stop button.  PLEXMON then disconnects from the MAP 
%     server, informs the trial handler that data monitoring has stopped, 
%     and enters an idle state.
%
%     Requirements on the trial handler function:
%     PLEXMON handles the mundane tasks of polling the Plexon MAP server 
%     and integrating the Plexon timestamp data with analog data and other 
%     information recorded in the Maestro data file.  But it is up to the trial 
%     handler function to do something "useful" with the data.  The function 
%     must take the form 
%
%                    trialConsumer( event, trial )
%     
%     where 'event' is a Matlab string and 'trial' is a structure containing 
%     trial data prepared by PLEXMON.  The event ID has three possible values: 
%
%     'start' : PLEXMON has started monitoring the Plexon data stream. Gives 
%        trial handler an opportunity to initialize or reinitialize itself. The 
%        second arg is an empty matrix in this case.
%     'stop'  : PLEXMON has stopped monitoring.  Trial handler may choose to 
%        clean up global state variables, report final results, and so on. 
%        Again, the second arg is an empty matrix.
%     'next'  : PLEXMON has just collected one Maestro trial's worth of data. 
%        The data is provided in the second argument, as a Matlab structure 
%        with the fields described below.
%
%        trial.name  :  String holding trial's name as culled from the Plexon 
%           strobed-character stream.
%        trial.maestroPath  :   String holding path to the corresponding 
%           Maestro data file.  This will be the concatenation of the Maestro 
%           directory specified in the GUI and the filename culled from the 
%           Plexon character stream.
%        trial.savedFile  :  Scalar flag -- nonzero if Maestro data file was 
%           saved.
%        trial.lostFix  :  Scalar flag -- nonzero if trial aborted prematurely 
%           because subject lost fixation.
%        trial.aborted  :  Scalar flag -- nonzero if trial aborted prematurely 
%           for another reason.
%        trial.maestroData  :  Maestro trial data as provided by READCXDATA. 
%           If the data file was not read, then this field is set to an empty 
%           matrix.
%        trial.plexunits  :  Nx3 matrix of neural unit event timestamps, where 
%           N is the number of unit events detected and col1 gives the Plexon 
%           channel#, col2 = unit#, and col3 = timestamp in seconds relative 
%           to the start of the Maestro trial.
%        trial.unitIDs  :  Mx1 vector listing IDs of the M different neural 
%           "units" for which at least one "spike" event was detected.  The ID 
%           is computed as channel# x 10 + unit#; valid channel #s lie in 
%           [1..16], and valid unit #s lie in [1..4].
%        trial.markers  :  Px2 matrix of marker pulse event timestamps, where 
%           P is the total # of pulse events detected on the Plexon's external 
%           event channels [2..9]. The first column holds the marker channel 
%           number, while col2 is a timestamp in seconds relative to the start 
%           of the Maestro trial.  Note that ch#2 corresponds to the trial 
%           "sync" markers on XS2 (see above).
%        trial.len  :  Length of trial in seconds (time between start and stop 
%           "sync" pulses); scalar.
%
%     The handler can do whatever it likes with the data.  PLEXSECTHIST, for 
%     example, builds "tagged section histograms" for any and all neural units 
%     recorded on the Plexon.  You are free to develop handlers for other 
%     analysis/presentation tasks, but it is important that they work FAST. 
%     PLEXMON does not resume monitoring the Plexon data stream until the 
%     handler function returns.  If the handler takes too long, some data could 
%     be missed.
%
%     How PLEXMON extracts a "trial" from the Plexon event stream:
%     PLEXMON is able to cull trial data from the Plexon data stream only 
%     because Maestro delivers strobed character data and synchronization 
%     pulses bracketing the timeline of each and every trial presented.  Before 
%     a trial actually begins, Maestro sends a "start" code (ASCII 0x2h), 
%     immediately followed by the null-terminated name of the trial, then the 
%     null-terminated data file name. If the trial is not to be saved in the 
%     first place, a "noFile" (ASCII 07h) character code, followed by a null 
%     character (ASCII 00h), serves as a placeholder for the data file name. 
%     After the trial has ended, if the animal lost fixation, a "lostFix" 
%     (ASCII 0Eh) code is sent. If the trial aborted for any reason other than 
%     a fixation break, the "abort" (ASCII 0Fh) character is written. If the 
%     data file was saved successfully, the "dataSaved" (ASCII 06h) character 
%     is sent. Finally, the "stop" code (ASCII 0x3h) is sent to mark the end of 
%     character data for the trial.
%
%     To synchronize the Plexon and Maestro timelines, Maestro delivers a pulse 
%     on a dedicated digital output channel, DOUT11.  DOUT11 is strobed 
%     immediately after the trial starts and immediately after the trial ends 
%     (even before the end-of-trial reward is delivered). PLEXMON assumes that 
%     DOUT11 is connected to the Plexon's XS2 input, and that Plexon external 
%     event channel# 2 is dedicated to XS2.
%
%     IMPORTANT:  It has been observed that XS2 pulses are occasionally missed. 
%     As a failsafe, the PLEXMON ASSUMES that a second marker pulse is 
%     delivered on Maestro digital output DOUT2 at the start of the Maestro 
%     trial's first and last segments.  Also, it is assumed that the last 
%     segment is a dummy segment of negligible length, so that the second DOUT2 
%     pulse can be considered to mark the end of the trial.  DOUT2 must be 
%     connected to the Plexon's external channel #4.  If there are not exactly 
%     two XS2 pulses in the Plexon timestamp data for a single trial, then the 
%     function will use the DOUT2 pulses instead.  If there are not exactly two 
%     DOUT2 pulses either, then the Plexon data cannot be synchronized with the 
%     Maestro trial timeline.  In this case, PLEXMON briefly displays an error 
%     message in the GUI, then discards the trial data and begins collecting the 
%     next trial.
%
%
%     System Requirements:  
%     WindowsXP SP1+, Matlab 7.0+.  Must be run on the same machine that runs 
%     the Plexon MAP server.
%
%     Dependencies: 
%     mexPlexOnline.* -- This MEX function handles communications with 
%        the Plexon server.
%     checkFile.* -- Simple Windows-specific MEX function which is used 
%        by PLEXMON to check for the existence of each Maestro data file 
%        before trying to read in the data.
%     readcxdata.* -- This MEX function reads a Maestro data file into 
%        a large Matlab structure.  Format of this structure is described 
%        in the Maestro User's Guide at 
%        http:\\keck.ucsf.edu\~sruffner\userguide\index.htm.
%     plexmon.fig -- GUIDE-generated description of the PLEXMON GUI.
%
% For the MEX functions, use .DLL for Windows XP (is anyone still doing that?!);
% for Windows 7 and beyond, use .MEXW32 for 32-bit OS, .MEXW64 for 64-bit.
%
%  See also:  READCXDATA, PLEXSECTHIST.
%
% 19mar2019: Modified to explicitly assign return value of checkfile() MEX
% function to a LHS variable. Without the explicit assignment, more recent
% Matlab version invokes checkfile() with no LHS argument, and so the function
% always fails, returning 0.
%

% Last Modified by GUIDE v2.5 08-Nov-2005 13:46:41

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @plexmon_OpeningFcn, ...
                   'gui_OutputFcn',  @plexmon_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin & isstr(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before plexmon is made visible.
function plexmon_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to plexmon (see VARARGIN)

% Choose default command line output for plexmon
handles.output = 0;

% initialize Maestro directory and trial consumer function names to empty
% strings.  initialize status line text.
handles.plexmon = [];
handles.plexmon.mdir = '';
set(handles.maestroDir, 'String', handles.plexmon.mdir);
handles.plexmon.trialConsumer = '';
set(handles.trialFunc, 'String', handles.plexmon.trialConsumer);

% plexmon state: idle, running, stopping
handles.plexmon.state = 'idle';
set(handles.stopPB, 'Enable', 'off');
set(handles.statusLine, 'String', 'Idle.');

% Update handles structure
guidata(hObject, handles);

% UIWAIT makes plexmon wait for user response (see UIRESUME)
% uiwait(handles.figure1);


% --- Outputs from this function are returned to the command line.
function varargout = plexmon_OutputFcn(hObject, eventdata, handles)
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


% --- Executes during object creation, after setting all properties.
function maestroDir_CreateFcn(hObject, eventdata, handles)
% hObject    handle to maestroDir (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc
    set(hObject,'BackgroundColor','white');
else
    set(hObject,'BackgroundColor',get(0,'defaultUicontrolBackgroundColor'));
end



function maestroDir_Callback(hObject, eventdata, handles)
% hObject    handle to maestroDir (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Save new contents of edit control labelled 'Maestro Dir:'
handles.plexmon.mdir = get(hObject,'String');
guidata(hObject, handles);

% --- Executes during object creation, after setting all properties.
function trialFunc_CreateFcn(hObject, eventdata, handles)
% hObject    handle to trialFunc (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: edit controls usually have a white background on Windows.
%       See ISPC and COMPUTER.
if ispc
    set(hObject,'BackgroundColor','white');
else
    set(hObject,'BackgroundColor',get(0,'defaultUicontrolBackgroundColor'));
end



function trialFunc_Callback(hObject, eventdata, handles)
% hObject    handle to trialFunc (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Save name of trial consumer function entered into edit control
handles.plexmon.trialConsumer = get(hObject,'String');
guidata(hObject, handles);


% startPB_Callback -- MAIN RUNTIME LOOP
% This callback executes when the startPB is depressed.  It connects 
% to the Plexon server and continuously polls the server for timestamp 
% data.  When a Maestro trial is completed, it prepares the Plexon 
% unit timestamps (relative to the start of the trial), then attempts 
% to read in the Maestro data file itself -- if the user has specified 
% a valid file system directory and the data file named in the Plexon 
% strobed character stream is found there.  To accommodate network delays, 
% it will wait up to 30 seconds for the data file to "appear".
%
% After reading in the Maestro file with MEX function readcxdata(), we 
% prepare a structure that is then passed to the "trial consumer" 
% function whose name is entered in the 'trialFunc' widget.
%
% If the trial consumer is unspecified or does not exist, the function 
% will abort before connecting to the Plexon.
%
% 23mar16(sar): The newer Omniplex D has 16-bit inputs for the character
% codes, vs 8-bits for the original Plexon MAP system. The upper 8 bits
% are not used and are masked out. The character codes are in column 3
% of the event data matrix retrieved by mexPlexOnline.
%
function startPB_Callback(hObject, eventdata, handles)
% hObject    handle to startPB (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% First check that trial consumer exists as an M-file or MEX file
bOk = (length(handles.plexmon.trialConsumer) > 0);
if( bOk )
   res = exist(handles.plexmon.trialConsumer);
   bOk = (res == 2 | res == 3);
end
if( ~bOk )
   set(handles.statusLine, 'String', 'ERROR: Must enter valid M-file or MEX function for trial handler!');
   return;
end

% Connect to Plexon server.  If this fails, abort.
plexServ = mexPlexOnline(9, -1);
if( plexServ == 0 )
   set(handles.statusLine, 'String', 'ERROR: Failed to connect to Plexon server!' );
   return;
end

% Tell trial handling function that we're starting to poll the Plexon for trials.
feval(handles.plexmon.trialConsumer, 'start', []);

% Check that string in 'Maestro Dir:' widget specifies a valid, existing directory.
% If not, then we do not attempt to read in Maestro file corresponding to each 
% trial parsed from the Plexon stream.
isMaestroDirValid = isdir(handles.plexmon.mdir);

% enter running state; disable startPB and edit controls, enable stopPB
handles.plexmon.state = 'running';
guidata(hObject, handles);
set(handles.startPB, 'Enable', 'off');
set(handles.stopPB, 'Enable', 'on');
set(handles.maestroDir, 'Enable', 'off');
set(handles.trialFunc, 'Enable', 'off');

% strobed character codes marking key events in Maestro trial
startCode = 2;       % trial start
stopCode = 3;        % trial stop
lostFixCode = 14;    % trial aborted b/c subject broke fixation
abortCode = 15;      % trial aborted for reason other than lostfix
dataSavedCode = 6;   % Maestro data file successfully saved
nullCode = 0;        % terminates trial name and file name in character stream

%
% RUNTIME LOOP:  Poll Plexon for timestamps.  After each trial, read in corresponding 
% Maestro file (if found) and invoke trial consumer.  Repeat until user hits the 
% stopPB or closes the plexmon UI.
%
% Here are the different stages in the acquisition and processing of one trial's 
% worth of data:
%    0 = Waiting for trial start.
%    1 = Acquiring.
%    2 = Reading in Maestro data file (may have to wait a long time here!)
%    3 = Calling trial handler function (plexmon UI is blocked here!)
%
nTrials = 0;
stopping = 0;
stage = 0;
timestamps = zeros(0,4);
trial = [];
tStartStage2 = clock;
set(handles.statusLine, 'String', 'Waiting for trial start...');
while( ~stopping )
   % If we're still acquiring the current trial, concatenate any new timestamps to what we already have.
   if( stage < 2 )
      [n ts] = mexPlexOnline(1,plexServ);
      if( n > 0 )
         % mask out any bits beyond the first 8 (for Omniplex D, which has 16-bit DI ports)
         ts(:,3) = bitand(ts(:,3), 255*ones(size(ts(:,3))), ‘int16’);
         timestamps = vertcat(timestamps, ts);
      end
   end

   % If we're still waiting for trial start, check new timestamps for the trial 'start' code.  If we got it, discard 
   % timestamps that precede it and move to next stage. Otherwise, discard all of the timestamps.
   if( stage == 0 & ~isempty(timestamps) )
      startIndices = find( timestamps(:,1)==4 & timestamps(:,2)==257 & timestamps(:,3)==startCode );
      if( ~isempty(startIndices) )
         timestamps = timestamps( startIndices(1):end, : );
         stage = 1;
         set(handles.statusLine, 'String', 'Acquiring trial...' );
      else
         timestamps = zeros(0,4);
      end
   end

   % If trial has started but not finished, check timestamp buffer for the trial 'stop' code.  If we got it, then 
   % process timestamp data and prepare trial data structure.  Remove all timestamps from timestamp buffer that belong 
   % to the completed trial; do NOT discard any timestamps after the 'stop' code!!!
   if( stage == 1 )
      stopIndices = find( timestamps(:,1)==4 & timestamps(:,2)==257 & timestamps(:,3)==stopCode );
      if( ~isempty(stopIndices) )
         trial = processTrial( timestamps( 1:stopIndices(1), : ) );
         if( stopIndices(1) == size(timestamps,1) )
            timestamps = zeros(0,4);
         else
            timestamps = timestamps( (stopIndices(1)+1):end, : );
         end

         % If timestamp data lacks the required sync pulses to synchronize Plexon and Maestro timelines, then we 
         % cannot use the data; processTrial() returns an empty matrix in this situation.  We post an error 
         % message in the status line and start over at stage 0.
         if( isempty(trial) )
            set(handles.statusLine, 'String', 'ERROR: Missing sync pulse(s).  Trial discarded!');
            pause(1);
            set(handles.statusLine, 'String', 'Waiting for trial start...');
            stage = 0;
            continue;
         end

         % If the Maestro directory exists and Maestro saved a data file, enter stage 2. Also construct path to 
         % this file, based on specified directory and the filename culled from Plexon timestamp data (this path is 
         % Windows-specific, of course).  Skip to stage 3 if we're not getting the Maestro data file.
         if( isMaestroDirValid & trial.savedFile & length(trial.maestroPath) > 0 )
            set(handles.statusLine, 'String', ['Getting Maestro file ' trial.maestroPath]);
            trial.maestroPath = [handles.plexmon.mdir '\\' trial.maestroPath];
            stage = 2;
            tStartStage2 = clock;
         else
            stage = 3;
         end
      end
   end

   % Get Maestro data file.  This could take some time because of network delays.  The longest we'll wait is 
   % 30 seconds.  We use Windows-specific MEX function checkfile() to see if we can open file with exclusive access.
   if( stage == 2 )
      gotFile = checkfile(trial.maestroPath);
      if(gotFile != 0)
         trial.maestroData = readcxdata(trial.maestroPath);
         stage = 3;
      elseif( etime(clock, tStartStage2) > 30 )
         set(handles.statusLine, 'String', 'Timed out waiting for Maestro file!');
         pause(0.5);
         stage = 3;
      end
   end

   % Once we've prepared the trial data, invoke trial handler to process it.  Since plexmon GUI will be unresponsive 
   % while the trial handler function executes, disable the stopPB while we're waiting.
   if( stage == 3 )
      msg = sprintf( 'WAIT -- Processing trial %s (num trials = %d)', trial.name, nTrials );
      set(handles.statusLine, 'String', msg);
      set(handles.stopPB, 'Enable', 'off');

      % pass trial data to handler for processing/presentation...
      feval(handles.plexmon.trialConsumer, 'next', trial);

      set(handles.stopPB, 'Enable', 'on' );
      nTrials = nTrials + 1;
      stage = 0;
      set(handles.statusLine, 'String', 'Waiting for trial start...');
   end

   % Pause briefly to let other callbacks run, etc. Then check to see if user has requested we stop.
   pause(0.2);
   handles = guidata(gcbo);
   stopping = strcmp(handles.plexmon.state, 'stopping');
end

% Close connection to Plexon server
mexPlexOnline(3, plexServ);

% Tell trial handling function that polling has stopped
feval(handles.plexmon.trialConsumer, 'stop', []);

% Return to idle state.
set(handles.maestroDir, 'Enable', 'on');
set(handles.trialFunc, 'Enable', 'on');
set(handles.startPB, 'Enable', 'on');
set(handles.stopPB, 'Enable', 'off');
set(handles.statusLine, 'String', 'Idle.');
handles.plexmon.state = 'idle';
guidata(hObject, handles);


% --- processTrial:  Prepares trial data structure given Plexon timestamps between trial 'start' and 'stop' codes. 
% 
% This function returns the trial data structure that is eventually passed to the trial handler function specified 
% in the plexmon GUI.  This MATLAB structure has the following fields:
%     t.name            String holding trial's name as culled from Plexon strobed-character stream.
%     t.maestroPath     String holding Maestro data file name as culled from Plexon strobed-character stream.
%     t.savedFile       Scalar flag -- nonzero if Maestro data file was saved.
%     t.lostFix         Scalar flag -- nonzero if trial aborted prematurely because subject lost fixation.
%     t.aborted         Scalar flag -- nonzero if trial aborted prematurely for another reason.
%     t.maestroData     Maestro trial data -- set to empty matrix (this is filled in elsewhere!)
%     t.plexunits       Nx3 matrix of neural unit event timestamps, where N is the number of unit events detected and 
%                       col1 = channel#, col2 = unit#, and col3 = timestamp in secs relative to trial start.
%     t.unitIDs         Mx1 vector listing IDs of the M different neural "units" for which at least one "spike" 
%                       event was detected.  The ID is computed as channel# x 10 + unit#; valid channel #s lie in 
%                       [1..24], and valid unit #s lie in [1..4].
%     t.markers         Px2 matrix of marker pulse event timestamps, where P is the total # of pulse events detected 
%                       on external event channels [2..9]. Col1 holds marker channel number, while col2 is timestamp 
%                       in secs relative to trial start.  Note that ch2 corresponds to sync markers on XS2.
%     t.len             Length of trial in seconds (time between start and stop sync pulses); scalar.
%
% 23mar16(sar) - Increased channel capacity from 16 to 24. For Omniplex D.
%
% The method ASSUMES that the Plexon strobed character stream is formatted exactly as described in the Maestro User's 
% Guide.  The "start" code is immediately followed by the null-terminated name of the trial, then the null-terminated 
% data file name. The latter is particularly important so that an analysis program can properly merge the Plexon unit 
% data with the behavioral responses recorded by Maestro. However, if the trial is not to be saved in the first place, 
% a "noFile" (ASCII 07h) character code, followed by a null character (ASCII 00h), serves as a placeholder for the 
% data file name. After the trial has ended, if the animal lost fixation, a "lostFix" (ASCII 0Eh) code is sent. If the 
% trial aborted for any reason other than a fixation break, the "abort" (ASCII 0Fh) character is written. If the data 
% file was saved successfully, the "dataSaved" (ASCII 06h) character is sent. Finally, the "stop" code is sent to mark 
% the end of character data for the trial.
%
% IT IS ALSO ASSUMED that the first row in the timestamp matrix provided marks the occurrence of the 'start' character 
% code, while the last row is the timestamp entry for the 'stop' code!
% 
% To synchronize the Plexon and Maestro timelines, Maestro delivers a pulse on a dedicated digital output channel, 
% DOUT11.  DOUT11 is strobed immediately after the trial starts and immediately after the trial ends (even before 
% the end-of-trial reward is delivered).  In order to use these sync markers, this function ASSUMES that DOUT11 is 
% connected to the Plexon's XS2 input, and that Plexon channel# 2 is dedicated to XS2.
%
% IMPORTANT:  It has been observed that XS2 pulses are occasionally missing.  As a failsafe, the function ASSUMES that 
% a second marker pulse is delivered on DOUT2 at the start of the Maestro trial's first and last segments.  Also, it 
% is assumed that the last segment is a dummy segment of negligible length, so that the second DOUT2 pulse can be 
% considered to mark the end of the trial.  DOUT2 must be connected to the Plexon's extern channel #4.  If there are 
% not exactly two XS2 pulses in the Plexon timestamp data, then the function will use the DOUT2 pulses instead.  If 
% there are not exactly two DOUT2 pulses either, then the Plexon data cannot be synchronized with the Maestro trial 
% timeline.  In this case, an empty matrix is returned.
%
function t = processTrial( ts )
% t      Trial data structure, as described above.  Empty matrix returned if sync pulses missing.
% ts     nx4 matrix of Plexon timestamps for exactly one trial, where --
%          col1 = timestamp type (1=neural unit, 4=external event)
%          col2 = channel number (ch 2 reserved for XS2 sync pulses, ch 257 is strobed character)
%          col3 = unit number (contains ASCII char code when col1=4 and col2=257)
%          col4 = timestamps in seconds

% Init all fields in our trial data structure
t.name = ''; 
t.maestroPath = ''; 
t.savedFile = 0; 
t.lostFix = 0; 
t.aborted = 0; 
t.maestroData = []; 
t.plexunits = zeros(0,3);
t.unitIDs = zeros(0,1);
t.markers = zeros(0,2);

% Strobed character codes marking key events in Maestro trial
startCode = 2;       % trial start
stopCode = 3;        % trial stop
lostFixCode = 14;    % trial aborted b/c subject broke fixation
abortCode = 15;      % trial aborted for reason other than lostfix
dataSavedCode = 6;   % Maestro data file successfully saved
noFileCode = 7;      % placeholder for data file name when no file was to be saved in the first place
nullCode = 0;        % terminates trial name and file name in character stream

% Cull external events from the set of all timestamps
extEvents = ts(find(ts(:,1)==4), :);

% Cull character stream from the set of all external events detected.  Note transposition from col to row vector!
charStream = extEvents(find(extEvents(:,2)==257), 3)';

% Get Maestro trial name -- first null-terminated string following 'start' code, which is ASSUMED to be the first 
% character in the character stream!
nullIndices = find(charStream == nullCode);
t.name = char( charStream(2:nullIndices(1)-1) );

% Get Maestro data file name if present -- null-terminated string immediately following trial name.  If not present, 
% there will be a "no file" placeholder ASCII code followed by the terminating null.
if( charStream(nullIndices(1)+1) ~= noFileCode )
   t.maestroPath = char( charStream(nullIndices(1)+1:nullIndices(2)-1) );
end

% Look for 'lostfix', 'aborted', and 'filesaved' codes.
charStream = charStream(nullIndices(2):end);
t.lostFix = ~isempty( find( charStream == lostFixCode ) );
t.aborted = ~isempty( find( charStream == abortCode ) );
t.savedFile = ~isempty( find( charStream == dataSavedCode ) );

% Get timestamps of sync pulses on XS2 (external event, ch# 2) that mark exact beginning and end of Maestro trial
syncTS = extEvents(find(extEvents(:,2)==2), 4);
if( length(syncTS) ~= 2 )
   % There should be exactly two XS2 markers.  If not, then something is wrong.  Try using DOUT2 (ext event, ch# 4)
   % instead.  If we don't get two DOUT2 markers, then we cannot sync Plexon data with Maestro trial.  ABORT.
   syncTS = extEvents(find(extEvents(:,2)==4), 4);
   if( length(syncTS) ~= 2 )
      t = [];
      return;
   end
end
t.len = syncTS(2) - syncTS(1);

% Save all external events with channel #s in the range 2-9 as "marker pulses".  First col is ch#, while second is time 
% of event relative to start of Maestro trial, in seconds.
t.markers = extEvents( find(extEvents(:,2)>=2 & extEvents(:,2)<=9), [2 4] );
t.markers(:,2) = t.markers(:,2) - syncTS(1);

% Indices of spikes from any sorted unit (unit# in col3 must not be zero!) that occurred during the trial timeline.
spikeIndices = find( (ts(:,1) == 1) & (ts(:,3) ~= 0) & (ts(:,4) > syncTS(1)) & (ts(:,4) < syncTS(2)) );

% If there were any spike events detected, prepare timestamp matrix and unitID vector
if( ~isempty(spikeIndices) )
    t.plexunits = ts(spikeIndices, 2:4);
    t.plexunits(:,3) = t.plexunits(:,3) - syncTS(1);

    unitsFound = zeros(24,4);                   % ASSUME 24 possible chans 1..24, and 4 sorted units 1..4 per chan
    for i = 1:size(t.plexunits, 1)
        chan = t.plexunits(i,1);
        unit = t.plexunits(i,2);
        if( ~unitsFound(chan,unit) )
            unitsFound(chan,unit) = 1;
            t.unitIDs(end+1,1) = 10*chan + unit;
        end
    end
end


% --- Executes on button press in stopPB.
function stopPB_Callback(hObject, eventdata, handles)
% hObject    handle to stopPB (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
if( strcmp(handles.plexmon.state, 'running') )
    set(handles.stopPB, 'Enable', 'off');
    handles.plexmon.state = 'stopping';
    guidata(hObject, handles);
end


% --- The CloseRequestFcn for the PLEXMON GUI.
% The function ensures that PLEXMON is in the 'idle' state before closing down the GUI.
function plexmon_close()

closereq;


% --- Executes when user attempts to close plexmon.
% The function ensures that PLEXMON is in the 'idle' state before closing down the GUI.
function plexmon_CloseRequestFcn(hObject, eventdata, handles)
% hObject    handle to plexmon (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% If PLEXMON is in the running state, tell it to stop!
if( strcmp(handles.plexmon.state, 'running') )
    set(handles.stopPB, 'Enable', 'off');
    handles.plexmon.state = 'stopping';
    guidata(hObject, handles);
end

% And wait up to ~1 second for it to stop before closing the PLEXMON GUI figure!
i = 0;
while( ~strcmp(handles.plexmon.state, 'idle') & (i < 5) )
   i = i+1;
   pause(0.2);
   handles = guidata(gcbo);
end

% This closes the figure.
delete(hObject);


