#ifndef _MATH_H_
#define _MATH_H_

/* $Id: math.h,v 1.2 2008/01/11 13:35:45 fredfoobar Exp $ */

#ifndef FLT_EVAL_METHOD
#define FLT_EVAL_METHOD 0
#endif

#if FLT_EVAL_METHOD == 1
typedef double	float_t;
typedef double	double_t;
#else
#if FLT_EVAL_METHOD == 2
typedef long double	float_t;
typedef long double	double_t;
#else
typedef float	float_t;
typedef double	double_t;
#endif
#endif

/* define these macros:
int fpclassify(real-floating x);
int isfinite(real-floating x);
int isinf(real-floating x);
int isnan(real-floating x);
int isnormal(real-floating x);
int signbit(real-floating x);
int isgreater(real-floating x, real-floating y);
int isgreaterequal(real-floating x, real-floating y);
int isless(real-floating x, real-floating y);
int islessequal(real-floating x, real-floating y);
int islessgreater(real-floating x, real-floating y);
int isunordered(real-floating x, real-floating y);

M_E		Value of e
M_LOG2E		Value of log 2 e
M_LOG10E	Value of log 10 e
M_LN2		Value of log e 2
M_LN10		Value of log e 10
M_PI		Value of pi
M_PI_2		Value of pi/2
M_PI_4		Value of pi/4
M_1_PI		Value of 1/pi
M_2_PI		Value of 2/pi
M_2_SQRTPI	Value of 2/sqrt(pi)
M_SQRT2		Value of sqrt(2)
M_SQRT1_2	Value of sqrt(1/2)

MAXFLOAT
        [XSI] [Option Start] Value of maximum non-infinite
        single-precision floating-point number. [Option End]
HUGE_VAL
        A positive double expression, not necessarily representable as a
        float. Used as an error value returned by the mathematics
        library. HUGE_VAL evaluates to +infinity on systems supporting
        IEEE Std 754-1985.
HUGE_VALF
        A positive float constant expression. Used as an error value
        returned by the mathematics library. HUGE_VALF evaluates to
        +infinity on systems supporting IEEE Std 754-1985.
HUGE_VALL
        A positive long double constant expression. Used as an error
        value returned by the mathematics library. HUGE_VALL evaluates
        to +infinity on systems supporting IEEE Std 754-1985.
INFINITY
        A constant expression of type float representing positive or
        unsigned infinity, if available; else a positive constant of
        type float that overflows at translation time.
NAN
        A constant expression of type float representing a quiet NaN.
        This symbolic constant is only defined if the implementation
        supports quiet NaNs for the float type.

    The following macros shall be defined for number classification. They
    represent the mutually-exclusive kinds of floating-point values. They
    expand to integer constant expressions with distinct values. Additional
    implementation-defined floating-point classifications, with macro
    definitions beginning with FP_ and an uppercase letter, may also be
    specified by the implementation.

FP_INFINITE
FP_NAN
FP_NORMAL
FP_SUBNORMAL
FP_ZERO

    The following optional macros indicate whether the fma() family of
    functions are fast compared with direct code:

FP_FAST_FMA
FP_FAST_FMAF
FP_FAST_FMAL

    If defined, the FP_FAST_FMA macro shall indicate that the fma() function
    generally executes about as fast as, or faster than, a multiply and an
    add of double operands. If undefined, the speed of execution is
    unspecified. The other macros have the equivalent meaning for the float
    and long double versions.

    The following macros shall expand to integer constant expressions whose
    values are returned by ilogb( x) if x is zero or NaN, respectively. The
    value of FP_ILOGB0 shall be either {INT_MIN} or - {INT_MAX}. The value
    of FP_ILOGBNAN shall be either {INT_MAX} or {INT_MIN}.

FP_ILOGB0
FP_ILOGBNAN

*/
#define MATH_ERRNO	1
#define MATH_ERREXCEPT	2
/*

    The following macro shall expand to an expression that has type int and
    the value MATH_ERRNO, MATH_ERREXCEPT, or the bitwise-inclusive OR of
    both:

math_errhandling

    The value of math_errhandling is constant for the duration of the
    program. It is unspecified whether math_errhandling is a macro or an
    identifier with external linkage. If a macro definition is suppressed or
    a program defines an identifier with the name math_errhandling , the
    behavior is undefined. If the expression (math_errhandling &
    MATH_ERREXCEPT) can be non-zero, the implementation shall define the
    macros FE_DIVBYZERO, FE_INVALID, and FE_OVERFLOW in <fenv.h>.

    The following shall be declared as functions and may also be defined as
    macros. Function prototypes shall be provided.

*/

