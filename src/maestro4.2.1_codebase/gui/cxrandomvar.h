//=====================================================================================================================
//
// cxrandomvar.h : Declaration of class CCxRandomVar, which represents a Maestro trial "random variable" during active
// trial sequencing. 
//
// ****** FOR DESCRIPTION, REVISION HISTORY, ETC, SEE IMPLEMENTATION FILE ******
//
//=====================================================================================================================

#if !defined(CXRANDOMVAR_H__INCLUDED_)
#define CXRANDOMVAR_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "util.h"                // for the RNG engine

class CCxRandomVar
{
public:
   typedef enum {
      UNIFORM = 0,               // uniform(lo, hi)
      GAUSSIAN,                  // normal(mean, stddev, abs max cutoff)
      EXPONENTIAL,               // exponential(rate, max cutoff)
      GAMMA                      // gamma(shape, scale, max cutoff)
   } RVType;

   static const int NUMRVTYPES = 4;

private:
   // the RV's distribution type
   RVType m_type;

   // parameters defining the RV's distribution; varies with distribution type
   double m_dParams[3];

   // a RNG engine that generates U(0,1)
   CUniformRNG m_RNGEngine;

   // the algorithm using the RNG engine to generate values on a normal(0,1) distribution generates two
   // values on each call to _genStandardGauss(). These variables provide the necessary state.
   BOOL m_bStdNormValReady;
   double m_dStdNormValNext;

   // no copy constructor or assignment operator defined
   CCxRandomVar(const CCxRandomVar& src); 
   CCxRandomVar& operator=(const CCxRandomVar& src);

public: 
   CCxRandomVar();
   CCxRandomVar(RVType t, int seed, double p1, double p2, double p3);
   ~CCxRandomVar() {}

   double Get();

private: 
   // generate a random variate from uniform(0,1)
   double _genStandardUniform();
   // generate a random variate from gaussian(0,1)
   double _genStandardGauss();
   // generate a random variate from gamma(k, 1)
   double _genGammaUnitScale(double k);
};


#endif   // !defined(CXRANDOMVAR_H__INCLUDED_)
