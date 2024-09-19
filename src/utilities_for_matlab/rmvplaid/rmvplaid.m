function img = rmvplaid(display, aperture, grat1, grat2)
% IMG = RMVPLAID(DISPLAY, APERTURE, GRAT1[, GRAT2])
% Construct an image of an RMVideo grayscale grating or plaid target as it would appear on the screen with the
% specified display parameters. It is a compiled MEX function.
% 
% Arguments:
% DISPLAY : A MATLAB structure holding RMVideo display characteristics:
%     .wpix   -- Width of display background in pixels.
%     .hpix   -- Height of display background in pixels.
%     .wmm    -- Width of display background in mm.
%     .hmm    -- Height of display background in mm.
%     .dmm    -- Distance from eye to center of display along perpendicular line-of-sight (LOS) vector, in mm.
%     .bkg    -- Background grayscale, [0..255].
% 
% APERTURE : A MATLAB structure defining the target aperture and type:
%     .oval   -- Nonzero selects oval aperture; else rectangular.
%     .sine   -- Nonzero selects sinewave gratings; else squarewave.
%     .wdeg   -- Width of aperture in deg subtended at eye.
%     .hdeg   -- Height of aperture in deg subtended at eye.
%     .xsigma -- Standard deviation of Gaussian window in X, in deg subtended at eye. No windowing in X if <= 0.
%     .ysigma -- Standard deviation of Gaussian window in Y, in deg subtended at eye. No windowing in Y if <= 0.
% 
% GRAT1, GRAT2 : MATLAB structures defining parameters for the two grating components of a plaid target. For a 
%  single-grating target, GRAT2 is omitted. Each structure contains the following fields:
%     .mean   -- Mean grayscale level, [0..255].
%     .con    -- Contrast as a percentage, [0..100].
%     .freq   -- Spatial frequency in cycles/deg subtended at eye.
%     .phase  -- Spatial phase in deg.
%     .daxis  -- Drift axis angle in deg CCW. Grating orientation is this angle + 90deg.
% 
% Returns: IMG  A NxM MATLAB UINT8 array holding a generated image of the RMVideo target, where N is the height 
% and M the width of the rectangle bounding the target aperture in pixels. The center of the image is at the 
% center of the matrix, the top-left corner is at (1,1), and the bottom-right is at (N,M). Each element of the 
% matrix gives the grayscale value [0..255] at that pixel location in the image. Use a 256x3 grayscale colormap to 
% visualize image.
% 
% NOTES: 
% 1) rmvplaid() is implemented both as a compiled MEX file in C and as interpreted M-file function. Both have 
% the same name and function prototype, but the MEX function will typically be much faster and takes precedence
% -- when both are present on the MATLAB command path, MATLAB will invoke the MEX file rather than the M-file.
% 2) If an invalid number of arguments is passed, or if the arguments contain bad data, the function aborts, 
% writing an error message to the console.
%
% USAGE EXAMPLE:
%  disp.wpix = 2304;
%  disp.hpix = 1440;
%  disp.wmm = 475;
%  disp.hmm = 305;
%  disp.dmm = 600;
%  disp.bkg = 128;
%  aperture.oval = 1;
%  aperture.sine = 1;
%  aperture.wdeg = 10;
%  aperture.hdeg = 10;
%  aperture.xsigma = 2;
%  aperture.ysigma = 2;
%  grat1.mean = 64;
%  grat1.con = 100;
%  grat1.freq = 1;
%  grat1.phase = 0;
%  grat1.daxis = 45;
%  grat2 = grat1;
%  grat2.freq = 0.5;
%  grat2.daxis = -45;
%  img = rmvplaid(disp, aperture, grat1, grat2);
%  graylut = zeros(256,3);
%  for i=1:256, lum = (i-1)/255; graylut(i,:) = [lum lum lum]; end;
%  colormap(graylut); image(img);
% 
% saruffner. 12May2009.
%

% do we have the required # of input arguments and output arguments?
if(nargin ~= 3 && nargin ~= 4 )
   error( 'Requires 3 or 4 inputs!' );
end

dspWDeg = 2 * atan2(display.wmm / 2.0, display.dmm) * 180 / pi;
dspHDeg = 2 * atan2(display.hmm / 2.0, display.dmm) * 180 / pi;

degPerPixelX = dspWDeg / display.wpix;
degPerPixelY = dspHDeg / display.hpix;

pixPerDeg = (1.0/degPerPixelX + 1.0/degPerPixelY) / 2.0;

