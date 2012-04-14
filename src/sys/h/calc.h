#ifndef _CALC_H_
#define _CALC_H_

#if defined(TI92P) || defined(V200)
#define CALC_HAS_QWERTY_KBD 1
#define CALC_HAS_LARGE_SCREEN 1
#elif defined(TI89) || defined(TI89TI)
#define CALC_HAS_QWERTY_KBD 0
#define CALC_HAS_LARGE_SCREEN 0
#else
#error TI92P, V200, TI89, or TI89TI must be defined!
#endif

#if defined(TI92P) || defined(TI89)
#define CALC_FLASH_BASE ((void *)0x400000)
#else
#define CALC_FLASH_BASE ((void *)0x800000) // XXX: verify this
#endif

#endif
