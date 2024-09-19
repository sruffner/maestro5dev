//=====================================================================================================================
//
// utilities.h : Declaration of several general-utility classes.
//
//=====================================================================================================================

#if !defined(UTILITIES_H__INCLUDED_)
#define UTILITIES_H__INCLUDED_

#include "math.h"


//=====================================================================================================================
// Declaration of class cMath
//
// This class is not intended for instantiation.  Rather it provides a set of useful math constants and INLINE methods
// for general purpose use.
//=====================================================================================================================

class cMath
{
public:
   static int abs( const int i )                         { return( (i < 0) ? -i : i ); }
   static float abs( const float f )                     { return( (f < 0) ? -f : f ); }
   static double abs( const double d )                   { return( (d < 0) ? -d : d ); }

   static int min( const int a, const int b )            { return( (a < b) ? a : b ); }
   static int max( const int a, const int b )            { return( (a > b) ? a : b ); }
   static float min( const float a, const float b )      { return( (a < b) ? a : b ); }
   static float max( const float a, const float b )      { return( (a > b) ? a : b ); }
   static double min( const double a, const double b )   { return( (a < b) ? a : b ); }
   static double max( const double a, const double b )   { return( (a > b) ? a : b ); }

   static int signof( const int i )          { return( (i < 0) ? -1 : ((i>0) ? 1 : 0) ); }
   static int signof( const float f )        { return( (f < 0) ? -1 : ((f>0) ? 1 : 0) ); }
   static int signof( const double d )       { return( (d < 0) ? -1 : ((d>0) ? 1 : 0) ); }

   static double toRadians( const double d ) { return( d*M_PI/180.0 ); }
   static float toRadians( const float f )   { return( float( double(f)*M_PI/180.0 ) ); }
   static double toDegrees( const double d ) { return( d*180.0/M_PI ); }
   static float toDegrees( const float f )   { return( float( double(f)*180.0/M_PI ) ); }

   static double sincos( const double d )    { return( ::sin( d ) * ::cos( d ) ); }
   static double sincosDeg( const double d ) { double r = d*M_PI/180.0; return( ::sin( r ) * ::cos( r ) ); }
   static double sinDeg( const double d )    { return( ::sin( d*M_PI/180.0 ) ); }
   static double cosDeg( const double d )    { return( ::cos( d*M_PI/180.0 ) ); }
   static double tanDeg( const double d )    { return( ::tan( d*M_PI/180.0 ) ); }
   static float sinDeg( const float f )      { return( float(::sin( double(f)*M_PI/180.0 )) ); }
   static float cosDeg( const float f )      { return( float(::cos( double(f)*M_PI/180.0 )) ); }
   static float tanDeg( const float f )      { return( float(::tan( double(f)*M_PI/180.0 )) ); }

   static double atan2Deg( const double y, const double x ) { return( ::atan2(y, x)*180.0/M_PI ); }
   static float atan2Deg( const float y, const float x ) { return( float(::atan2(y, x)*180.0/M_PI) ); }

   static double frac( double d )            { int i = (int)d; return( d - (double)i ); }

   static int rangeLimit( int i, double dMin, double dMax )
   {
      int m = int(dMin); int M = int(dMax); return( (i<m) ? m : ((i>M) ? M : i) );
   }
   static float rangeLimit( float f, double dMin, double dMax )
   {
      float m = float(dMin); float M = float(dMax); return( (f<m) ? m : ((f>M) ? M : f) );
   }
   static double rangeLimit( double d, double dMin, double dMax )
   {
      return( (d<dMin) ? dMin : ((d>dMax) ? dMax : d) );
   }

   static int limitToUnitCircleDeg( int iDeg )
   {
      int j = iDeg % 360; return( (j<0) ? (j+360) : j );
   }
   static double limitToUnitCircleDeg( double dDeg )
   {
      double d = ::fmod(dDeg,360.0); return( (d<0.0) ? (d+360.0) : d );
   }
   static float limitToUnitCircleDeg( float fDeg )
   {
      return( (float) cMath::limitToUnitCircleDeg( (double) fDeg ) );
   }

};




//=====================================================================================================================
// Declaration of class CFPoint
//
// This class encapsulates a point in the 2D Cartesian plane with float-valued coords.  The origin of our coord system
// is (0,0), the "x-axis" increases rightward, and the "y-axis" increases upward.
//=====================================================================================================================

