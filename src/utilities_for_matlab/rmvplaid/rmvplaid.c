//=====================================================================================================================
//
// rmvplaid.c --   Implementation of MATLAB MEX function rmvplaid(), which generates an image of an RMVideo plaid or
//		   grating target.
//
// AUTHOR:  saruffner.
//
// IMG = RMVPLAID(DISPLAY, APERTURE, GRAT1[, GRAT2])
// Construct an image of an RMVideo grayscale grating or plaid target as it would appear on the screen with the
// specified display parameters.
//
// Arguments (all are required):
// DISPLAY : A MATLAB structure holding RMVideo display characteristics:
//      .wpix   -- Width of display background in pixels.
//      .hpix   -- Height of display background in pixels.
//      .wmm    -- Width of display background in mm.
//      .hmm    -- Height of display background in mm.
//      .dmm    -- Distance from eye to center of display along perpendicular line-of-sight (LOS) vector, in mm.
//      .bkg    -- Background grayscale, [0..255].
//
// APERTURE : A MATLAB structure defining the target aperture and type:
//      .oval   -- Nonzero selects oval aperture; else rectangular.
//      .sine   -- Nonzero selects sinewave gratings; else squarewave.
//      .wdeg   -- Width of aperture in deg subtended at eye.
//      .hdeg   -- Height of aperture in deg subtended at eye.
//      .xsigma -- Standard deviation of Gaussian window in X, in deg subtended at eye. No windowing in X if <= 0.
//      .ysigma -- Standard deviation of Gaussian window in Y, in deg subtended at eye. No windowing in Y if <= 0.
//
// GRAT1, GRAT2 : MATLAB structures defining parameters for the two grating components of a plaid target. For a 
// single-grating target, GRAT2 is omitted. Each structure contains the following fields:
//      .mean   -- Mean grayscale level, [0..255].
//      .con    -- Contrast as a percentage, [0..100].
//      .freq   -- Spatial frequency in cycles/deg subtended at eye.
//      .phase  -- Spatial phase in deg.
//      .daxis  -- Drift axis angle in deg CCW. Grating orientation is this angle + 90deg.
//
// Returns: IMG : A NxM MATLAB UINT8 array holding a generated image of the RMVideo target, where N is the height and 
// M the width of the rectangle bounding the target aperture in pixels. Each element of the array corresponds to the 
// grayscale value [0..255] at that pixel location in the image. Use a 256x3 grayscale colormap to visualize image.
// 
// If an invalid number of arguments is passed, or if the arguments contain bad data, the function aborts, printing an
// error message to the console.
// 
// USAGE EXAMPLE:
// disp.wpix = 2304;
// disp.hpix = 1440;
// disp.wmm = 475;
// disp.hmm = 305;
// disp.dmm = 600;
// disp.bkg = 128;
// aperture.oval = 1;
// aperture.sine = 1;
// aperture.wdeg = 10;
// aperture.hdeg = 10;
// aperture.xsigma = 2;
// aperture.ysigma = 2;
// grat1.mean = 64;
// grat1.con = 100;
// grat1.freq = 1;
// grat1.phase = 0;
// grat1.daxis = 45;
// grat2 = grat1;
// grat2.freq = 0.5;
// grat2.daxis = -45;
// img = rmvplaid(disp, aperture, grat1, grat2);
// graylut = zeros(256,3);
// for i=1:256, lum = (i-1)/255; graylut(i,:) = [lum lum lum]; end;
// colormap(graylut); image(img);
//
//=====================================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "mex.h"

//=====================================================================================================================
// MODULE GLOBALS, CONSTANTS
//=====================================================================================================================

