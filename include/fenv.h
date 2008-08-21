#ifndef _FENV_H_
#define _FENV_H_

/* FIXME: not complete at all */

typedef int fenv_t;	/* FIXME */
typedef int fexcept_t;	/* FIXME */

/* FIXME */
#define FE_DIVBYZERO
#define FE_INEXACT
#define FE_INVALID
#define FE_OVERFLOW
#define FE_UNDERFLOW

#define FE_ALL_EXCEPT (FE_DIVBYZERO|FE_INEXACT|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW)

/* FIXME */
#define FE_DOWNWARD
#define FE_TONEAREST
#define FE_TOWARDZERO
#define FE_UPWARD

/* FIXME */
#define FE_DFL_ENV

int	feclearexcept(int __excepts);
int	fegetexceptflag(fexcept_t *__flagp, int __excepts);
int	feraiseexcept(int __excepts);
int	fesetexceptflag(const fexcept_t *__flagp, int __excepts);
int	fetestexcept(int __excepts);
int	fegetround(void);
int	fesetround(int __rounding_mode);
int	fegetenv(fenv_t *__envp);
int	feholdexcept(fenv_t *__envp);
int	fesetenv(const fenv_t *__envp);
int	feupdateenv(const fenv_t *__envp);

#endif
