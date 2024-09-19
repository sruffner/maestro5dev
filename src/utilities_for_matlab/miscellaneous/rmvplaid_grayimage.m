function img = rmvplaid_grayimage(displayinfo, aperture, grat1, grat2)
% Construct an image of an RMVideo grayscale grating or plaid target.
% RMVPLAID_GRAYIMAGE(DISPLAYINFO, APERTURE, GRAT1[, GRAT2]) ....
%
% Arguments (all are required):
%   DISPLAYINFO A MATLAB structure holding RMVideo display characteristics:
%       .wpix   -- Width of display background in pixels.
%       .hpix   -- Height of display background in pixels.
%       .wmm    -- Width of display background in mm.
%       .hmm    -- Height of display background in mm.
%       .dmm    -- Distance from eye to center of display along
%                   perpendicular line-of-sight (LOS) vector, in mm.
%       .bkg    -- Background grayscale, [0..255].
%
%   APERTURE  A MATLAB structure defining the target aperture and type:
%       .oval   -- Nonzero selects oval aperture; else rectangular.
%       .sine   -- Nonzero selects sinewave gratings; else squarewave.
%       .wdeg   -- Width of aperture in deg subtended at eye.
%       .hdeg   -- Height of aperture in deg subtended at eye.
%       .xsigma -- Standard deviation of Gaussian window in X, in deg
%                   subtended at eye. Non-positive value is treated as
%                   infinite standard deviation (no windowing in X).
%       .ysigma -- Standard deviation of Gaussian window in Y, in deg
%                   subtended at eye. Non-positive value is treated as
%                   infinite standard deviation (no windowing in Y).
%
%   GRAT1, GRAT2 MATLAB structures defining parameters for the two grating
%   components of a plaid target. For a single-grating target, GRAT2 is
%   omitted. Each structure contains the following fields:
%       .mean   -- Mean grayscale level, [0..255].
%       .con    -- Contrast as a percentage, [0..100].
%       .freq   -- Spatial frequency in cycles/deg subtended at eye.
%       .phase  -- Spatial phase in deg.
%       .daxis  -- Drift axis angle in deg CCW. Grating orientation is 
%                   this angle + 90deg.
%
% Returns:
% A NxM MATLAB array holding a generated image of the RMVideo target, where
% N is the height and M the width of the target aperture in pixels. Each 
% element of the array corresponds to the grayscale value [0..255] at that
% pixel location in the image.
%

% do we have the required # of input arguments and output arguments?
if(nargin ~= 3 && nargin ~= 4 )
   error( 'Requires 3 or 4 inputs!' );
end

% image initially an empty matrix -- indicating an error
img = [];

dspWDeg = 2 * atan2(displayinfo.wmm / 2.0, displayinfo.dmm) * 180 / pi;
dspHDeg = 2 * atan2(displayinfo.hmm / 2.0, displayinfo.dmm) * 180 / pi;

degPerPixelX = dspWDeg / displayinfo.wpix;
degPerPixelY = dspHDeg / displayinfo.hpix;

pixPerDeg = (1.0/degPerPixelX + 1.0/degPerPixelY) / 2.0;

% for now, see if we can compute image for a sinewave grating or plaid
% at any orientation.
tgtWPix = ceil(pixPerDeg * aperture.wdeg);
tgtHPix = ceil(pixPerDeg * aperture.hdeg);
img = zeros(tgtHPix, tgtWPix);

phaseRad = grat1.phase * pi / 180;
mean = grat1.mean / 255.0;
con = grat1.con / 100.0;
pixPerCycle = pixPerDeg / grat1.freq;
if(grat1.daxis == 0)
   ctr = tgtWPix / 2;
   x = 2*pi*((0:tgtWPix-1) - ctr)/pixPerCycle + phaseRad;
   row = floor(255.0 * (mean * (1.0 + sin(x)) * con) + 0.5) + 1;

   for i=1:tgtHPix 
       img(i,:) = row; 
   end;