typedef struct tagInfo
{
   double wpix, hpix;               // screen dimensions in pixels
   double wmm, hmm, dmm;            // screen dimensions and LOS distance from eye, in mm
   double bkg;                      // grayscale luminance of screen background
   bool oval, sine, plaid;          // oval vs rectangular; sinewave vs squarewave; grating vs plaid
   double wdeg, hdeg;               // dimensions of aperture bounding rectangle, in deg subtended at eye
   double xsigma, ysigma;           // standard deviations in X and Y for 2D Gaussian window
   double mean[2];                  // mean luminance for gratings 1 & 2, scaled from [0..255] to [0..1]
   double con[2];                   // contrast for gratings 1 & 2, scaled from [0..100] to [0..1]
   double freq[2];                  // spatial frequency for gratings 1 & 2 in cyc/deg subtended at eye
   double phase[2];                 // spatial phase offset for gratings 1 & 2, in RADIANS!!
   double daxis[2];                 // drift axis angle for gratings 1 & 2 in RADIANS CCW
   double pixPerCycle[2];           // num pixels spanning one cycle of grating 1,2
} INFO, *PINFO;

// information culled from function arguments that's needed to compute the image
INFO info;

// value of PI
double dPi;

//=====================================================================================================================
// FUNCTIONS DEFINED IN THIS MODULE
//=====================================================================================================================
void parseArguments(int nrhs, const mxArray *prhs[]);


//=== mexFunction (readcxdata) ========================================================================================
//
//    See file header for function prototype.
//
//    ARGS:       nlhs, plhs  -- [out] array output ("left-hand side") containing data/info in the file.  We EXPECT
//                               nlhs==1, since readcxdata() returns everything in a single data structure as described
//                               in the outputFields structure.
//                nrhs, prhs  -- [in] array input.  See above.
//
//    RETURNS:    NONE.  If a fatal error occurs while processing the file, an error message is printed to STDOUT and
//                a partially completed (possibly) image is returned.
//
void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] )
{
   double dW, dH, dPixPerDeg;
   double a_sq, b_sq, c_sq;               // for oval aperture calcs; unit = pixel^2
   double x_sigSq, y_sigSq;               // for Gaussian window calcs: -1 / (2 * x_sig^2) in pixel^(-2)
   double px, py;                         // pixel coords in image, where (0,0) is at image center and max Y on top row
   double lum, mask, px_sq, py_sq;        // for grating and windowing calcs
   double k, gratPix, alpha;
   int i, nGrats;
   uint32_T tgtW, tgtH, x, y;
   uint8_T* pbImage;
   bool doGauss;                          // true if Gaussian windowing required
   
   // compute value of pi
   dPi = atan2(+0, -1);
   
   // check, parse arguments
   if(nlhs > 1) mexErrMsgTxt("Too many output arguments!");
   parseArguments(nrhs, prhs);
   info.plaid = (nrhs == 4) ? true : false;
   
   // compute pixels per deg subtended at eye
   dW = 2.0 * atan2(info.wmm/2.0, info.dmm) * 180.0 / dPi;
   dH = 2.0 * atan2(info.hmm/2.0, info.dmm) * 180.0 / dPi;
   dW /= info.wpix;
   dH /= info.hpix;
   dPixPerDeg = (1.0/dW + 1.0/dH) / 2.0;
   
   // convert selected grating parameters and compute pix-per-cycle
   for(i = 0; i < ((info.plaid) ? 2 : 1); i++)
   {
      info.mean[i] /= 255.0;
      info.con[i] /= 100.0;
      info.phase[i] *= (dPi / 180.0);
      info.daxis[i] *= (dPi / 180.0);
      info.pixPerCycle[i] = dPixPerDeg / info.freq[i];
   }
   
   // compute rectangular bounds of target aperture and allocate image matrix (uint8 type)
   tgtW = (uint32_T) ceil(dPixPerDeg * info.wdeg);
   tgtH = (uint32_T) ceil(dPixPerDeg * info.hdeg);
   if(tgtW > info.wpix || tgtH > info.hpix)
      mexErrMsgTxt("Target bounds exceed specified screen size!");
   dW = ((double)tgtW) / 2.0;
   dH = ((double)tgtH) / 2.0;
   
   plhs[0] = mxCreateNumericMatrix(tgtH, tgtW, mxUINT8_CLASS, mxREAL);
   pbImage = (uint8_T*) mxGetPr(plhs[0]);

   // compute constants for "outside oval" test: x^2 / a^2 + y^2 / b^2 > 1.  Units = pixels^2.
   a_sq = ((double) tgtW * tgtW) / 4.0;
   b_sq = ((double) tgtH * tgtH) / 4.0;
   
   // Gaussian windowing constants; note conversion from deg subtended at eye to pixels
   doGauss = (info.xsigma > 0 || info.ysigma > 0) ? true : false;
   x_sigSq = 0;
   if(info.xsigma > 0)
   {
      px = info.xsigma * dPixPerDeg; 
      x_sigSq = (-0.5) / (px * px);
   }
   y_sigSq = 0;
   if(info.ysigma > 0)
   {
      py = info.ysigma * dPixPerDeg; 
      y_sigSq = (-0.5) / (py * py);
   }


   // calculate the luminance at every pixel in the rectangular image...
   nGrats = info.plaid ? 2 : 1;
   for(x = 0; x < tgtW; x++)
   {
      for(y = 0; y < tgtH; y++)
      {
         px = x - dW;
         py = dH - y;
         px_sq = px * px;
         py_sq = py * py;
         
         // compute mask value at pixel location. If it's less than 0.001, then lum == display bkg
         mask = 1.0;
         if(info.oval)
         {
            c_sq = px*px / a_sq + py*py / b_sq;
            if(c_sq > 1.0) mask = 0;
         }
         if(mask > 0 && doGauss)
            mask = exp(px_sq*x_sigSq + py_sq*y_sigSq);
         if(mask < 0.001)
         {
            // set lum at pixel to bkg
            pbImage[x*tgtW + y] = (uint8_T) info.bkg;
            continue;
         }
         
         // compute and add luminance contributions from each grating.
         lum = 0.0;
         for(i=0; i<nGrats; i++)
         {
            gratPix = px;
            if(info.daxis[i] != 0)
            {
               alpha = atan2(py, px) - info.daxis[i];
               gratPix = sqrt(px_sq + py_sq) * cos(alpha);
            }
            k = 2*dPi*gratPix/info.pixPerCycle[i] + info.phase[i];
            if(info.sine)
               lum += 255.0 * info.mean[i] * (1.0 + sin(k)) * info.con[i];
            else
            {
               k = fmod(k, 2*dPi);
               if(k < 0) k += 2*dPi;
               if(k < dPi) lum += 255.0 * info.mean[i] * (1.0 + info.con[i]);
               else lum += 255.0 * info.mean[i] * (1.0 - info.con[i]);
            }
         }
         
         // saturation and masking
         if(lum < 0) lum = 0;
         if(lum > 255.0) lum = 255.0;
         if(mask < 1.0) lum = info.bkg * (1-mask) + lum * mask;
         if(lum > 255.0) lum = 255.0;
         
         // store it
         pbImage[x*tgtW + y] = (uint8_T) lum;
      }
   }
}

