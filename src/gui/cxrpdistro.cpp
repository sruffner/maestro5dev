//=====================================================================================================================
//
// cxrpdistro.cpp : Implementation of class CCxRPDistro, which encapsulates information for a CCxTrial object that
//                  includes a distribution-based reward/penalty contingency.
//
// AUTHOR:  saruffner
//
// DESCRIPTION:
// Maestro v1.4 introduced a special scheme for rewarding or penalizing the subject based on their behavioral response
// to a particular stimulus paradigm. Initially, the behavioral response was the eye velocity magnitude. As of v2.1.1, 
// the experimenter has a choice from among four different aspects of eye velocity: horizontal eye velocity, vertical 
// eye velocity, eye velocity magnitude, or eye velocity direction. The basic idea behind the scheme is to attempt to
// alter the *distribution* of responses through motivational techniques. A new special operation in Trial mode, called
// "R/P Distro", implements this protocol, which has two distinct phases. In the "measurement phase", the trial is
// presented repeatedly (likely mixed in with other trials) and the selected behavioral response is averaged over the
// designated special segment. The accumulated response samples form a "response distribution". The researcher can view
// this distribution on the Maestro GUI, then define a "reward window" spanning some portion of the distribution.
//
// With a reward window defined, the protocol enters the "reward/penalty phase". The trial is again presented 
// repeatedly and the response is averaged over the special segment as before. This time, however, if the response
// falls within the reward window, the subject receives a small mid-trial reward at the end of that segment, plus the 
// usual reward at trial's end. If the response is outside the reward window, the subject is "penalized": no mid-trial 
// reward, and a reduced reward at trial's end. [The differential end-of-trial rewards are specified by using different
// pulse lengths for the two reward pulses specified in a trial definition: reward pulse 2 is used as the mid-trial 
// reward and as the end-of-trial reward when the subject's response is outside the response window. Reward pulse 1 is 
// used as the end-of-trial reward when the response is in the window. Obviousl, pulse 1 should be significantly 
// larger than pulse 2.] A new response distribution is built up with repeated presentations of the trial. The 
// researcher can view both the "before" and "after" distributions on the GUI to see if any change has occurred.
//
// Furthermore, the reward window can be "dynamic" -- Maestro can shift it in a "preferred" direction if the subject's
// mean response shifts in that direction after a set number of presentations. The window is defined by several
// parameters: the minimum and maximum window bounds [Rmin..Rmax],, the number N of response samples collected before
// the window is "updated", and the window shift D. Each time N valid response samples are recorded during the reward
// phase, Maestro will compute the mean response sample M. If D>0 and Rmin<M, then the entire window is shifted by D:
// [Rmin..Rmax] -> [Rmin+D .. Rmax+D]. If D<0, the shift criterion is M<Rmax. If the shift would put the reward window
// partially outside the valid response range, then the response range is broadened to include the new reward window 
// bounds. Note that if D=0 or N=0, dynamic updating is disabled.
//
// This two-phased contingency protocol demands that Maestro store the "before" and "after" distributions, provide a
// means of selecting the response measure to use, define the reward window and the valid response range, and keep a 
// count of how many times the measured response falls within the reward window vs how many times it does not.
//
// CCxRPDistro is a helper class that encapsulates this information for a CCxTrial object. It stores response samples
// for a "current" and "previous" distribution, as well the response measure type, the valid response range, reward 
// window parameters, etc. As response samples are added to the object, it recalcuates the sample mean and standard 
// deviation of the "current" distribution. During the "reward" phase, it updates reward/penalty statistics and handles
// adjustment of the "dynamic" reward window.  Response samples, valid response range, and reward window parameters are 
// assumed to be specified in the same units (deg/sec for eye velocity magnitude, deg CCW for eye velocity direction)
// and are maintained in single-precision floating-point.
//
// REVISION HISTORY:
// 28nov2005-- Began development.
// 14dec2005-- Added concept of a "valid response range".  Response samples falling outside this range are ignored for
//             the purposes of building the distribution, but they are still tallied as pass/fail if a reward window
//             is defined.
// 21dec2005-- Added GetTextSummary().
// 11mar2006-- Based on initial usage, we're making some major revisions to the R/P Distro feature...
//             (1) Replaced the two reward windows with a single "dynamic" reward window defined by the following
//             properties: enable flag, center, width, update delta, and update interval N.  After N valid samples are
//             collected, Maestro will compute the mean response.  If the update delta is nonzero and the difference
//             between the window center and the computed mean has the same sign as the delta, then "center" becomes
//             center+delta, and the interval counter is reset to 0.
//             (2) Added integers specifying the number of most recent valid samples to account for when computing the
//             mean, standard deviation, and histograms for both the current and previous distributions.
// 16mar2006-- More mods: (1) Changed back to defining the reward window by min/max bounds.  (2) The criterion for
//             "dynamic shift" is slightly different:  If shift is positive, shift window if the mean over the N valid
//             responses is greater than the minimum bound of the reward window.  Conversely, if shift is negative,
//             shift window if the mean is less than the maximum bound of the reward window.
// 25apr2007-- Began mods to introduce four alternative behavioral response measures: H eye velocity, V eye velocity, 
//             eye velocity magnitude, eye velocity direction. Units are deg/sec for eye velocity; for direction, they 
//             are deg CCW from rightward motion [0..360).
// 05sep2017-- Fix compiler issues while compiling for 64-bit Win 10 using VStudio 2017.
//=====================================================================================================================

