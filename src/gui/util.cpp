//===================================================================================================================== 
//
// util.cpp : Implementation of several general utility classes.
//
//===================================================================================================================== 

#include "util.h"


//===================================================================================================================== 
// Implementation of class cMath
//===================================================================================================================== 

const double cMath::PI              = 3.14159265359;
const double cMath::TWOPI           = 2.0 * cMath::PI;
const double cMath::DEGTORAD        = cMath::PI/180.0;


//===================================================================================================================== 
// Implementation of class CElapsedTime
//
// This is a quick-n-dirty utility to track an elapsed time in microseconds.  It should NEVER be used for precise 
// timing, OK?!  To use it, merely construct an instance when you wish to start tracking an elapsed time, call Get() to 
// obtain the current elapsed time in us, and call Reset() to reset the current elapsed time to zero.  GetAndReset() 
// does both tasks in one call.
//
// The implementation uses the Windows high-resolution counter via QueryPerformanceCounter().
//
// 04nov2015 -- Revised implementation to use the Windows high-resolution counter via QueryPerformanceCounter(), if
// available. RTX documentation recommends using this instead of RtGetClockTime() to get better accuracy on a 
// multi-processor system.
// 31aug2017 -- RTX-based implementation removed here because we're attempting to build Maestro for Windows 10 without
// using RTX. In addition, RTX64 docs recommend using QueryPerformanceCounter (which is supported in an RTSS process)
// rather than RtGetClockTime().
// 12sep2017 -- Removed alternate implementation based on GetTickCount(). Windows documentation indicates that the
// high-resolution performance counter will be available on any system running Windows XP or later.
//===================================================================================================================== 

double CElapsedTime::m_hpcPeriodUS = -1;  // actual value calculated on first use
VOID CElapsedTime::_GetHPCPeriod()
{
   if (m_hpcPeriodUS < 0)
   {
      LARGE_INTEGER li;
      ::QueryPerformanceFrequency(&li);
      m_hpcPeriodUS = 1000000.0 / ((double)li.QuadPart);
   }
}

CElapsedTime::CElapsedTime()
{
   _GetHPCPeriod();
   Reset();
}

VOID CElapsedTime::Reset() { ::QueryPerformanceCounter(&m_timeZeroCount); }

double CElapsedTime::Get() 
{
   double t;
   LARGE_INTEGER li;
   ::QueryPerformanceCounter(&li);
   t = (li.QuadPart - m_timeZeroCount.QuadPart) * m_hpcPeriodUS;
   return(t);
}

double CElapsedTime::GetAndReset()
{
   double t;
   LARGE_INTEGER li;
   QueryPerformanceCounter(&li);
   t = (li.QuadPart - m_timeZeroCount.QuadPart) * m_hpcPeriodUS;
   m_timeZeroCount = li;
   return(t);
}


//===================================================================================================================== 
// Implementation of class CRand16
//
// A 16-bit unsigned random integer is formed from the output of 16 independent random-bit sequences.  The sequences 
// are generated using an algorithm presented in:  Press, WH; et al.  "Numerical recipes in C:  the art of scientific 
// computing".  New York:  Cambridge University Press, 1988.  Chap 7, p. 226.  Each sequence is based on a different 
// modulo-2 primitive polynomial of degree 31, each started using a different 32-bit seed.  These polynomials were 
// obtained from http://sue.csc.uvic.ca/~cos/per/neck.  The generated bit sequences will not repeat for 2^31 - 1 
// iterations.
//
// As an added twist, since our specific application in CXDRIVER needs only small integers, we have modified the 
// algorithm to spit out unsigned numbers up to a specified maximum value.  We calcuate the # of bits required to 
// represent the maximum integer, then only generate that many bits.  See CRand16::Generate() for details.  Of course, 
// if the number generated is larger than the maximum, we have to try again -- so the algorithm is inefficient.  For 
// example, if the maximum number is 64 (requires 7 bits), then 50% of the numbers generated will be too big (65..127). 
// Still, for the small integers that we require, this is not a big problem. 
//
// TECH NOTE:  On a 600MHz P3 machine with 256MB RAM, CRand16::Generate(32768) executed in ~950ns on average, with an 
// observed range of roughly 0-4us.  This test measures the worst possible performance (when no other higher priority 
// threads are running), since an arg of 32768 requires all 16 bits -- so roughly half the #s generated are too big.
//
//===================================================================================================================== 

