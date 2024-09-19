% this script tests PL_GetNumUnits function

% before using any of the PL_XXX functions
% you need to call PL_InitClient ONCE
% and use the value returned by PL_InitClient
% in all PL_XXX calls
s = PL_InitClient(0);
if s == 0
   return
end

% call PL_GetWF 10 times and plot the waveforms
counts = PL_GetNumUnits(s);

plot(counts');


% you need to call PL_Close(s) to close the connection
% with the Plexon server
PL_Close(s);
s = 0;