#include "stdafx.h"                          // standard MFC stuff
#include "math.h"

#include "util.h"                            // for cMath utility class
#include "cxobj_ifc.h"                       // for TH_RPD_*** defines
#include "cxrpdistro.h"



//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================

CCxRPDistro::CCxRPDistro()
{
   Reset();
}

CCxRPDistro::~CCxRPDistro()
{
   Reset();
}



//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================

//=== Reset ===========================================================================================================
//
//    Clears the state of this CCxRPDistro object:  both previous and current distributions cleared of any samples,
//    reward window is disabled, and any statistics are zeroed. The response measure is eye velocity magnitude.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID CCxRPDistro::Reset()
{
   m_iRespType = TH_RPD_EYEVEL;

   m_currSamples.RemoveAll();
   m_fCurrMean = 0.0f;
   m_fCurrStdDev = 0.0f;
   m_nCurrValidSamples = 0;

   m_prevSamples.RemoveAll();
   m_fPrevMean = 0.0f;
   m_fPrevStdDev = 0.0f;
   m_nPrevValidSamples = 0;

   m_nCurrMostRecent = 0;
   m_nPrevMostRecent = 0;

   m_bRewEnable = FALSE;
   m_fRewMin = 2.5f;
   m_fRewMax = 7.5f;
   m_fRewShift = 0.0f;
   m_nUpdateIntv = 10;
   m_nSampleCount = 0;

   m_fRespMin = 0.0f;
   m_fRespMax = 10.0f;

   m_nTries = 0;
   m_nPassed = 0;
   m_iLastResult = -1;
}

//=== StartNewDistribution ============================================================================================
//
//    Prepare to collect samples for a new distribution.  The current distribution is copied to the previous
//    distribution (writing over the "old" previous distribution), then it is reset.  Statistics also reset.
//
//    ARGS:       NONE.
//    RETURNS:    NONE.
//
VOID CCxRPDistro::StartNewDistribution()
{
   m_prevSamples.RemoveAll();
   m_prevSamples.Append( m_currSamples );
   m_fPrevMean = m_fCurrMean;
   m_fPrevStdDev = m_fCurrStdDev;
   m_nPrevValidSamples = m_nCurrValidSamples;

   // NOTE: We do NOT copy m_nCurrMostRecent to m_nPrevMostRecent.  These are set by the experimenter!  Therefore we
   // must recalc stats for the (new) previous distribution!
   if( m_nPrevMostRecent != m_nCurrMostRecent )
      Recalc( m_prevSamples, m_nPrevMostRecent, m_nPrevValidSamples, m_fPrevMean, m_fPrevStdDev );

   m_currSamples.RemoveAll();
   m_fCurrMean = 0.0f;
   m_fCurrStdDev = 0.0f;
   m_nCurrValidSamples = 0;

   m_nTries = 0;
   m_nPassed = 0;
   m_iLastResult = -1;
}

