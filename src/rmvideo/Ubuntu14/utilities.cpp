//=====================================================================================================================
//
// util.cpp : Implementation of several general utility classes.
//
//=====================================================================================================================

#include "unistd.h"
#include "time.h"
#include "utilities.h"


//=====================================================================================================================
// Implementation of class CElapsedTime
//
// This is a quick-n-dirty utility to track an elapsed time in nanoseconds.  It should NEVER be used for precise
// timing, OK?!  To use it, merely construct an instance when you wish to start tracking an elapsed time, call get() to
// obtain the current elapsed time in secs, and call reset() to reset the current elapsed time to zero. GetAndReset()
// does both tasks in one call.
//
// The implementation relies on the POSIX 1003.1-2001 standard function clock_gettime() using the high-resolution 
// per-process clock CLOCK_PROCESS_CPUTIME_ID. If the clock is not available, CElapsedTime::get() and getAndReset() 
// always return 0. To verify that this high-resolution clock is available, call the static method isSupported().
//
// Internally, the seconds portion of the clock time returned by clock_gettime(), representing seconds since the
// "Epoch" (00.00.00 on Jan 1970), is at least a 32-bit integer, and more likely a 64-bit integer -- so we do NOT
// worry about the time wrapping back to zero!
//
// 21Jan2019-- Replaced CLOCK_PROCESS_CPUTIME_ID with CLOCK_MONOTONIC. The former will give an underestimate of 
// elapsed time because it only counts clock cycles spent doing work for the calling process; it excludes any cycles
// when process is blocked or sleeping. THIS IS AN IMPORTANT CHANGE. CElapsedTime is used when estimating the refresh
// period of the RMVideo display. Using CLOCK_PROCESS_CPUTIME_ID consistently underestimated the refresh period, so 
// the reported refresh rate was overestimated.
//
//=====================================================================================================================

void CElapsedTime::reset()
{
   struct timespec tNow;
   if( ::clock_gettime(CLOCK_MONOTONIC, &tNow) == 0 )
      m_dTimeZero = double( tNow.tv_sec ) + 1.0e-9 * double( tNow.tv_nsec );
   else
      m_dTimeZero = 0.0;
}

double CElapsedTime::get()
{
   struct timespec tNow;
   if( ::clock_gettime(CLOCK_MONOTONIC, &tNow) == 0 )
   {
      double dTimeNow = double( tNow.tv_sec ) + 1.0e-9 * double( tNow.tv_nsec );
      return( dTimeNow - m_dTimeZero );
   }
   else
      return( 0.0 );
}

double CElapsedTime::getAndReset()
{
   struct timespec tNow;
   if( ::clock_gettime(CLOCK_MONOTONIC, &tNow) == 0 )
   {
      double dTimeNow = double( tNow.tv_sec ) + 1.0e-9 * double( tNow.tv_nsec );
      double dElapsed = dTimeNow - m_dTimeZero;
      m_dTimeZero = dTimeNow;
      return( dElapsed );
   }
   else
      return( 0.0 );
}

bool CElapsedTime::isSupported()
{
   if( ::sysconf(_SC_TIMERS) > 0 )
   {
      struct timespec tSpec;
      if( ::clock_getres(CLOCK_MONOTONIC, &tSpec) == 0 )
         return( true );
   }
   return( false );
}



//=====================================================================================================================
// Implementation of class CUniformRNG
//
// This class encapsulates a pseudo-random number generator that returns a sequence of uniformly distributed floating-
// point values in (0.0 .. 1.0), endpoints excluded.  It encapsulates the "ran1" algorithm presented on p.282 in:
// Press, WH; et al.  "Numerical recipes in C: the art of acientific computing".  New York:  Cambridge University
// Press, Copyright 1988-1992.
//
// The algorithm uses a 32-entry table to shuffle the output of a "Minimal Standard" linear congruential generator, of
// the form I(n+1) = A*I(n) % M (with A and M carefully chosen).  Schrage's method is used to compute I(n+1) without
// an integer overflow.  The 32-bit integers output by the algorithm fall in the range [1..M-1]; dividing by M=2^31
// gives a double-valued output in (0..1).  For algorithmic details, consult the book.  There are few comments here.
//
// Portability note:  We assume here that int is 32-bit!!
//
// IAW the licensing policy of "Numerical Recipes in C", this class is not distributable in source code form without
// obtaining the appropriate license; however, it may appear in an executable file that is distributed.
//=====================================================================================================================


int CUniformRNG::LC_M                  = 2147483647;
int CUniformRNG::LC_A                  = 16807;
int CUniformRNG::LC_Q                  = 127773;
int CUniformRNG::LC_R                  = 2836;
int CUniformRNG::NDIV                  = (1 + (CUniformRNG::LC_M - 1)/CUniformRNG::TABLESZ);
double CUniformRNG::DSCALE             = (1.0/CUniformRNG::LC_M);

//=== setSeed =========================================================================================================
//
//    Initialize the random generator with the specified seed value.  The absolute value is used; if it is zero, the
//    the value 1 will be used instead.
//
//    ARGS:       seed  -- [in] seed value
//
//    RETURNS:    NONE.
//
void CUniformRNG::setSeed( int seed )
{
   m_curr = (seed == 0) ? 1 : ((seed < 0) ? -seed : seed);              // start at strictly positive seed value

   int k;                                                               // after discarding first 8 integers generated
   for( int j = TABLESZ+7; j >= 0; j-- )                                // by the algorithm, fill shuffle table with
   {                                                                    // next TABLESZ integers generated
      k = m_curr/LC_Q;
      m_curr = LC_A * (m_curr - k*LC_Q) - k*LC_R;
      if( m_curr < 0 ) m_curr += LC_M;
      if( j < TABLESZ ) m_shuffle[j] = m_curr;
   }

   m_lastOut = m_shuffle[0];
}

//=== generate ========================================================================================================
//
//    Generate next random number in sequence, uniformly distributed in (0.0 .. 1.0).  Note that the endpoint values
//    are excluded.  The algorithm is such that we could see some skewing of the distribution at the largest float
//    value less than 1.0.
//
//    ARGS:       NONE.
//
//    RETURNS:    next value in a uniformly distributed random # sequence.
//
double CUniformRNG::generate()
{
   int k = m_curr/LC_Q;                                  // compute I(n+1) = A*I(n) % M using Schrage's method to
   m_curr = LC_A * (m_curr - k*LC_Q) - k*LC_R;           // avoid integer overflows
   if( m_curr < 0 ) m_curr += LC_M;

   int index = m_lastOut / NDIV;                         // use last number retrieved from shuffle table to calc index
   m_lastOut = m_shuffle[index];                         // of next number to retrieve. Replace that entry in shuffle
   m_shuffle[index] = m_curr;                            // table with the curr output of LC generator, calc'd above.

   return( DSCALE * m_lastOut );                         // convert int in [1..M-1] to floating-pt output in (0..1)
}