double		acos(double __x);
float		acosf(float __x);
double		acosh(double __x);
float		acoshf(float __x);
long double	acoshl(long double __x);
long double	acosl(long double __x);
double		asin(double __x);
float		asinf(float __x);
double		asinh(double __x);
float		asinhf(float __x);
long double	asinhl(long double __x);
long double	asinl(long double __x);
double		atan(double __x);
double		atan2(double __y, double __x);
float		atan2f(float __y, float __x);
long double	atan2l(long double __y, long double __x);
float		atanf(float __x);
double		atanh(double __x);
float		atanhf(float __x);
long double	atanhl(long double __x);
long double	atanl(long double __x);
double		cbrt(double __x);
float		cbrtf(float __x);
long double	cbrtl(long double __x);
double		ceil(double __x);
float		ceilf(float __x);
long double	ceill(long double __x);
double		copysign(double __x, double __y);
float		copysignf(float __x, float __y);
long double	copysignl(long double __x, long double __y);
double		cos(double __x);
float		cosf(float __x);
double		cosh(double __x);
float		coshf(float __x);
long double	coshl(long double __x);
long double	cosl(long double __x);
double		erf(double __x);
double		erfc(double __x);
float		erfcf(float __x);
long double	erfcl(long double __x);
float		erff(float __x);
long double	erfl(long double __x);
double		exp(double __x);
double		exp2(double __x);
float		exp2f(float __x);
long double	exp2l(long double __x);
float		expf(float __x);
long double	expl(long double __x);
double		expm1(double __x);
float		expm1f(float __x);
long double	expm1l(long double __x);
double		fabs(double __x);
float		fabsf(float __x);
long double	fabsl(long double __x);
double		fdim(double __x, double __y);
float		fdimf(float __x, float __y);
long double	fdiml(long double __x, long double __y);
double		floor(double __x);
float		floorf(float __x);
long double	floorl(long double __x);
double		fma(double __x, double __y, double __z);
float		fmaf(float __x, float __y, float __z);
long double	fmal(long double __x, long double __y, long double __z);
double		fmax(double __x, double __y);
float		fmaxf(float __x, float __y);
long double	fmaxl(long double __x, long double __y);
double		fmin(double __x, double __y);
float		fminf(float __x, float __y);
long double	fminl(long double __x, long double __y);
double		fmod(double __x, double __y);
float		fmodf(float __x, float __y);
long double	fmodl(long double __x, long double __y);
double		frexp(double __x, int *__exp);
float		frexpf(float  __x, int *__exp);
long double	frexpl(long double	__x, int *__exp);
double		hypot(double __x, double __y);
float		hypotf(float __x, float __y);
long double	hypotl(long double __x, long double __y);
int		ilogb(double __x);
int		ilogbf(float __x);
int		ilogbl(long double __x);

/*
[XSI][Option Start]
double      j0(double);
double      j1(double);
double      jn(int, double);
[Option End]
*/

double		ldexp(double __x, int __exp);
float		ldexpf(float __x, int __exp);
long double	ldexpl(long double __x, int __exp);
double		lgamma(double __x);
float		lgammaf(float __x);
long double	lgammal(long double __x);
long long	llrint(double __x);
long long	llrintf(float __x);
long long	llrintl(long double __x);
long long	llround(double __x);
long long	llroundf(float __x);
long long	llroundl(long double __x);
double		log(double __x);
double		log10(double __x);
float		log10f(float __x);
long double	log10l(long double __x);
double		log1p(double __x);
float		log1pf(float __x);
long double	log1pl(long double __x);
double		log2(double __x);
float		log2f(float __x);
long double	log2l(long double __x);
double		logb(double __x);
float		logbf(float __x);
long double	logbl(long double __x);
float		logf(float __x);
long double	logl(long double __x);
long		lrint(double __x);
long		lrintf(float __x);
long		lrintl(long double __x);
long		lround(double __x);
long		lroundf(float __x);
long		lroundl(long double __x);
double		modf(double __x, double *__iptr);
float		modff(float __x, float *__iptr);
long double	modfl(long double __x, long double *__iptr);
double		nan(const char *__tagp);
float		nanf(const char *__tagp);
long double	nanl(const char *__tagp);
double		nearbyint(double __x);
float		nearbyintf(float __x);
long double	nearbyintl(long double __x);
double		nextafter(double __x, double __y);
float		nextafterf(float __x, float __y);
long double	nextafterl(long double __x, long double __y);
double		nexttoward(double __x, long double __y);
float		nexttowardf(float __x, long double __y);
long double	nexttowardl(long double __x, long double __y);
double		pow(double __x, double __y);
float		powf(float __x, float __y);
long double	powl(long double __x, long double __y);
double		remainder(double __x, double __y);
float		remainderf(float __x, float __y);
long double	remainderl(long double __x, long double __y);
double		remquo(double __x, double __y, int *__quo);
float		remquof(float __x, float __y, int *__quo);
long double	remquol(long double __x, long double __y, int *__quo);
double		rint(double __x);
float		rintf(float __x);
long double	rintl(long double __x);
double		round(double __x);
float		roundf(float __x);
long double	roundl(long double __x);

/*
[XSI][Option Start]
double      scalb(double, double);
[Option End]
*/

double		scalbln(double __x, long __n);
float		scalblnf(float __x, long __n);
long double	scalblnl(long double, long __n);
double		scalbn(double __x, int __n);
float		scalbnf(float __x, int __n);
long double	scalbnl(long double, int __n);
double		sin(double __x);
float		sinf(float __x);
double		sinh(double __x);
float		sinhf(float __x);
long double	sinhl(long double __x);
long double	sinl(long double __x);
double		sqrt(double __x);
float		sqrtf(float __x);
long double	sqrtl(long double __x);
double		tan(double __x);
float		tanf(float __x);
double		tanh(double __x);
float		tanhf(float __x);
long double	tanhl(long double __x);
long double	tanl(long double __x);
double		tgamma(double __x);
float		tgammaf(float __x);
long double	tgammal(long double __x);
double		trunc(double __x);
float		truncf(float __x);
long double	truncl(long double __x);

/*
[XSI][Option Start]
double      y0(double);
double      y1(double);
double      yn(int, double);
[Option End]
*/
extern int	signgam;

#endif /* _MATH_H_ */