//=== AddSample =======================================================================================================
//
//    Add a sample to the current distribution being compiled, then recalculate its mean and standard deviation.  If
//    the reward window is enabled, we also update reward statistics:  total #samples collected and #samples falling
//    within the reward window.
//
//    IF the sample falls outside the "valid" response range defined on this CCxRPDistro object, the sample is saved,
//    but it is not included in the distribution stats (the response range can be later changed to encompass the
//    ignored sample OR to exclude additional samples!).  However, if the reward window is enabled, the response sample
//    is tallied as a pass or fail regardless.
//
//    IF the reward window is dynamic and we've collected the required number of valid response samples, we compute
//    the mean M over those samples.  Then, for shift value D and current reward window bounds [Rmin..Rmax]:
//       1) If D>0:  If M>Rmin, then shift window to [Rmin+D..Rmax+D].
//       2) If D<0:  If M<Rmax, then shift window to [Rmin+D..Rmax+D].
//    If necessary, the valid response range is broadened to contain the new reward window bounds.
//
//    ARGS:       fVal  -- [in] the sample value.
//    RETURNS:    NONE.
//
VOID CCxRPDistro::AddSample( float fVal )
{
   // add sample to the current distribution
   m_currSamples.Add( fVal );

   // if sample falls within valid response range, recalc stats
   BOOL isValid = BOOL(fVal >= m_fRespMin && fVal <= m_fRespMax);
   if( isValid )
      Recalc( m_currSamples, m_nCurrMostRecent, m_nCurrValidSamples, m_fCurrMean, m_fCurrStdDev );

   // if reward window is enabled...
   if( IsRewardWinEnabled() )
   {
      // update reward stats if reward window defined -- whether or not sample falls within valid response range!
      ++m_nTries;
      m_iLastResult = 0;
      if( fVal >= m_fRewMin && fVal <= m_fRewMax )
      {
         ++m_nPassed;
         m_iLastResult = 1;
      }

      // if reward window is dynamic and response sample is valid, update dynamic state...
      if( (m_fRewShift != 0.0f) && (m_nUpdateIntv > 0) && isValid)
      {
         ++m_nSampleCount;
         if( m_nSampleCount >= m_nUpdateIntv )
         {
            // we need mean calc'd over the last "update intv" valid samples.  We already have it IF update intv = the
            // "N most recent" for the current distribution.  If not, we have to calc it.
            float fMeanResp = m_fCurrMean;
            if( m_nUpdateIntv != m_nCurrMostRecent )
            {
               int nValid = 0;
               float fStd = 0.0f;
               Recalc( m_currSamples, m_nUpdateIntv, nValid, fMeanResp, fStd );
            }

            // if shift in mean response relative to min or max bound is in the right direction, then shift window
            if( (m_fRewShift>0.0f && fMeanResp>m_fRewMin) || (m_fRewShift<0.0f && fMeanResp<m_fRewMax) )
            {
               m_fRewMin += m_fRewShift;
               m_fRewMax += m_fRewShift;

               // if necessary, broaden the valid response range so that it still encompasses reward window
               if( m_fRewShift > 0.0f )
               {
                  if( m_fRewMax > m_fRespMax ) m_fRespMax = m_fRewMax + 1.0f;
               }
               else
               {
                  if( m_fRewMin < m_fRespMin ) m_fRespMin = m_fRewMin - 1.0f;
               }
            }

            // reset our sample counter!
            m_nSampleCount = 0;
         }
      }
   }
   else
      m_iLastResult = -1;
}

//=== Get/SetResponseType ============================================================================================= 
//
//    Get/set the type of behavioral response measure monitored during the RP Distro operation. Four possible response 
//    measures are supported, as defined by these constants in CXOBJ_IFC.H:
//       TH_RPD_EYEVEL : Eye velocity vector magnitude in deg/sec.
//       TH_RPD_HEVEL  : Horizontal eye velocity in deg/sec.
//       TH_RPD_VEVEL  : Vertical eye velocity in deg/sec.
//       TH_RPD_EYEDIR : Eye velocity vector direction in deg CCW (0 deg = rightward motion).
//
//    If the 'type' argument specified in SetResponseType() is not one of these, the method has no effect. If the 
//    response type is changed, then both the current and previous distributions are cleared since they would no longer 
//    contain response data of the appropriate type!
// 
int CCxRPDistro::GetResponseType()
{
   return(m_iRespType);
}

VOID CCxRPDistro::SetResponseType(int type)
{
   if(type == m_iRespType || type < TH_RPD_EYEVEL || type > TH_RPD_EYEDIR) return;
   
   m_iRespType = type;
   StartNewDistribution();    // calling this twice resets both distributions!
   StartNewDistribution();
}

//=== GetResponseTypeDesc =============================================================================================
//
//    Get a short name for the specified RP Distro behavioral response measure, suitable for use in the GUI.
//
LPCTSTR CCxRPDistro::GetResponseTypeDesc(int type)
{
   static LPCTSTR TYPENAMES[] = {"Eye Speed", "H Eye Speed", "V Eye Speed", "Eye Motion Dir"};
   
   if(type < TH_RPD_EYEVEL || type > TH_RPD_EYEDIR) return("");
   else return(TYPENAMES[type]);
}

