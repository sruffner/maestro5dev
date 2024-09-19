//=====================================================================================================================
//
// cxrpdistro.h : Declaration of class CCxRPDistro, which encapsulates information for a CCxTrial object that includes
//                a distribution-based reward/penalty contingency.
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================

#if !defined(CXRPDISTRO_H__INCLUDED_)
#define CXRPDISTRO_H__INCLUDED_

#include "afxtempl.h"                        // for CTypedPtrList template

//=====================================================================================================================
// Declaration of class CCxRPDistro
//=====================================================================================================================
//
typedef CArray<float, float> CSampleArray;      // array of samples of a measured behavioral parameter

class CCxRPDistro
{
//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   int m_iRespType;                             // response measure type -- see TH_RPD_*** constants in cxobj_ifc.h
   
   CSampleArray m_currSamples;                  // distribution currently being collected
   float m_fCurrMean;                           // stats for current distribution (recomputed when sample added)
   float m_fCurrStdDev;
   int m_nCurrValidSamples;

   CSampleArray m_prevSamples;                  // previous distribution collected (if any)
   float m_fPrevMean;                           // stats for previous distribution
   float m_fPrevStdDev;
   int m_nPrevValidSamples;

   int m_nCurrMostRecent;                       // for each distribution, stats/histogram are reported over the N most
   int m_nPrevMostRecent;                       // recent valid samples.  If N < 2, all valid samples are included.

   BOOL m_bRewEnable;                           // TRUE if reward window is enabled
   float m_fRewMin;                             // minimum bound of reward window (in response sample units)
   float m_fRewMax;                             // maximum bound of reward window (in response sample units)
   float m_fRewShift;                           // window shift for dynamic window updates (0=not dynamic)
   int m_nUpdateIntv;                           // dynamic window update interval (# valid response samples)
   int m_nSampleCount;                          // valid response sample counter for dynamic window updates

   float m_fRespMin;                            // bounds of "valid" response range
   float m_fRespMax;

   int m_nTries;                                // #samples collected since reward window(s) defined
   int m_nPassed;                               // #samples falling within reward window(s)
   int m_iLastResult;                           // 0 = fail, 1 = pass, -1 = no reward window defined

//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
private:
   CCxRPDistro( const CCxRPDistro& src );                // no copy constructor or assignment operator defined
   CCxRPDistro& operator=( const CCxRPDistro& src );

public:
   CCxRPDistro();                                        // constructor
   ~CCxRPDistro();                                       // destructor


//=====================================================================================================================
// OPERATIONS
//=====================================================================================================================
public:
   VOID Reset();                                         // reset state: no distr data, no reward windows, zero stats
   VOID StartNewDistribution();                          // copy current => prev distribution, reset curr distribution
   VOID AddSample( float fVal );                         // add sample to current distribution; update stats, rew win

public:
   int GetResponseType();                                // get/set the type of behavioral response measured
   VOID SetResponseType(int type);

   static LPCTSTR GetResponseTypeDesc(int type);         // short GUI name for the specified behavioral response type

   int GetTotalCurrentSamples();                         // #samples in current distribution being collected
   int GetNumValidCurrentSamples();                      // #samples in current distrib within valid response range
   float GetCurrentSample( int i );                      // get a particular sample from the current distribution
   float GetCurrentMean();                               // get mean of current distribution
   float GetCurrentStdDev();                             // get standard deviation for current distribution
   int GetCurrentNumMostRecent();                        // get/set the number of most recent valid samples to include
   VOID SetCurrentNumMostRecent( int n );                // when calc'g stats/histogram (if 0, all valid samples used)
   BOOL GetCurrentHistogram( int* pBins, int nBins );    // prepare a histogram of current distribution

   int GetTotalPreviousSamples();                        // #samples in previous distribution compiled
   int GetNumValidPreviousSamples();                     // #samples in previous distrib within valid response range
   float GetPreviousSample( int i );                     // get a particular sample from the previous distribution
   float GetPreviousMean();                              // get mean of previous distribution
   float GetPreviousStdDev();                            // get standard deviation for previous distribution
   int GetPreviousNumMostRecent();                       // get/set the number of most recent valid samples to include
   VOID SetPreviousNumMostRecent( int n );               // when calc'g stats/histogram (if 0, all valid samples used)
   BOOL GetPreviousHistogram( int* pBins, int nBins );   // prepare a histogram of previous distribution

   VOID GetResponseRange( float& fMin, float& fMax );    // get/set range of response values that count toward distrib
   VOID SetResponseRange( float fMin, float fMax );

   BOOL IsRewardWinEnabled();                            // get/set properties of the reward window
   VOID SetRewardWinEnabled( BOOL bEna );
   float GetRewardWinMinimum();
   VOID SetRewardWinMinimum( float fMin );
   float GetRewardWinMaximum();
   VOID SetRewardWinMaximum( float fMax );
   float GetRewardWinShift();
   VOID SetRewardWinShift( float fShift );
   int GetRewardWinUpdateIntv();
   VOID SetRewardWinUpdateIntv( int nSamples );

   int GetNumTries();                                    // get #samples collected since reward window(s) defined
   int GetNumPassed();                                   // get #samples falling w/in reward window(s)
   int GetNumFailed()                                    // get #samples falling outside reward window(s)
   {
      return( GetNumTries() - GetNumPassed() );
   }
   int GetLastResult()                                   // did last response sample "pass" (1) or "fail" (0), or was
   {                                                     // it not assessed b/c no reward window is defined (-1)?
      return( m_iLastResult );
   }

   VOID GetTextSummary( CString& strOut, int nBins );    // get summary of current state, for printing to file

private:
   VOID Recalc(const CSampleArray& samples,              // recalc mean & std dev of sample set, excluding samples that
      int nRecent, int& nValid, float& fMean,            // fall outside valid response range
      float& fStdDev );
   VOID RestrictRewardWinToValidRange();                 // restrict reward window to the valid response range

};


#endif   // !defined(CXRPDISTRO_H__INCLUDED_)
