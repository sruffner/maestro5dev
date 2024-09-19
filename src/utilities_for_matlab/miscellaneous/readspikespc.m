function wave = readspikespc(f)
%READSPIKESPC Extract the 50KHz spike waveform stored in the spikesPC file associated with the Cntrlx data file F.
% This utility function lets you read into Matlab the spike waveform data recorded by spikesPC, a DOS program that
% worked in conjunction with the pre-Maestro program Cntrlx to record the extracellular electrode signal at 50KHz.
%
% The single argument F is the full path name of a Cntrlx data file generated after 29 Jan 2002. The version number must
% be 1. The utility relies on the readcxdata() MEX function to parse the data file and obtain the name of the companion
% spikesPC file, as well as the elapsed time at which recording started in the Cntrlx timeline. The latter is needed for
% trial files, since recording typically begin some time AFTER the start of the trial, yet the spikesPC recording 
% begins at trial start.
%
% The companion spikesPC file must be in the same directory as F.
%
% OUTPUT: An Nx1 vector: The portion of the spikesPC-recorded waveform parsed from the spikesPC file and aligned with
% the recorded timeline of the Cntrlx data file. The sample period for the waveform data is 20 microsecs (50KHz). The 
% data are stored in their raw, 12-bit digitized form [-2048..2047] -- same as would be provided in the 'spikewave'
% field in readcxdata()'s output structure.
%


% some constants
SPIKESPC_HDRSZ = 1024;
SPIKESPC_RECSZ = 16384;
SAMPLEPER_US = 20;
SAMPLESPERMS = 50;
SPIKESPC_UNITREC = 1;

% parse the Cntrlx data file
res = readcxdata(f);
if(~(isstruct(res) && isstruct(res.key)))
   error('Failed to parse Cntrlx data file');
end;
if(~ischar(res.key.spikesFName))
   error('No spikesPC filename included in Cntrlx data file!');
end;

% prepare full path to companion spikesPC file and make sure it's there
[fp, ~, ~] = fileparts(f);
spikesFile = fullfile(fp, res.key.spikesFName);
if(exist(spikesFile, 'file') ~= 2)
   spikesFile = fullfile(fp, upper(res.key.spikesFName));
   if(exist(spikesFile, 'file') ~= 2)
      error('Companion spikesPC file not found!');
   end;
end;

% open spikesPC file and retrieve header. Verify sample period of 20us.
fid = fopen(spikesFile);
if(fid < 0)
   error('Failed to open spikesPC file');
end;
hdrBytes = fread(fid, SPIKESPC_HDRSZ, '*uint8');
nTotalSamples = getInt32(hdrBytes, 1);
sampPer = getInt16(hdrBytes, 9);
if(sampPer ~= SAMPLEPER_US)
   error('Bad header -- Sample period should be 20 microsecs');
end;

% get index of spike waveform sample at which recording started. For Cont mode files, it is assumed to be t=0. For Trial
% mode files, the start time in ms is available in the readcxdata() output. 
iFirstSample = int32(0);
if(bitand(res.key.flags, 1) == 0)
   iFirstSample = int32(res.trialInfo.tRecord) * int32(SAMPLESPERMS);
end;
iFirstSample = iFirstSample + 1;  % Matlab indices start at 1, not zero!

% just in case SPIKESPC stopped recording before Cntrlx started! (should never happen)
if(nTotalSamples <= iFirstSample)
   error('SpikesPC recording stopped before recorded timeline in Cntrlx data file!');
end;


% allocate spike waveform buffer
wave = zeros(nTotalSamples-iFirstSample + 1, 1);

% process all unit trace records in file and decompress spike waveform samples into the waveform buffer
isBrokenSample = false;
i32FirstByte = int32(0);
i32LastSample = int32(0);
nSamplesSaved = 0;
n = int32(1);
while(n <= nTotalSamples)
   rawBytes = fread(fid, SPIKESPC_RECSZ, '*uint8');
   if(rawBytes(1) ~= uint8(SPIKESPC_UNITREC))
      continue;
   end;

   i=5;
   while(i <= SPIKESPC_RECSZ)
      nextByte = int32( bitand(int32(rawBytes(i)), 255) );
      if((i==5) && isBrokenSample)
         % special case: 2-byte compressed sample broken over consecutive records
         sample = int32(bitand(i32FirstByte, 127)) * (2^8);
         sample = bitor(sample, bitand(nextByte, 255));
         sample = sample - 4096;
         i32LastSample = i32LastSample + sample;
         isBrokenSample = false;
      elseif(bitand(nextByte, 128) == 128)
         % start of 2-byte compressed sample. If this is last byte in record, get 2nd byte from next record!
         if(i == SPIKESPC_RECSZ)
            i32FirstByte = nextByte;
            isBrokenSample = true;
            i = i + 1;
            continue;
         else
            i = i + 1;
            sample = int32(bitand(nextByte, 127)) * (2^8);
            nextByte = int32( bitand(int32(rawBytes(i)), 255) );
            sample = bitor(sample, bitand(nextByte, 255));
            sample = sample - 4096;
            i32LastSample = i32LastSample + sample; 
         end;
      else
         % a 1-byte compressed sample
         i32LastSample = i32LastSample + nextByte - int32(64);
      end;
      i = i + 1;

      % add decompressed sample to spike waveform buffer, ignoring samples recorded prior to the start of recording
      % on the Cntrlx side
      if(n >= iFirstSample)
         nSamplesSaved = nSamplesSaved + 1;
         wave(nSamplesSaved) = double(i32LastSample);
      end;

      % stop once we've processed the last sample in spikesPC file (could be anywhere in the last record)
      n = n + 1;
      if(n > nTotalSamples)
         break;
      end;
   end;
end;

fclose(fid);
return;

   %=== getInt32(buf, idx) ============================================================================================
   % Nested function extracts a 32-bit integer from the 4 bytes in BUF[idx:idx+3]. 
   %
   function n = getInt32(buf, idx)
      n = int32(buf(idx));
      n = n + int32( int32(buf(idx+1))*(2^8) );
      n = n + int32( int32(buf(idx+2))*(2^16) );
      n = n + int32( int32(buf(idx+3))*(2^24) );
   end
   %== end getInt32(buf, idx) =========================================================================================

   %=== getInt16(buf, idx) ============================================================================================
   % Nested function extracts a 16-bit integer from the 2 bytes in BUF[idx:idx+1]. 
   %
   function n = getInt16(buf, idx)
      n = int16(buf(idx));
      n = n + int16( int16(buf(idx+1))*(2^8) );
   end
   %== end getInt16(buf, idx) =========================================================================================

end