//=== GetTotalCurrentSamples, GetNumValidCurrentSamples ===============================================================
//
//    Get the total number of response samples in the current distribution being compiled, and the number of those
//    which fall within the valid response range.
//
//    ARGS:       NONE.
//    RETURNS:    Total #samples, or # of "valid" samples.
//
int CCxRPDistro::GetTotalCurrentSamples()
{
   return( static_cast<int>(m_currSamples.GetSize()) );
}

int CCxRPDistro::GetNumValidCurrentSamples()
{
   return( m_nCurrValidSamples );
}

//=== GetCurrentSample ================================================================================================
//
//    Retrieve a particular sample from the current distribution being compiled.
//
//    ARGS:       i  -- index of requested sample.
//    RETURNS:    The sample value, or zero if index is invalid.
//
float CCxRPDistro::GetCurrentSample( int i )
{
   int n = GetTotalCurrentSamples();
   return( (i<0 || i>=n) ? 0.0f : m_currSamples[i] );
}

//=== GetCurrentMean ==================================================================================================
//
//    Get sample mean for the current distribution being compiled.  This is the mean computed over the N most recent
//    valid samples, where N is returned by GetCurrentNumMostRecent().  If N < 2, all valid samples are included.
//
//    ARGS:       NONE.
//    RETURNS:    Mean value of samples collected in the current distribution.  If no samples yet, zero is returned.
//
float CCxRPDistro::GetCurrentMean()
{
   return( m_fCurrMean );
}

//=== GetCurrentStdDev ================================================================================================
//
//    Get sample standard deviation for the current distribution being compiled.  It is computed over the N most recent
//    valid samples, where N is returned by GetCurrentNumMostRecent().  If N < 2, all valid samples are included.
//
//    ARGS:       NONE.
//    RETURNS:    Standard dev of samples collected in the current distribution.  If no samples yet, zero is returned.
//
float CCxRPDistro::GetCurrentStdDev()
{
   return( m_fCurrStdDev );
}

//=== Get/SetCurrentNumMostRecent =====================================================================================
//
//    Get/set the number of most recent valid samples to include when calculating stats and histogram for the current
//    distribution.  If this number is < 2, then ALL valid samples are included in the calculations.
//
int CCxRPDistro::GetCurrentNumMostRecent()
{
   return( m_nCurrMostRecent );
}

VOID CCxRPDistro::SetCurrentNumMostRecent( int n )
{
   m_nCurrMostRecent = (n<2) ? 0 : n;
   Recalc(m_currSamples, m_nCurrMostRecent, m_nCurrValidSamples, m_fCurrMean, m_fCurrStdDev);
}

//=== GetCurrentHistogram =============================================================================================
//
//    Generate a histogram representation of the current distribution being compiled, including only the N most recent
//    samples that fall within the "valid response range", where N is the number returned by GetCurrentNumMostRecent().
//    If N < 2, then ALL valid samples are included.  The valid response range is divided into the specified number of
//    bins. If there are currently no "valid" samples, all histogram bins will be empty.
//
//    ARGS:       pBins -- [out] the histogram bins.  Integer array must have at least nBins elements!
//                nBins -- [in] number of bins in histogram.  Must lie in [5..50].
//
//    RETURNS:    FALSE if specified #bins is illegal.
//
BOOL CCxRPDistro::GetCurrentHistogram( int* pBins, int nBins )
{
   int i;

   // check for valid # of bins
   if( nBins < 5 || nBins > 50 )
      return( FALSE );

   // zero all bins initially
   for( i=0; i<nBins; i++ ) pBins[i] = 0;

   // now examine all, or the N most recent, valid samples and populate histogram bins
   int ns = GetTotalCurrentSamples();
   int nCount = 0;
   float fBinSize = (m_fRespMax - m_fRespMin) / float(nBins);
   for( i=ns-1; i>=0; i-- ) if( m_currSamples[i] >= m_fRespMin && m_currSamples[i] <= m_fRespMax )
   {
      int iBin = (int) ::floor( (m_currSamples[i] - m_fRespMin) / fBinSize );
      if( iBin >= nBins ) iBin = nBins-1;
      ++(pBins[iBin]);

      // stop after including the N most recent valid samples, IF N >= 2
      ++nCount;
      if( m_nCurrMostRecent >= 2 && nCount >= m_nCurrMostRecent )
         break;
   }

   return( TRUE );
}