const DWORD CRand16::PRIMPOLY[16] =                // primitive polynomials:  masks include all nonzero coeff other 
                                   {               // than 31 & 0; bit N-1 = coeff N; coeff0 is special.
                                    0x00000004,    //    (31,  3,  0)
                                    0x00000007,    //    (31,  3,  2,  1,  0)
                                    0x00001084,    //    (31, 13,  8,  3,  0)
                                    0x00100807,    //    (31, 21, 12,  3,  2,  1,  0)
                                    0x000A0054,    //    (31, 20, 18,  7,  5,  3,  0)
                                    0x0008401C,    //    (31, 20, 15,  5,  4,  3,  0)
                                    0x0000808E,    //    (31, 16,  8,  4,  3,  2,  0)
                                    0x00064402,    //    (31, 19, 18, 15, 11,  2,  0)
                                    0x04020228,    //    (31, 27, 18, 10,  6,  4,  0)
                                    0x20081820,    //    (31, 30, 20, 13, 12,  6,  0)
                                    0x0201001A,    //    (31, 26, 17,  5,  4,  2,  0)
                                    0x04444444,    //    (31, 27, 23, 19, 15, 11,  7,  3,  0)
                                    0x01042047,    //    (31, 25, 19, 14,  7,  3,  2,  1,  0)
                                    0x0060604C,    //    (31, 23, 22, 15, 14,  7,  4,  3,  0)
                                    0x02040166,    //    (31, 26, 19,  9,  7,  6,  3,  2,  0)
                                    0x0000585C     //    (31, 15, 13, 12,  7,  5,  4,  3,  0)
                                    }; 


//=== SetSeed ========================================================================================================= 
//
//    Initialize the state of all random-bit sequences based on the specified seed.  Uses a simple linear integral 
//    congruential generator to get the initial state of each bit sequence.
//
//    ARGS:       dwSeed   -- [in] seed value
//
//    RETURNS:    NONE.
//
void CRand16::SetSeed( DWORD dwSeed )
{
   DWORD a = 2147437301; 
   DWORD c = 453816981;
   m_dwBitSeq[0] = a * dwSeed + c;
   for( int i = 1; i < 16; i++ ) m_dwBitSeq[i] = a * m_dwBitSeq[i-1] + c;
}


//=== Generate ======================================================================================================== 
//
//    Generate next random unsigned integer in [0..wMax].  To restrict a number to this range, we generate the minimum 
//    # of bits required to represent an integer in [0..wMax].  If the resulting number N > wMax, we try again until 
//    we generate a number in the allowed range.  Thus, if wMax != 65535 always, the individual bit sequences do not 
//    "progress" in lock step!  (Hopefully, this will not impact the randomness of the integers generated -- after all, 
//    the 16 bit sequences are independent.)
//
//    ARGS:       wMax  -- [in] max number desired (default value is 65535).
//
//    RETURNS:    Integer generated.
//
WORD CRand16::Generate( WORD wMax /* =0xFFFF */ )
{
   int nBits = 1;                                                    // determine #bits, N, required to represent max# 
   DWORD k = 2;
   while( k <= DWORD(wMax) ) { ++nBits; k *= 2; }

   WORD wVal;
   do                                                                // keep generating N-bit integers until we get 
   {                                                                 // a number in the range [0..max]
      wVal = 0;
      for( int i = 0; i < nBits; i++ )
      {
         if( m_dwBitSeq[i] & ((DWORD) (1<<30)) )                     // state of bit 30 is next value in each bit seq 
         {
            m_dwBitSeq[i] = ((m_dwBitSeq[i] ^ PRIMPOLY[i]) << 1) | 0x00000001;
            wVal |= (1 << i);
         }
         else
            m_dwBitSeq[i] <<= 1;
      }
   }
   while( wVal > wMax );

   return( wVal );
}



