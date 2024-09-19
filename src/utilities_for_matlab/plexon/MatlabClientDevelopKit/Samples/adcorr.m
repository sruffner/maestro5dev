
clear hist
s = PL_InitClient(0);
if s == 0
   return
end


% get A/D data, calculate correlation coefficient, and plot a history of it
plotEvery = 10;
pe = 0
for i=0:1000
    % returns an nchans columns by nsamples rows matrix containing the
    % data collected since the last call
    [n, t, d] = PL_GetAD(s);
    if ( n > 2 ) 
        cc = corrcoef(d);    % produces nchan x nchan matrix
        hist(i) = cc(1,2);   % correlation coeff between channels 1 and 2
        if ( pe == plotEvery )
            plot(hist);
            pe = 0;
        end
    end
    pause(0.01);    % wait a bit
end

% you need to call PL_Close(s) to close the connection
% with the Plexon server
PL_Close(s);
s = 0;