// parses the RHS arguments and initializes module global 'info' accordingly. Returns to MATLAB with error msg if
// something is amiss
void parseArguments(int nrhs, const mxArray *prhs[])
{
   int i, nGrats;
   mxArray* pMXField;
   
   if(nrhs < 3 || nrhs > 4) mexErrMsgTxt("Invalid number of output arguments!");
   
   // from the 'display' structure argument
   pMXField = mxGetField(prhs[0], 0, "wpix");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'wpix' in 'display' argument!");
   else info.wpix = mxGetScalar(pMXField);
   
   pMXField = mxGetField(prhs[0], 0, "hpix");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'hpix' in 'display' argument!");
   else info.hpix = mxGetScalar(pMXField);
   
   pMXField = mxGetField(prhs[0], 0, "wmm");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'wmm' in 'display' argument!");
   else info.wmm = mxGetScalar(pMXField);
   
   pMXField = mxGetField(prhs[0], 0, "hmm");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'hmm' in 'display' argument!");
   else info.hmm = mxGetScalar(pMXField);
   
   pMXField = mxGetField(prhs[0], 0, "dmm");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'dmm' in 'display' argument!");
   else info.dmm = mxGetScalar(pMXField);
   
   pMXField = mxGetField(prhs[0], 0, "bkg");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'bkg' in 'display' argument!");
   else info.bkg = mxGetScalar(pMXField);
   
   if(info.wpix < 600 || info.wpix > 4000 || info.hpix < 400 || info.hpix > 3000 ||
      info.wmm < 100 || info.wmm > 10000 || info.hmm < 100 || info.hmm > 10000 ||
      info.dmm < 100 || info.dmm > 10000 || info.bkg < 0 || info.bkg > 255)
   {
      mexErrMsgTxt("Unreasonable display/screen characteristics!");
   }
   
   // from the 'aperture' structure argument
   pMXField = mxGetField(prhs[1], 0, "oval");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'oval' in 'aperture' argument!");
   else info.oval = (0 != mxGetScalar(pMXField)) ? true : false;
   
   pMXField = mxGetField(prhs[1], 0, "sine");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'sine' in 'aperture' argument!");
   else info.sine = (0 != mxGetScalar(pMXField)) ? true : false;
   
   pMXField = mxGetField(prhs[1], 0, "wdeg");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'wdeg' in 'aperture' argument!");
   else info.wdeg = mxGetScalar(pMXField);
   
   pMXField = mxGetField(prhs[1], 0, "hdeg");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'hdeg' in 'aperture' argument!");
   else info.hdeg = mxGetScalar(pMXField);
   
   pMXField = mxGetField(prhs[1], 0, "xsigma");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'xsigma' in 'aperture' argument!");
   else info.xsigma = mxGetScalar(pMXField);
   
   pMXField = mxGetField(prhs[1], 0, "ysigma");
   if(pMXField == NULL) mexErrMsgTxt("Missing field 'ysigma' in 'aperture' argument!");
   else info.ysigma = mxGetScalar(pMXField);

   if(info.wdeg <= 0 || info.wdeg > 90 || info.hdeg <= 0 || info.hdeg > 90)
   {
      mexErrMsgTxt("Unreasonable target aperture characteristics!");
   }
   
   // from the 'grat1' and 'grat2' structure arguments
   nGrats = (nrhs == 4) ? 2 : 1;
   for(i = 0; i < nGrats; i++)
   {
      pMXField = mxGetField(prhs[i+2], 0, "mean");
      if(pMXField == NULL) mexErrMsgTxt("Missing field 'mean' in 'grat1' or 'grat2!");
      else info.mean[i] = mxGetScalar(pMXField);
   
      pMXField = mxGetField(prhs[i+2], 0, "con");
      if(pMXField == NULL) mexErrMsgTxt("Missing field 'con' in 'grat1' or 'grat2!");
      else info.con[i] = mxGetScalar(pMXField);
   
      pMXField = mxGetField(prhs[i+2], 0, "freq");
      if(pMXField == NULL) mexErrMsgTxt("Missing field 'freq' in 'grat1' or 'grat2!");
      else info.freq[i] = mxGetScalar(pMXField);
   
      pMXField = mxGetField(prhs[i+2], 0, "phase");
      if(pMXField == NULL) mexErrMsgTxt("Missing field 'phase' in 'grat1' or 'grat2!");
      else info.phase[i] = mxGetScalar(pMXField);
   
      pMXField = mxGetField(prhs[i+2], 0, "daxis");
      if(pMXField == NULL) mexErrMsgTxt("Missing field 'daxis' in 'grat1' or 'grat2!");
      else info.daxis[i] = mxGetScalar(pMXField);
   
      if(info.mean[i] < 0 || info.mean[i] > 255 || info.con[i] < 0 || info.con[i] > 100 ||
         info.freq[i] < 0.1 || info.freq[i] > 10)
      {
         mexPrintf("Grating #%d : ", i+1);
         mexErrMsgTxt("Unreasonable parameter found");
      }
   }
}