class CFPoint
{
//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   float h;                        // the horizontal coordinate
   float v;                        // the vertical coordinate


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
public:
   CFPoint()                                       { h = 0.0f; v = 0.0f; }
   CFPoint( int i )                                { h = float(i); v = float(i); }
   CFPoint( int x, int y )                         { h = float(x); v = float(y); }
   CFPoint( float f )                              { h = f; v = f; }
   CFPoint( float x, float y )                     { h = x; v = y; }
   CFPoint( double d )                             { h = float(d); v = float(d); }
   CFPoint( double x, double y )                   { h = float(x); v = float(y); }
   CFPoint( const CFPoint& src )                   { h = src.h; v = src.v; }

   ~CFPoint()                                      {}


//=====================================================================================================================
// OPERATIONS/ATTRIBUTES
//=====================================================================================================================
public:
   float GetH() const                              { return( h ); }
   float GetV() const                              { return( v ); }

   void Zero()                                     { h = 0.0f; v = 0.0f; }
   void Set( int x, int y )                        { h = float(x); v = float(y); }
   void Set( float x, float y )                    { h = x; v = y; }
   void Set( double x, double y )                  { h = float(x); v = float(y); }
   void SetPolar( float r, float theta )           { h = r * cMath::cosDeg( theta ); v = r * cMath::sinDeg( theta ); }
   void SetPolar( double r, double theta )         {
                                                      h = float( r * cMath::cosDeg(theta) );
                                                      v = float( r * cMath::sinDeg(theta) );
                                                   }
   void SetH( int x )                              { h = float(x); }
   void SetH( float x )                            { h = x; }
   void SetH( double x )                           { h = float(x); }
   void SetV( int y )                              { v = float(y); }
   void SetV( float y )                            { v = y; }
   void SetV( double y )                           { v = float(y); }
   void OffsetH( int x )                           { h += float(x); }
   void OffsetH( float x )                         { h += x; }
   void OffsetH( double x )                        { h += float(x); }
   void OffsetV( int y )                           { v += float(y); }
   void OffsetV( float y )                         { v += y; }
   void OffsetV( double y )                        { v += float(y); }
   void Offset( const CFPoint& pt )                { h += pt.h; v += pt.v; }
   void Offset( int x, int y )                     { h += float(x); v += float(y); }
   void Offset( float x, float y )                 { h += x; v += y; }
   void Offset( double x, double y )               { h += float(x); v += float(y); }
   void Truncate()                                 { int i = int(h); h = float(i); i = int(v); v = float(i); }
   void DiscardIntegerPart()                       { h = h - int(h); v = v - int(v); }

   void operator+=( const CFPoint& pt )            { h += pt.h; v += pt.v; }
   void operator-=( const CFPoint& pt )            { h -= pt.h; v -= pt.v; }
   void operator*=( const CFPoint& pt )            { h *= pt.h; v *= pt.v; }
   void operator*=( const int a )                  { h *= float(a); v *= float(a); }
   void operator*=( const float a )                { h *= a; v *= a; }
   void operator*=( const double a )               { h *= float(a); v *= float(a); }
   const CFPoint& operator=( const CFPoint& src )  { h = src.h; v = src.v; return( *this ); }

   CFPoint operator-() const                       { return( CFPoint( -h, -v ) ); }
   CFPoint operator-( const CFPoint& pt ) const    { return( CFPoint( h - pt.h, v - pt.v ) ); }
   CFPoint operator+( const CFPoint& pt ) const    { return( CFPoint( h + pt.h, v + pt.v ) ); }
   CFPoint operator*( const int f ) const          { return( CFPoint( h * float(f), v * float(f) ) ); }
   CFPoint operator*( const float f ) const        { return( CFPoint( h * f, v * f ) ); }
   CFPoint operator*( const double f ) const       { return( CFPoint( h * float(f), v * float(f) ) ); }
   CFPoint operator*( const CFPoint& pt ) const    { return( CFPoint( h * pt.h, v * pt.v ) ); }

   CFPoint IntegerPart() const                     { return( CFPoint( int(h), int(v) ) ); }
   CFPoint FractionalPart() const                  { CFPoint p=IntegerPart(); p.h= h - p.h; p.v= v - p.v; return(p); }

   bool operator==( const CFPoint& pt ) const      { return( h == pt.h && v == pt.v ); }
   bool operator!=( const CFPoint& pt ) const      { return( h != pt.h || v != pt.v ); }