//=== GetTotalPreviousSamples, GetNumValidPrevoiusSamples =============================================================
//
//    Get the total number of response samples in the previous distribution compiled, and the number of those which
//    fall within the valid response range.
//
//    ARGS:       NONE.
//    RETURNS:    Total #samples, or # of "valid" samples.
//
int CCxRPDistro::GetTotalPreviousSamples()
{
   return( static_cast<int>(m_prevSamples.GetSize()) );
}

int CCxRPDistro::GetNumValidPreviousSamples()
{
   return( m_nPrevValidSamples );
}

//=== GetPreviousSample ================================================================================================
//
//    Retrieve a particular sample from the last distribution collected.
//
//    ARGS:       i  -- index of requested sample.
//    RETURNS:    The sample value, or zero if index is invalid.
//
float CCxRPDistro::GetPreviousSample( int i )
{
   int n = GetTotalPreviousSamples();
   return( (i<0 || i>=n) ? 0.0f : m_prevSamples[i] );
}

//=== GetPreviousMean ==================================================================================================
//
//    Get sample mean for the last distribution collected.  This is the mean computed over the N most recent valid
//    samples, where N is returned by GetPreviousNumMostRecent().  If N < 2, all valid samples are included.
//
//    ARGS:       NONE.
//    RETURNS:    Mean value of samples collected in the previous distribution.  If no samples, zero is returned.
//
float CCxRPDistro::GetPreviousMean()
{
   return( m_fPrevMean );
}

//=== GetPreviousStdDev ================================================================================================
//
//    Get sample standard deviation for the last distribution collected.  It is computed over the N most recent valid
//    samples, where N is returned by GetPreviousNumMostRecent().  If N < 2, all valid samples are included.
//
//    ARGS:       NONE.
//    RETURNS:    Standard dev of samples collected in the previous distribution.  If no samples, zero is returned.
//
float CCxRPDistro::GetPreviousStdDev()
{
   return( m_fPrevStdDev );
}

//=== Get/SetPreviousNumMostRecent ====================================================================================
//
//    Get/set the number of most recent valid samples to include when calculating stats and histogram for the previous
//    distribution.  If this number is < 2, then ALL valid samples are included in the calculations.
//
int CCxRPDistro::GetPreviousNumMostRecent()
{
   return( m_nPrevMostRecent );
}

VOID CCxRPDistro::SetPreviousNumMostRecent( int n )
{
   m_nPrevMostRecent = (n<2) ? 0 : n;
   Recalc(m_prevSamples, m_nPrevMostRecent, m_nPrevValidSamples, m_fPrevMean, m_fPrevStdDev);
}

//=== GetPreviousHistogram ============================================================================================
//
//    Generate a histogram representation of the previous distribution compiled, including only the N most recent
//    samples that fall within the "valid response range", where N is returned by GetPreviousNumMostRecent().  If N<2,
//    then ALL valid samples are included.  The valid response range is divided into the specified number of bins. If
//    there are currently no "valid" samples, all histogram bins will be empty.
//
//    ARGS:       pBins -- [out] the histogram bins.  Integer array must have at least nBins elements!
//                nBins -- [in] number of bins in histogram.  Must lie in [5..50].
//
//    RETURNS:    FALSE if specified #bins is illegal.
//
BOOL CCxRPDistro::GetPreviousHistogram( int* pBins, int nBins )
{
   int i;

   // check for valid # of bins
   if( nBins < 5 || nBins > 50 )
      return( FALSE );

   // zero all bins initially
   for( i=0; i<nBins; i++ ) pBins[i] = 0;

   // now examine all, or the N most recent, valid samples and populate histogram bins
   int ns = GetTotalPreviousSamples();
   int nCount = 0;
   float fBinSize = (m_fRespMax - m_fRespMin) / float(nBins);
   for( i=ns-1; i>=0; i-- ) if( m_prevSamples[i] >= m_fRespMin && m_prevSamples[i] <= m_fRespMax )
   {
      int iBin = (int) ::floor( (m_prevSamples[i] - m_fRespMin) / fBinSize );
      if( iBin >= nBins ) iBin = nBins-1;
      ++(pBins[iBin]);

      // stop after including the N most recent valid samples, IF N >= 2
      ++nCount;
      if( m_nPrevMostRecent >= 2 && nCount >= m_nPrevMostRecent )
         break;
   }

   return( TRUE );
}