//===================================================================================================================== 
// Implementation of class CUniformRNG
//
// This class encapsulates a pseudo-random number generator that returns a sequence of uniformly distributed 32-bit
// integers in [1..M-1], where M=2^31. It implements the "ran1" algorithm on p.282 in Press, WH; et al. "Numerical 
// recipes in C: the art of acientific computing".  New York:  Cambridge University Press, Copyright 1988-1992.
//
// The algorithm uses a 32-entry table to shuffle the output of a "Minimal Standard" linear congruential generator, of 
// the form I(n+1) = A*I(n) % M (with A and M carefully chosen). Schrage's method is used to compute I(n+1) without 
// an integer overflow.  Since the 32-bit integers output by the algorithm fall in the range [1..M-1], dividing the
// output by M results in a sequence of floating-point values uniformly distributed on (0..1). For algorithmic details,
// consult the book.  There are few comments here.  
//
// Portability note:  We assume here that int is 32-bit!!
//
// IAW the licensing policy of "Numerical Recipes in C", this class is not distributable in source code form without 
// obtaining the appropriate license; however, it may appear in an executable file that is distributed.
//===================================================================================================================== 
int CUniformRNG::LC_M = 2147483647;
int CUniformRNG::LC_A = 16807;
int CUniformRNG::LC_Q = 127773;
int CUniformRNG::LC_R = 2836;
int CUniformRNG::NDIV = (1 + (CUniformRNG::LC_M - 1)/ CUniformRNG::TABLESZ);
double CUniformRNG::DSCALE = (1.0/ CUniformRNG::LC_M);

/** SetSeed =========================================================================================================== 
 Initialize the random generator with the specified seed value.
 @param seed [in] The seed value. Should be strictly positive. If negative, absolute value is used; 0 replaced by 1.
*/
void CUniformRNG::SetSeed( int seed ) 
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

/** Generate ========================================================================================================== 
 Generate next random value in sequence, uniformly distributed in (0..1).
 @return Next value produced by the RNG engine.
*/
double CUniformRNG::Generate()
{
   int k = m_curr/LC_Q;                                  // compute I(n+1) = A*I(n) % M using Schrage's method to 
   m_curr = LC_A * (m_curr - k*LC_Q) - k*LC_R;           // avoid integer overflows
   if( m_curr < 0 ) m_curr += LC_M;

   int index = m_lastOut / NDIV;                         // use last number retrieved from shuffle table to calc index 
   m_lastOut = m_shuffle[index];                         // of next number to retrieve. Replace that entry in shuffle 
   m_shuffle[index] = m_curr;                            // table with the curr output of LC generator, calc'd above.

   return(DSCALE * m_lastOut);                           // convert int in [1..M-1] to floating-pt output in (0..1)
}

//===================================================================================================================== 
// Implementation of class CGaussRNG
//
// This class encapsulates a pseudo-random number generator that returns a sequence of normally distributed floating-
// point values with zero mean and unit variance.  It encapsulates the "gasdev" algorithm presented on p.289 in: 
// Press, WH; et al.  "Numerical recipes in C: the art of acientific computing".  New York:  Cambridge University 
// Press, Copyright 1988-1992.
//
// The algorithm uses the polar form of the Box-Muller transformation to transform a sequence of uniform deviates to 
// a sequence of Gaussian deviates.  We use CUniformRNG as the source of the uniform sequence.  For algorithmic 
// details, consult the book. 
//
// IAW the licensing policy of "Numerical Recipes in C", this class is not distributable in source code form without 
// obtaining the appropriate license; however, it may appear in an executable file that is distributed.
//===================================================================================================================== 

//=== SetSeed ========================================================================================================= 
//
//    Initialize the random generator with the specified seed value.  The absolute value is used; if it is zero, the 
//    the value 1 will be used instead.
//
//    ARGS:       seed  -- [in] seed value
//
//    RETURNS:    NONE.
//
void CGaussRNG::SetSeed(int seed)
{
   m_uniformRNG.SetSeed(seed);
   m_bGotNext = FALSE;
}

//=== Generate ======================================================================================================== 
//
//    Generate next random number in sequence, normally distributed with zero mean and unit variance.
//
//    ARGS:       NONE.
//
//    RETURNS:    next value in the Gaussian distributed random # sequence.
//
double CGaussRNG::Generate()
{
   double dVal = 0;
   if (!m_bGotNext)
   {
      double v1, v2, rsq, fac;
      do                                                          // get two uniform deviates v1,v2 such that (v1,v2) 
      {                                                           // lies strictly inside the unit circle, but not at 
         v1 = 2.0 * m_uniformRNG.Generate() - 1.0;                // the origin
         v2 = 2.0 * m_uniformRNG.Generate() - 1.0;
         rsq = v1*v1 + v2*v2;
      } while (rsq >= 1.0 || rsq == 0.0);

      fac = sqrt(-2.0 * log(rsq) / rsq);                        // use Box-Muller transformation to transform the 
      m_dNext = v1*fac;                                           // the uniform deviates to two Gaussian deviates, one 
      m_bGotNext = TRUE;                                          // of which is saved for next call to this function!
      dVal = v2*fac;
   }
   else
   {
      dVal = m_dNext;
      m_bGotNext = FALSE;
   }

   return(dVal);
}