   bool IsNear( CFPoint& pt, float f ) const       {
                                                      float fa = cMath::abs( f );
                                                      return( cMath::abs(h-pt.h) <= fa && cMath::abs(v-pt.v) <= fa );
                                                   }
   bool IsNear( CFPoint& pt, CFPoint& bnd ) const  {
                                                      float hB = cMath::abs( bnd.GetH() );
                                                      float vB = cMath::abs( bnd.GetV() );
                                                      return( cMath::abs(h-pt.h) <= hB && cMath::abs(v-pt.v) <= vB );
                                                   }
   bool IsFar( CFPoint& pt, float f ) const        {
                                                      float fa = cMath::abs( f );
                                                      return( cMath::abs(h-pt.h) > fa || cMath::abs(v-pt.v) > fa );
                                                   }
   bool IsFar( CFPoint& pt, CFPoint& bnd ) const   {
                                                      float hB = cMath::abs( bnd.GetH() );
                                                      float vB = cMath::abs( bnd.GetV() );
                                                      return( cMath::abs(h-pt.h) > hB || cMath::abs(v-pt.v) > vB );
                                                   }

   float DistSquared() const                       { return( h*h + v*v ); }
   float Distance() const                          { return( float(::sqrt( double(h*h + v*v) )) ); }

   float DistSquared( const CFPoint& pt ) const    { return( (h-pt.h)*(h-pt.h) + (v-pt.v)*(v-pt.v) ); }
   float Distance( const CFPoint& pt ) const       { return( float(::sqrt( double(DistSquared(pt)) )) ); }
};




//=====================================================================================================================
// Declaration of class CFRect
//
// This class encapsulates a rectangle in the 2D Cartesian plane with four floats, representing the top, left, bottom,
// and right edges of the rectangle.  The origin of our coord system is (0,0), the "x-axis" increases rightward, and
// the "y-axis" increases upward.  The rectangle is ALWAYS "normalized" in the sense: left <= right, bot <= top.
//=====================================================================================================================

class CFRect
{
//=====================================================================================================================
// DATA OBJECTS
//=====================================================================================================================
private:
   float t, l;                      // top left corner of rectangle
   float b, r;                      // bottom right corner of rectangle


//=====================================================================================================================
// CONSTRUCTION/DESTRUCTION
//=====================================================================================================================
public:
   CFRect()                                        {}
   CFRect( const double w )                        { Set( w ); }
   CFRect( const double w, const double h )        { Set( w, h ); }
   CFRect( const double dl, const double dt, const double dr, const double db ) { Set( dl, dt, dr, db ); }
   CFRect( const CFPoint& tl, const CFPoint& br )  { Set( tl, br ); }
   CFRect( const CFPoint& ctr, const double w, const double h ) { Set( ctr, w, h ); }
   CFRect( const CFRect& src )                     { l = src.Left(); t = src.Top(); r = src.Right(); b = src.Bot(); }

   ~CFRect()                                       {}


//=====================================================================================================================
// OPERATIONS/ATTRIBUTES
//=====================================================================================================================
private:
   void Normalize()                                {
                                                      float f;
                                                      if( l > r ) { f = l; l = r; r = f; }
                                                      if( b > t ) { f = t; t = b; b = f; }
                                                   }
public:
   CFPoint TopLeft() const                         { return( CFPoint( l, t ) ); }
   CFPoint BotRight() const                        { return( CFPoint( r, b ) ); }
   CFPoint Center() const                          { return( CFPoint( (l+r)/2.0f, (t+b)/2.0f ) ); }
   float Left() const                              { return( l ); }
   float Right() const                             { return( r ); }
   float Top() const                               { return( t ); }
   float Bot() const                               { return( b ); }
   float Width() const                             { return( r - l ); }
   float Height() const                            { return( t - b ); }
   float Area() const                              { return( (r - l)*(t - b) ); }

   void Empty()                                    { t = l = b = r = 0.0f; }
   void Set( const double s )                      { float fs = float(cMath::abs( s/2.0 )); l = b = -fs; t = r = fs; }
   void Set( const double w, const double h )      {
                                                      float x = float( cMath::abs( w/2.0 ) );
                                                      float y = float( cMath::abs( h/2.0 ) );
                                                      l = -x; r = x; t = y; b = -y;
                                                   }
   void Set( const double dl, const double dt, const double dr, const double db )
                                                   {
                                                      l = float(dl); t = float(dt); r = float(dr); b = float(db);
                                                      Normalize();
                                                   }
   void Set( const CFPoint& tl,const CFPoint& br ) {
                                                      l = tl.GetH(); t = tl.GetV(); r = br.GetH(); b = br.GetV();
                                                      Normalize();
                                                   }
   void Set( const CFPoint& ctr, const double w, const double h )
                                                   {
                                                      float x = float( cMath::abs( w/2.0 ) );
                                                      float y = float( cMath::abs( h/2.0 ) );
                                                      l = ctr.GetH() - x; r = ctr.GetH() + x;
                                                      b = ctr.GetV() - y; t = ctr.GetV() + y;
                                                   }