//=== GetResponseRange ================================================================================================
//
//    Get the valid response range.  Samples outside this range will not be compiled in the distribution, although they
//    are still tallied in reward stats (when a reward window is defined).
//
//    ARGS:       fMin  -- [out] minimum valid response value.
//                fMax  -- [out] maximum valid response value.
//    RETURNS:    NONE.
//
VOID CCxRPDistro::GetResponseRange( float& fMin, float& fMax )
{
   fMin = m_fRespMin;
   fMax = m_fRespMax;
}

//=== SetResponseRange ================================================================================================
//
//    Set the valid response range.  If the specified minimum is less than the maximum, they are swapped.  If they are
//    equal, then the maximum is automatically set to (min+1).
//
//    Whenever the valid response range is changed, the current and previous distribution stats are recalculated --
//    since the set of "valid" samples may have changed.  The reward window is also adjusted to ensure that it is
//    entirely contained in the valid response range.
//
//    ARGS:       fMin  -- [out] minimum valid response value.
//                fMax  -- [out] maximum valid response value.
//    RETURNS:    NONE.
//
VOID CCxRPDistro::SetResponseRange( float fMin, float fMax )
{
   if( fMin == fMax )
   {
      m_fRespMin = fMin;
      m_fRespMax = m_fRespMin + 1.0f;
   }
   else if( fMin > fMax )
   {
      m_fRespMin = fMax;
      m_fRespMax = fMin;
   }
   else
   {
      m_fRespMin = fMin;
      m_fRespMax = fMax;
   }

   Recalc(m_currSamples, m_nCurrMostRecent, m_nCurrValidSamples, m_fCurrMean, m_fCurrStdDev);
   Recalc(m_prevSamples, m_nPrevMostRecent, m_nPrevValidSamples, m_fPrevMean, m_fPrevStdDev);
   RestrictRewardWinToValidRange();
}

//=== Get/SetRewardWin*** =============================================================================================
//
//    Getter/setter methods for the different properties defining the CCxRPDistro object's reward window: enable flag,
//    window min/max bounds in response sample units, dynamic shift value (0 if window is not dynamic), and the number
//    of valid response samples to be collected before each dynamic window update.  This last parameter must be 2 or
//    more; otherwise, it is set to 0, which will disable the dynamic update feature.
//
//    Whenever a reward window bound is changed, the new bounds will be auto-corrected to ensure that min<max and that
//    the new reward window lies within the current valid response range -- so be sure to check both bounds after
//    calling either SetRewardWinMinimum() or SetRewardWinMaximum().
//
BOOL CCxRPDistro::IsRewardWinEnabled()
{
   return( m_bRewEnable );
}

VOID CCxRPDistro::SetRewardWinEnabled( BOOL bEna )
{
   m_bRewEnable = bEna;
}

float CCxRPDistro::GetRewardWinMinimum()
{
   return( m_fRewMin );
}

VOID CCxRPDistro::SetRewardWinMinimum( float fMin )
{
   m_fRewMin = fMin;
   if( m_fRewMin >= m_fRewMax ) m_fRewMin = m_fRewMax - 1.0f;
   RestrictRewardWinToValidRange();
}

float CCxRPDistro::GetRewardWinMaximum()
{
   return( m_fRewMax );
}

VOID CCxRPDistro::SetRewardWinMaximum( float fMax )
{
   m_fRewMax = fMax;
   if( m_fRewMax <= m_fRewMin ) m_fRewMax = m_fRewMin + 1.0f;
   RestrictRewardWinToValidRange();
}

float CCxRPDistro::GetRewardWinShift()
{
   return( m_fRewShift );
}

VOID CCxRPDistro::SetRewardWinShift( float fShift )
{
   m_fRewShift = fShift;
}

int CCxRPDistro::GetRewardWinUpdateIntv()
{
   return( m_nUpdateIntv );
}

VOID CCxRPDistro::SetRewardWinUpdateIntv( int nSamples )
{
   m_nUpdateIntv = (nSamples < 2) ? 0 : nSamples;
}

//=== GetNumTries =====================================================================================================
//
//    Get the number of "tries", ie, the number of samples collected for the current distribution since a reward window
//    was last defined/redefined on this CCxRPDistro object.  If no reward window is defined, this stat is irrelevant
//    and the method returns 0.
//
//    ARGS:       NONE.
//    RETURNS:    As described.
//
int CCxRPDistro::GetNumTries()
{
   return( m_nTries );
}

