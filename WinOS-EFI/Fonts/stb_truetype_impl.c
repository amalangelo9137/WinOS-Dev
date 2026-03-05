#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <stdint.h>

// Map allocation/copy to UEFI
#define STBTT_malloc(size, userdata)    AllocatePool(size)
#define STBTT_free(ptr, userdata)       FreePool(ptr)
#define STBTT_memcpy(dest, src, n)      CopyMem((dest),(src),(n))
#define STBTT_memset(ptr, val, n)       SetMem((ptr),(n),(val))

// satisfy MSVC float helper
int _fltused = 0;

// Minimal math helpers used by stb_truetype (avoid linking CRT)
static double stb_fabs(double x) { return x < 0.0 ? -x : x; }
static int    stb_ifloor(double x) { int i = (int)x; return (x < 0.0 && (double)i != x) ? i-1 : i; }
static int    stb_iceil (double x) { int i = (int)x; return (x > 0.0 && (double)i != x) ? i+1 : i; }
static double stb_sqrt(double x) { if (x <= 0.0) return 0.0; double r = x; for (int i=0;i<16;++i) r = 0.5*(r + x/r); return r; }

// cube-root used by stb; implement directly and map STBTT_pow to it for the cube-root case
static double stb_cuberoot(double x) {
    if (x == 0.0) return 0.0;
    double sign = x < 0.0 ? -1.0 : 1.0;
    double a = sign < 0 ? -x : x;
    double r = a;
    for (int i = 0; i < 20; ++i) r = (2.0*r + a/(r*r)) / 3.0;
    return sign * r;
}

// Provide standard names expected by stb_truetype
double Floor(double x) { return (double)stb_ifloor(x); }
double Ceil(double x)  { return (double)stb_iceil(x); }
double Sqrt(double x)  { return stb_sqrt(x); }
double Fabs(double x)  { return stb_fabs(x); }

// Map STB macros used inside header
#define STBTT_ifloor(x) stb_ifloor(x)
#define STBTT_iceil(x)  stb_iceil(x)
#define STBTT_sqrt(x)   stb_sqrt(x)
#define STBTT_fabs(x)   stb_fabs(x)
#ifndef STBTT_pow
#define STBTT_pow(x,y)  ((y) == (1.0/3.0) ? stb_cuberoot(x) : stb_cuberoot(x)) /* stb only needs cbrt */
#endif

// Avoid pulling in <assert.h> and the CRT _wassert symbol
#ifndef STBTT_assert
#define STBTT_assert(x) ((void)0)
#endif

// Implement the stb_truetype implementation
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"