% compute rectangular bounds of target aperture and allocate image matrix (uint8 type)
tgtWPix = ceil(pixPerDeg * aperture.wdeg);
tgtHPix = ceil(pixPerDeg * aperture.hdeg);
if(tgtWPix > display.wpix || tgtHPix > display.hpix)
   error('Target bounds exceed specified screen size!');
end
img = zeros(tgtHPix, tgtWPix, 'uint8');

% compute constants for "outside oval" test: x^2 / a^2 + y^2 / b^2 > 1.  Units = pixels^2.
a_sq = (tgtWPix * tgtWPix) / 4.0;
b_sq = (tgtHPix * tgtHPix) / 4.0;
   
% Gaussian windowing constants; note conversion from deg subtended at eye to pixels
doGauss = 0;
if(aperture.xsigma > 0 || aperture.ysigma > 0) 
   doGauss = 1;
end
x_sigSq = 0;
if(aperture.xsigma > 0)
   px = aperture.xsigma * pixPerDeg; 
   x_sigSq = (-0.5) / (px * px);
end
y_sigSq = 0;
if(aperture.ysigma > 0)
   py = aperture.ysigma * pixPerDeg; 
   y_sigSq = (-0.5) / (py * py);
end

% grating parameters converted
phaseRad = [grat1.phase * pi / 180.0 0];
mean = [grat1.mean / 255.0 0];
con = [grat1.con / 100.0 0];
daxisRad = [grat1.daxis * pi / 180.0 0];
pixPerCycle = [pixPerDeg / grat1.freq 0];
if(nargin == 4)
   phaseRad(2) = grat2.phase * pi / 180.0;
   mean(2) = grat2.mean / 255.0;
   con(2) = grat2.con / 100.0;
   daxisRad(2) = grat2.daxis * pi / 180.0;
   pixPerCycle(2) = pixPerDeg / grat2.freq;
end

% compute the two luminance values for a squarewave grating
sqwav_min = zeros(2,1);
sqwav_max = zeros(2,1);
for i=1:2
    sqwav_min(i) = 255.0 * mean(i) * (1 - con(i));
    if(sqwav_min(i) < 0)
        sqwav_min(i) = 0;
    elseif(sqwav_min(i) > 255)
        sqwav_min(i) = 255;
    end
    
    sqwav_max(i) = 255.0 * mean(i) * (1 + con(i));
    if(sqwav_max(i) < 0)
        sqwav_max(i) = 0;
    elseif(sqwav_max(i) > 255)
        sqwav_max(i) = 255;
    end
end

% calculate the luminance at every pixel in the rectangular image...
halfW = tgtWPix / 2.0;
halfH = tgtHPix / 2.0;
nGrats = 1;
if(nargin == 4) 
   nGrats = 2;
end

for x = 0:(tgtWPix-1)
   for y=0:(tgtHPix-1)
      px = x - halfW;
      py = halfH - y;
      px_sq = px * px;
      py_sq = py * py;
      
      % compute mask value at pixel location. If it's less than 0.001, then lum == display bkg
      mask = 1.0;
      if(aperture.oval ~= 0)
         c_sq = px*px / a_sq + py*py / b_sq;
         if(c_sq > 1.0) 
            mask = 0;
         end
      end
      if((mask ~= 0) && (doGauss ~= 0))
         mask = exp(px_sq*x_sigSq + py_sq*y_sigSq);
      end
      if(mask < 0.001)
         img(y+1, x+1) = uint8(display.bkg);
         continue;
      end
      
      % compute and add luminance contributions from each grating.
      lum = 0.0;
      for i=1:nGrats
         gratPix = px;
         if(daxisRad(i) ~= 0)
            alpha = atan2(py, px) - daxisRad(i);
            gratPix = sqrt(px_sq + py_sq) * cos(alpha);
         end
         k = 2*pi*gratPix/pixPerCycle(i) + phaseRad(i);
         if(aperture.sine ~= 0)
            lum = lum + 255.0 * mean(i) * (1.0 + sin(k)) * con(i);
         else
            if(mod(k, 2*pi) < pi)
                lum = lum + sqwav_max(i);
            else
                lum = lum + sqwav_min(i);
            end
         end
      end
      
      % saturation and masking
      if(lum > 255.0) 
         lum = 255.0;
      end
      if(mask < 1.0) 
         lum = display.bkg * (1-mask) + lum * mask;
      end
      
      % store it
      img(y+1, x+1) = uint8(lum);
   end
end