//=== GetNumPassed ====================================================================================================
//
//    Get the number of times this CCxRPDistro's reward contingency was "satisfied", ie, the number of samples
//    collected for the current distribution that fall within a defined "reward window".  If no reward window is
//    defined, this stat is irrelevant and the method returns 0.
//
//    ARGS:       NONE.
//    RETURNS:    As described.
//
int CCxRPDistro::GetNumPassed()
{
   return( m_nPassed );
}

//=== GetTextSummary ==================================================================================================
//
//    Provide a "snapshot" summary of this CCxRPDistro object's current state.  The textual summary is essentially a
//    complete dump of the object's contents, formatted with linefeeds so it can be directly written to a text file.
//    It includes sample histograms for the current and previous distributions (if they contain any samples!)
//
//    ARGS:       strOut   -- [out] CString that will hold the text summary.  Any character content already in the
//                         buffer will be replaced.
//                nBins    -- [in] #bins in the sample histograms included in text summary.  Restricted to [5..50].
//    RETURNS:    NONE.
//
VOID CCxRPDistro::GetTextSummary( CString& strOut, int nBins )
{
   int i, nB;
   int bins[50];
   CString strLine;
   CString strTemp;

   // restrict #bins allowed for sample histograms included in summary
   nB = (nBins < 5) ? 5 : ((nBins > 50) ? 50 : nBins);

   // start with an empty buffer
   strOut.Empty();

   // behavioral response type
   strLine.Format("Measured response type: %s\n", GetResponseTypeDesc(m_iRespType));
   strOut += strLine;
   
   // valid response range
   strLine.Format( "Valid response range: [%.3f to %.3f]\n", m_fRespMin, m_fRespMax );
   strOut += strLine;

   // reward windows
   strLine = _T("Reward Window: ");
   if( !m_bRewEnable )
      strLine += _T("NONE\n");
   else
   {
      strTemp.Format("[%.3f to %.3f], shift=%.3f, updN=%d", m_fRewMin, m_fRewMax, m_fRewShift, m_nUpdateIntv);
      strLine += strTemp;
      strLine += _T("\n");
   }
   strOut += strLine;

   // reward stats
   strLine.Format( "#passed = %d, #failed= %d\n", GetNumPassed(), GetNumFailed() );
   strOut += strLine;

   // current distribution stats and all samples, plus sample histogram
   CString strMostRecentCaveat;
   if( m_nCurrMostRecent < 2 )
      strMostRecentCaveat = _T("(over ALL valid samples)");
   else
      strMostRecentCaveat.Format( "(over %d most recent valid samples)", m_nCurrMostRecent );

   strLine.Format( "Current: N = %d total, %d valid; mean = %.3f, stdev = %.3f %s\n", GetTotalCurrentSamples(),
      GetNumValidCurrentSamples(), GetCurrentMean(), GetCurrentStdDev(), strMostRecentCaveat );
   strOut += strLine;

   if( GetTotalCurrentSamples() > 0 )
   {
      strLine = _T(" All samples: ");
      for( i=0; i<GetTotalCurrentSamples(); i++ )
      {
         strTemp.Format( "%.3f ", GetCurrentSample(i) );
         if( strTemp.GetLength() + strLine.GetLength() > 80 )
         {
            strOut += strLine + _T("\n");
            strLine = _T("   ");
         }
         strLine += strTemp;
      }
      if( strLine.GetLength() > 3 )
         strOut += strLine + _T("\n");

      GetCurrentHistogram( &(bins[0]), nB );
      strLine.Format( "Sample histogram %s: \n", strMostRecentCaveat );
      strOut += strLine;
      float fBinStart = m_fRespMin;
      float fBinSize = (m_fRespMax-m_fRespMin) / float(nB);
      strLine = _T("   ");
      for( i=0; i<nB; i++ )
      {
         strTemp.Format( "%d [%.3f %.3f]; ", bins[i], fBinStart, fBinStart+fBinSize );
         if( strTemp.GetLength() + strLine.GetLength() > 80 )
         {
            strOut += strLine + _T("\n");
            strLine = _T("   ");
         }
         strLine += strTemp;
         fBinStart += fBinSize;
      }
      if( strLine.GetLength() > 3 )
         strOut += strLine + _T("\n");
   }

   // previous distribution stats and all samples, plus sample histogram
   if( m_nPrevMostRecent < 2 )
      strMostRecentCaveat = _T("(over ALL valid samples)");
   else
      strMostRecentCaveat.Format( "(over %d most recent valid samples)", m_nPrevMostRecent );

   strLine.Format( "Previous: N = %d total, %d valid; mean = %.3f, stdev = %.3f %s\n", GetTotalPreviousSamples(),
      GetNumValidPreviousSamples(), GetPreviousMean(), GetPreviousStdDev(), strMostRecentCaveat );
   strOut += strLine;

   if( GetTotalPreviousSamples() > 0 )
   {
      strLine = _T(" All samples: ");
      for( i=0; i<GetTotalPreviousSamples(); i++ )
      {
         strTemp.Format( "%.3f ", GetPreviousSample(i) );
         if( strTemp.GetLength() + strLine.GetLength() > 80 )
         {
            strOut += strLine + _T("\n");
            strLine = _T("   ");
         }
         strLine += strTemp;
      }
      if( strLine.GetLength() > 3 )
         strOut += strLine + _T("\n");

      GetPreviousHistogram( &(bins[0]), nB );
      strLine.Format( "Sample histogram %s: \n", strMostRecentCaveat );
      strOut += strLine;
      float fBinStart = m_fRespMin;
      float fBinSize = (m_fRespMax-m_fRespMin) / float(nB);
      strLine = _T("   ");
      for( i=0; i<nB; i++ )
      {
         strTemp.Format( "%d [%.3f %.3f]; ", bins[i], fBinStart, fBinStart+fBinSize );
         if( strTemp.GetLength() + strLine.GetLength() > 80 )
         {
            strOut += strLine + _T("\n");
            strLine = _T("   ");
         }
         strLine += strTemp;
         fBinStart += fBinSize;
      }
      if( strLine.GetLength() > 3 )
         strOut += strLine + _T("\n");
   }

}

