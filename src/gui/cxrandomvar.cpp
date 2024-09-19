//=====================================================================================================================
//
// cxrandvar.cpp : Implementation of class CCxRandomVar, representing a trial random variable.
//
// AUTHOR:  saruffner
//
// DESCRIPTION: 
// As of Maestro v3.3, a Maestro trial may define up to 10 "random variables". Each time a trial is presented, a new
// value, or "random variate", is drawn from the statistical distribution represented by the random variable. These 
// values, in turn, may be assigned to various parameters (segment duration, target velocity, and so on) in the trial. 
// In this way, each presentation of the same trial is, in fact, different in some stochastic manner that's under the 
// user's control.
//
// CCxRandomVar implements a trial random variable. It supports four distinct distributions: uniform, Gaussian 
// (normal), exponential, and gamma. At the heart of the implementation is a pseudo-random number generator that
// produces a sequence of 32-bit integers uniformly distributed over [1..(2^31)-1] -- see CRNG32Engine. A uniform
// distribution over (0..1) is obtained directly from this engine by dividing the integer output by 2^31. The 4
// general distributions can all be ultimately derived from U(0,1). For example, U(A,B) = A + (B-A)*U(0,1), and
// E(L) = -ln(U(0,1)) / L. Generation of a Gaussian or gamma RV is considerably more complex. We rely on algorithms
// from Numerical Recipes in C and other sources.
//
// Note that the defining parameters of the distribution, as well as the seed for RNG engine, are specified at
// construction time. Thus, CCxRandomVar is not suited for editing the RV's definition. In fact, it is intended only
// for use while sequencing a set of trials.
//
// CREDITS:
// (1) The internal function _genStandardGauss() uses the "gasdev" algorithm to generate a sequence of random
// variates drawn from a standard normal distribution, with a mean of 0 and standard deviation of 1. This algorithm is
// presented on p.289 in: Press, WH; et al.  "Numerical recipes in C: the art of acientific computing". New York: 
// Cambridge University Press, Copyright 1988-1992. The algorithm uses the polar form of the Box-Muller transformation 
// to transform a sequence of uniform deviates to a sequence of Gaussian deviates. For algorithmic details, consult the 
// book. IAW the licensing policy of "Numerical Recipes in C", this class is not distributable in source code form 
// without obtaining the appropriate license; however, it may appear in an executable file that is distributed.
// (2) The internal function _genGammaUnitScale() employs an algorithm developed by: G. Marsaglia and W. Tsang. 
// A simple method for generating gamma variables. In ACM Transactions on Mathematical Software 26(3):363-372, 2000. 
// The acceptance-rejection algorithm relies on drawing variates from normal(0,1) and uniform(0,1). It is concisely 
// described here: www.hongliangjie.com/2012/12/19/how-to-generate-gamma-random-variables.
// 
// HISTORY:
// 29aug2016: Began development.
// 08sep2016: Initial development complete. Need to test...
//
//=====================================================================================================================

#include "stdafx.h"                          // standard MFC stuff

#include "cxrandomvar.h"


/** Construct a uniform(0,1) random variable. */
CCxRandomVar::CCxRandomVar()
{
   m_type = UNIFORM;
   m_dParams[0] = 0.0;
   m_dParams[1] = 1.0;

   m_bStdNormValReady = FALSE;
}

/** 
 Construct a random variable following the distribution specified. The four supported distributions have different
 defining parameters, as follows:

 RVType.UNIFORM: Uniform(p1, p2). Lower bound p1 < upper bound p2. Otherwise, no restrictions. Parameter p3 unused.

 RVType.GAUSSIAN: Normal(p1, p2), where p1 is the mean (no restriction) and p2 > 0 is the standard deviation. Parameter
 p3 is the "max spread". Any value generated that is beyond +/-p3 of the mean is rejected. We require that p3 >= 3*p2.
 Note that 99.7% of values drawn from a normal distribution are within 3 standard deviations of the mean.
 
 RVType.EXPON: Exponential(p1), where p1 > 0 is the rate ("lambda"). The distribution domain here is [0..+inf]. 
 Parameter p2 is the max spread; any value greater than p2 is rejected. We require that p2 >= 3/p1. Note that the
 mean of the distribution is 1/lambda, and roughly 95% of values drawn from the distribution will be less than
 3/lambda. Parameter p3 unused.

 RVType.GAMMA: Gamma(p1, p2), where p1 > 0 is the shape ("kappa") and p2 > 0 is the scale ("theta"). The domain is
 again [0..+inf], and p3 is the max spread. We require that p3 >= mean + 3*stddev. The mean of a gamma distribution
 is kappa*theta, while the standard deviation is sqrt(kappa)*theta.

 IMPORTANT: If a distribution parameter is invalid, it is auto-corrected silently.

 @param t The random variable type. If RVType.FUNCTION, RVType.UNIFORM is used instead.
 @param seed The seed used to initialize an internal 32-bit integer RNG. Auto-corrected if not strictly positive.
 @param p1, p2, p3 Parameters defining the distribution. See description above.
*/
CCxRandomVar::CCxRandomVar(RVType t, int seed, double p1, double p2, double p3)
{
   m_type = (t < 0 || t >= NUMRVTYPES) ? UNIFORM : t;

   m_RNGEngine.SetSeed(seed);
   m_bStdNormValReady = FALSE;

   if(m_type == UNIFORM)
   {
      // uniform(lo, hi): auto-correct upper bound if it is not greater than lower bound specified.
      m_dParams[0] = p1;
      m_dParams[1] = (p1 >= p2) ? (p1 + 1.0) : p2;
   }
   else if(m_type == GAUSSIAN)
   {
      // normal(mean, std): Auto-correct non-positive std-dev. Max spread in p3 must be at least 3*std.
      m_dParams[0] = p1;
      m_dParams[1] = (p2 <= 0) ? cMath::abs(p1) : p2;
      m_dParams[2] = (p3 < m_dParams[1]*3.0) ? m_dParams[1]*3.0 : p3;
   }
   else if(m_type == EXPONENTIAL)
   {
      // expon(lambda): Auto-correct non-positive lambda. Max spread in p2 must >= 3/lambda.
      m_dParams[0] = (p1 <= 0) ? 1.0 : p1;
      m_dParams[1] = (p2 < 3.0/m_dParams[0]) ? 3.0/m_dParams[0] : p2;
   }
   else if(m_type == GAMMA)
   {
      // gamma(k,theta): Auto-correct non-positive shape k, scale theta. Max spread in p3 >= theta*(k + 3*sqrt(k)).
      m_dParams[0] = (p1 <= 0) ? 1.0 : p1;
      m_dParams[1] = (p2 <= 0) ? 1.0 : p2;
      double minSpread = m_dParams[1]*(m_dParams[0] + 3.0*::sqrt(m_dParams[0]));
      m_dParams[2] = (p3 < minSpread) ? minSpread : p3;
   }
}