else
    for x=1:tgtWPix
       for y=1:tgtHPix
        px = x - tgtWPix/2;
        py = tgtHPix/2 - y;
        alpha = atan2(py, px) * 180 / pi - grat1.daxis;
        gratPix = sqrt(px*px + py*py) * cosd(alpha);
        k = 2*pi*gratPix/pixPerCycle + phaseRad;
        img(y,x) = floor(255.0 * mean * (1.0 + sin(k)) * con + 0.5) + 1;
       end;
    end;
end;

% here we add in the second grating component
if(nargin == 4)
   phaseRad = grat2.phase * pi / 180;
   mean = grat2.mean / 255.0;
   con = grat2.con / 100.0;
   pixPerCycle = pixPerDeg / grat2.freq;
   if(grat2.daxis == 0)
      ctr = tgtWPix / 2;
      x = 2*pi*((0:tgtWPix-1) - ctr)/pixPerCycle + phaseRad;
      row = floor(255.0 * (mean * (1.0 + sin(x)) * con) + 0.5) + 1;

      for i=1:tgtHPix 
          img(i,:) = img(i,:) + row; 
      end;
   else
      for x=1:tgtWPix
         for y=1:tgtHPix
            px = x - tgtWPix/2;
            py = tgtHPix/2 - y;
            alpha = atan2(py, px) * 180 / pi - grat2.daxis;
            gratPix = sqrt(px*px + py*py) * cosd(alpha);
            k = 2*pi*gratPix/pixPerCycle + phaseRad;
            img(y,x) = img(y,x) + floor(255.0 * mean * (1.0 + sin(k)) * con + 0.5) + 1;
         end;
      end;
   end;
end;

% if oval aperture or Gaussian windowing in effect, compute and apply 
% alpha mask to the image rect. Since the mask color is 0, the blending 
% function is img(y,x) = (1-y
if(aperture.oval ~= 0 || aperture.xsigma > 0 || aperture.ysigma > 0)
    mask = computeMask(tgtWPix, tgtHPix, aperture.oval, aperture.xsigma*pixPerDeg, aperture.ysigma*pixPerDeg);
    for x=1:tgtWPix
        for y=1:tgtHPix
            img(y,x) = displayinfo.bkg * (1-mask(y,x)) + img(y,x) * mask(y,x);
        end;
    end;
end;

% limit all pixels to [1..256]
for x=1:tgtWPix
    for y=1:tgtHPix
        if(img(y,x) < 1) 
            img(y,x) = 1;
        elseif(img(y,x) > 256) 
            img(y,x) = 256;
        end;
    end;
end;

return;

% w, h, xsig, ysig in pixels!
function mask = computeMask(w, h, oval, xsig, ysig) 
mask = [];
if(oval == 0 && xsig == 0 && ysig == 0)
    return;
end;
doGauss = 0;
if(xsig > 0 || ysig > 0)
    doGauss = 1;
end;
mask = zeros(h,w);
a_sq = w * w / 4;
b_sq = h * h / 4;
x_negInv2SigSq = 0;
y_negInv2SigSq = 0;
if(xsig > 0)
   x_negInv2SigSq = (-0.5) / (xsig * xsig);
end;
if(ysig > 0)
   y_negInv2SigSq = (-0.5) / (ysig * ysig);
end;

for x=1:w
    for y=1:h
        maskval = 1;
        px = x - w/2;
        py = h/2 - y;
        px_sq = px*px;
        py_sq = py*py;
        if(oval ~= 0)
            c = px*px / a_sq + py*py / b_sq;
            if(c > 1) 
                maskval = 0;
            end;
        end;
        
        if(maskval == 1 && doGauss == 1)
            maskval = exp(px_sq*x_negInv2SigSq + py_sq*y_negInv2SigSq);
        end;
        
        mask(y,x) = maskval;
    end;
end;