//=== Recalc ==========================================================================================================
//
//    Helper method determines the total number of valid response samples in the given response sample distribution,
//    and calculates the mean and standard deviation over all valid samples, or over the N most recent valid samples
//    (if N >= 2).
//
//    ARGS:       samples -- [in] the collection of response samples.
//                nRecent -- [in] the number of most recent valid samples over which to compute mean and std dev.
//                nValid -- [out] the TOTAL number of valid response samples in the sample collection.
//                fMean -- [out] the computed response mean.
//                fStdDev -- [out] the computed response standard deviation.
//    RETURNS:    NONE.
//
VOID CCxRPDistro::Recalc(const CSampleArray& samples, int nRecent, int& nValid, float& fMean, float& fStdDev )
{
   int n = static_cast<int>(samples.GetSize());
   fMean = 0.0f;
   fStdDev = 0.0f;

   // calculate mean, including all valid response samples or only the N most recent
   nValid = 0;
   int iCount = 0;
   float fSum = 0.0f;
   for( int i=n-1; i>=0; i-- ) if( samples[i] >= m_fRespMin && samples[i] <= m_fRespMax )
   {
      ++nValid;
      if( nRecent < 2 )
         fSum += samples[i];
      else if( iCount < nRecent )
      {
         fSum += samples[i];
         ++iCount;
      }
   }
   if( nValid == 0 )
      return;
   fMean = fSum / float((nRecent<2) ? nValid : iCount);

   // calculate std dev, likewise including all valid response samples or only the N most recent
   int iLast = (nRecent<2) ? 0 : n-iCount;
   fSum = 0.0f;
   for( int i=n-1; i>iLast; i-- ) if( samples[i] >= m_fRespMin && samples[i] <= m_fRespMax )
   {
      float f = samples[i] - fMean;
      fSum += f*f;
   }
   fStdDev = (float) ::sqrt( double(fSum/float((nRecent<2) ? nValid : iCount)) );
}

//=== RestrictRewardWinToValidRange ===================================================================================
//
//    Helper method restricts the min/max bounds of the reward window so that it always lies in the valid response
//    range.  Call this method whenever the valid response range or the reward window definition changes!
//
VOID CCxRPDistro::RestrictRewardWinToValidRange()
{
   if( m_fRewMin < m_fRespMin ) m_fRewMin = m_fRespMin;
   if( m_fRewMax > m_fRespMax ) m_fRewMax = m_fRespMax;
   if( m_fRewMin >= m_fRewMax )
   {
      m_fRewMin = m_fRespMin;
      m_fRewMax = m_fRespMax;
   }
}