/**
 Draw the next floating-point value from this random variable's distribution. 
 @return The next random variate.
*/
double CCxRandomVar::Get()
{
   double out = 0;
   switch(m_type)
   {
   case UNIFORM: 
      // uniform(A,B) = A + (B-A)*uniform(0,1)
      out = m_dParams[0] + _genStandardUniform()*(m_dParams[1]-m_dParams[0]);
      break;
   case GAUSSIAN: 
      // gauss(M,S) = M + S*gauss(0,1). But reject values that are beyond +/-spread of mean
      do { out = m_dParams[1]*_genStandardGauss(); } while(cMath::abs(out) > m_dParams[2]);
      out += m_dParams[0];
      break;
   case EXPONENTIAL: 
      // expon(L) = -ln(uniform(0,1))/L. But reject values that are greater than spread
      do { out = -::log(_genStandardUniform())/m_dParams[0]; } while(out > m_dParams[1]);
      break;
   default: // RVType::GAMMA
      // gamma(K,S) = S*gamma(K,1). But reject values that are greater than spread
      do { out = m_dParams[1]*_genGammaUnitScale(m_dParams[0]); } while(out > m_dParams[2]);
      break;
   }

   return(out);
}

/**
 Generate a random variate drawn from the uniform distribution defined on the interval (0..1), endpoints excluded.
 @return The random variate.
*/
double CCxRandomVar::_genStandardUniform()
{
   return(m_RNGEngine.Generate());
}

/**
 Generate a random variate drawn from the standard normal distribution, with mean 0 and variance 1. This method
 implements the "gasdev" algorithm in "Numerical Recipes for C". See CREDITS in the file header.

 @return The random variate.
*/
double CCxRandomVar::_genStandardGauss()
{
   double dVal = 0;
   if(!m_bStdNormValReady)
   {
      // get two uniform(0,1) deviates v1,v2 such that (v1,v2) lies inside the unit circle, but not at the origin
      double v1, v2, rsq, fac;
      do 
      { 
         v1 = 2.0 * _genStandardUniform() - 1.0; 
         v2 = 2.0 * _genStandardUniform() - 1.0;
         rsq = v1*v1 + v2*v2;
      } while( rsq >= 1.0 || rsq == 0.0 );

      // use Box-Muller transformation to transform the two uniform deviates into Gaussian deviates, one of which is
      // saved for the next call to this function!
      fac = sqrt( -2.0 * log(rsq) / rsq ); 
      m_dStdNormValNext = v1*fac; 
      m_bStdNormValReady = TRUE; 
      dVal = v2*fac;
   }
   else 
   {
      dVal = m_dStdNormValNext;
      m_bStdNormValReady = FALSE;
   }

   return( dVal );
}

/**
 Generate a random variate from a gamma distribution with specified shape parameter and a scale of 1: gamma(k,1).
 Implements Marsaglia and Tsang's approach, which relies on drawing variates from normal(0,1) and uniform(0,1),
 then applying an accept/reject condition. See CREDITS in file header.
 
 For other values of the scale factor, note that gamma(k,S) = S*gamma(k,1).

 @param k The desired shape parameter. Must be strictly positive. If not, 1 is assumed!
*/
double CCxRandomVar::_genGammaUnitScale(double k)
{
   if(k <= 0) k = 1.0;

   // if scale is less than 1, we add one and compute gamma(k,1) = gamma(k+1,1)*pow(uniform(0,1), 1.0/k)
   double dAdjFac = 1.0;
   BOOL bAdj = FALSE;
   if(k < 1)
   {
      dAdjFac = ::pow(_genStandardUniform(), 1.0 / k);
      k = k + 1.0;
      bAdj = TRUE;
   }

   double d = k - 1.0/3.0;
   double c = 1.0/(3.0*::sqrt(d));
   double rvNorm, rvUnif, v;

   while(TRUE)
   {
      do
      {
         rvNorm = _genStandardGauss();
         v = 1.0 + c * rvNorm;
      } while(v <= 0);

      v = v*v*v;
      rvUnif = _genStandardUniform();

      // accept/reject: this quick calccan avoid the slower log() calls much of the time
      if(rvUnif < 1 - 0.0331*rvNorm*rvNorm*rvNorm*rvNorm) 
         break;
      if(::log(rvUnif) < 0.5*rvNorm*rvNorm + d *(1 - v + ::log(v)))
         break;
   }

   v = d*v;
   if(bAdj) v = dAdjFac*v;
   return(v);
}