   void Scale( const double dh, const double dv )  { l *= float(dh); r *= float(dh); t *= float(dv); b *= float(dv); }
   void Offset( const CFPoint& pt )                { l += pt.GetH(); r += pt.GetH(); t += pt.GetV(); b += pt.GetV(); }
   void Offset( double dh, double dv )             { l += float(dh); r += float(dh); t += float(dv); b += float(dv); }
   void CenterAt( const CFPoint& pt )              { Offset( pt - Center() ); }

   void Truncate()                                 {
                                                      int i = int(l); l = float(i); i = int(r); r = float(i);
                                                          i = int(t); t = float(i); i = int(b); b = float(i);
                                                   }
   void RangeRestrict( const double dMin, const double dMax )
                                                   {
                                                      float fMin = float(dMin); float fMax = float(dMax);
                                                      l = (l<fMin) ? fMin : ((l>fMax) ? fMax : l);
                                                      t = (t<fMin) ? fMin : ((t>fMax) ? fMax : t);
                                                      r = (r<fMin) ? fMin : ((r>fMax) ? fMax : r);
                                                      b = (b<fMin) ? fMin : ((b>fMax) ? fMax : b);
                                                   }

   const CFRect& operator=( const CFRect& src )    {
                                                      l = src.Left(); t = src.Top(); r = src.Right(); b = src.Bot();
                                                      return( *this );
                                                   }
};




//=====================================================================================================================
// Declaration of class CElapsedTime
//=====================================================================================================================

class CElapsedTime
{
private:
   double  m_dTimeZero;             // high-resolution clock reading (in seconds) when elapsed time last "reset"

public:
   void reset();                    // reset elapsed time to zero

   CElapsedTime() { reset(); }
   ~CElapsedTime() {}

   double get();                    // get elapsed time in seconds since last reset
   double getAndReset();            // get elapsed time in seconds since last reset, then reset

   static bool isSupported();       // verify that host machine supports high-res clock required by CElapsedTime
};



//=====================================================================================================================
// Declaration of ABSTRACT class CRandomNG
//
// This class represents a pseudo-random number generator that returns a sequence of floating-point values.  The range
// and statistical distribution are defined by the implementing class.
//=====================================================================================================================

class CRandomNG
{
public:
   CRandomNG() {}                         // constructor -- start out with a seed of 1
   ~CRandomNG() {}                        // destructor

   virtual void setSeed( int seed ) = 0;  // initialize random-number generator with specified (nonzero) seed
   virtual double generate() = 0;         // generate next random number in (0.0 .. 1.0)
};




//=====================================================================================================================
// Declaration of class CUniformRNG
//
// This class encapsulates a pseudo-random number generator that returns a sequence of uniformly distributed floating-
// point values in (0.0 .. 1.0), endpoints excluded.
//=====================================================================================================================

class CUniformRNG : public CRandomNG
{
private:
   enum { TABLESZ = 32 };                 // size of shuffle table
   static int LC_M;                       // parameters of internal linear congruential generator
   static int LC_A;                       // I' = A*I % M, using Schrage's method (Q,R) to avoid integer overflow
   static int LC_Q;
   static int LC_R;
   static int NDIV;
   static double DSCALE;                  // we multiply by 1/M to convert int to double value in (0.0..1.0)

   int m_shuffle[TABLESZ];                // the shuffle table
   int m_lastOut;                         // the last integer spit out of shuffle table
   int m_curr;                            // current value I of the linear congruential generator

public:
   CUniformRNG() { setSeed( 1 ); }        // constructor -- start out with a seed of 1
   ~CUniformRNG() {}                      // destructor

   void setSeed( int seed );              // initialize random-number generator with specified seed
   double generate();                     // generate next random number in (0.0 .. 1.0)
};


#endif   // !defined(UTILITIES_H__INCLUDED_)
