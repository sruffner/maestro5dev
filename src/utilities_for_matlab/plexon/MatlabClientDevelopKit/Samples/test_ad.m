% this script tests PL_GetAD (get A/D data) function

% NOTE1: Reading data from up to 256 A/D ("slow") channels is supported; however, 
% please make sure that you are using the latest version of Rasputin (which includes
% support for acquisition from multiple NIDAQ cards in parallel).

% NOTE2: Although Rasputin now allows each A/D card to be run at either "slow" rate 
% (submultiple of 40 kHz, e.g. 10 kHz) or "fast" rate (MAP rate, 40 kHz) if the TIM
% mezzanine board option is installed, PL_GetAD assumes that all A/D cards are run 
% at the same rate; there is no facility for reading data from both "slow" and "fast" 
% rate cards into Matlab simultaneously.  If you need this functionality, or have any
% questions, please contact Plexon.

% before using any of the PL_XXX functions
% you need to call PL_InitClient ONCE
% and use the value returned by PL_InitClient
% in all PL_XXX calls
s = PL_InitClient(0);
if s == 0
   return
end

% get A/D data and plot it
for i=1:10
   [n, t, d] = PL_GetAD(s);
   plot(d);
   pause(1);
end

% you need to call PL_Close(s) to close the connection
% with the Plexon server
PL_Close(s);
s = 0;

