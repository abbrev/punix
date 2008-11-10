#ifndef _INTTYPES_H_
#define _INTTYPES_H_

/* $Id$ */

/* should be POSIX compliant */

#include <stdint.h>

/* should the members be swapped? */
typedef struct {
	intmax_t quot;
	intmax_t rem;
} imaxdiv_t;

/* fprintf() macros */

/* signed integers */

/* decimal notation */
#define PRId8  "d"
#define PRId16 "d"
#define PRId32 "ld"
#define PRId64 "lld"

#define PRIdLEAST8  "d"
#define PRIdLEAST16 "d"
#define PRIdLEAST32 "ld"
#define PRIdLEAST64 "lld"

#define PRIdFAST8  "d"
#define PRIdFAST16 "d"
#define PRIdFAST32 "ld"
#define PRIdFAST64 "lld"

#define PRIdMAX "lld"
#define PRIdPTR "ld"

#define PRIi8  "i"
#define PRIi16 "i"
#define PRIi32 "li"
#define PRIi64 "lli"

#define PRIiLEAST8  "i"
#define PRIiLEAST16 "i"
#define PRIiLEAST32 "li"
#define PRIiLEAST64 "lli"

#define PRIiFAST8  "i"
#define PRIiFAST16 "i"
#define PRIiFAST32 "li"
#define PRIiFAST64 "lli"

#define PRIiMAX "lli"
#define PRIiPTR "li"

/* unsigned integers */

/* octal notation */
#define PRIo8  "o"
#define PRIo16 "o"
#define PRIo32 "lo"
#define PRIo64 "llo"

#define PRIoLEAST8  "o"
#define PRIoLEAST16 "o"
#define PRIoLEAST32 "lo"
#define PRIoLEAST64 "llo"

#define PRIoFAST8  "o"
#define PRIoFAST16 "o"
#define PRIoFAST32 "lo"
#define PRIoFAST64 "llo"

#define PRIoMAX "llo"
#define PRIoPTR "lo"

/* decimal notation */
#define PRIu8  "u"
#define PRIu16 "u"
#define PRIu32 "lu"
#define PRIu64 "llu"

#define PRIuLEAST8  "u"
#define PRIuLEAST16 "u"
#define PRIuLEAST32 "lu"
#define PRIuLEAST64 "llu"

#define PRIuFAST8  "u"
#define PRIuFAST16 "u"
#define PRIuFAST32 "lu"
#define PRIuFAST64 "llu"

#define PRIuMAX "llu"
#define PRIuPTR "lu"

/* lowercase hexadecimal notation */
#define PRIx8  "x"
#define PRIx16 "x"
#define PRIx32 "lx"
#define PRIx64 "llx"

#define PRIxLEAST8  "x"
#define PRIxLEAST16 "x"
#define PRIxLEAST32 "lx"
#define PRIxLEAST64 "llx"

#define PRIxFAST8  "x"
#define PRIxFAST16 "x"
#define PRIxFAST32 "lx"
#define PRIxFAST64 "llx"

#define PRIxMAX "llx"
#define PRIxPTR "lx"

/* uppercase hexadecimal notation */
#define PRIX8  "X"
#define PRIX16 "X"
#define PRIX32 "lX"
#define PRIX64 "llX"

#define PRIXLEAST8  "X"
#define PRIXLEAST16 "X"
#define PRIXLEAST32 "lX"
#define PRIXLEAST64 "llX"

#define PRIXFAST8  "X"
#define PRIXFAST16 "X"
#define PRIXFAST32 "lX"
#define PRIXFAST64 "llX"

#define PRIXMAX "llX"
#define PRIXPTR "lX"

/* fscanf() macros */

/* signed integers */

/* decimal notation */
#define SCNd8  "hhd"
#define SCNd16 "hd"
#define SCNd32 "ld"
#define SCNd64 "lld"

#define SCNdLEAST8  "hhd"
#define SCNdLEAST16 "hd"
#define SCNdLEAST32 "ld"
#define SCNdLEAST64 "lld"

#define SCNdFAST8  "hhd"
#define SCNdFAST16 "hd"
#define SCNdFAST32 "ld"
#define SCNdFAST64 "lld"

#define SCNdMAX "lld"

#define SCNi8  "hhi"
#define SCNi16 "hi"
#define SCNi32 "li"
#define SCNi64 "lli"

#define SCNiLEAST8  "hhi"
#define SCNiLEAST16 "hi"
#define SCNiLEAST32 "li"
#define SCNiLEAST64 "lli"

#define SCNiFAST8  "hhi"
#define SCNiFAST16 "hi"
#define SCNiFAST32 "li"
#define SCNiFAST64 "lli"

#define SCNiMAX "lli"

/* unsigned integers */

/* octal notation */
#define SCNo8  "hho"
#define SCNo16 "ho"
#define SCNo32 "lo"
#define SCNo64 "llo"

#define SCNoLEAST8  "hho"
#define SCNoLEAST16 "ho"
#define SCNoLEAST32 "lo"
#define SCNoLEAST64 "llo"

#define SCNoFAST8  "hho"
#define SCNoFAST16 "ho"
#define SCNoFAST32 "lo"
#define SCNoFAST64 "llo"

#define SCNoMAX "llo"

/* decimal notation */
#define SCNu8  "hhu"
#define SCNu16 "hu"
#define SCNu32 "lu"
#define SCNu64 "llu"

#define SCNuLEAST8  "hhu"
#define SCNuLEAST16 "hu"
#define SCNuLEAST32 "lu"
#define SCNuLEAST64 "llu"

#define SCNuFAST8  "hhu"
#define SCNuFAST16 "hu"
#define SCNuFAST32 "lu"
#define SCNuFAST64 "llu"

#define SCNuMAX "llu"

/* hexadecimal notation */
#define SCNx8  "hhx"
#define SCNx16 "hx"
#define SCNx32 "lx"
#define SCNx64 "llx"

#define SCNxLEAST8  "hhx"
#define SCNxLEAST16 "hx"
#define SCNxLEAST32 "lx"
#define SCNxLEAST64 "llx"

#define SCNxFAST8  "hhx"
#define SCNxFAST16 "hx"
#define SCNxFAST32 "lx"
#define SCNxFAST64 "llx"

#define SCNxMAX "llx"

/* hexadecimal notation (not in POSIX, but added for consistency with PRIX*) */
#define SCNX8  "hhX"
#define SCNX16 "hX"
#define SCNX32 "lX"
#define SCNX64 "llX"

#define SCNXLEAST8  "hhX"
#define SCNXLEAST16 "hX"
#define SCNXLEAST32 "lX"
#define SCNXLEAST64 "llX"

#define SCNXFAST8  "hhX"
#define SCNXFAST16 "hX"
#define SCNXFAST32 "lX"
#define SCNXFAST64 "llX"

#define SCNXMAX "llX"

intmax_t  imaxabs(intmax_t);
imaxdiv_t imaxdiv(intmax_t, intmax_t);
intmax_t  strtoimax(const char *, char **, int);
uintmax_t strtoumax(const char *, char **, int);
/*intmax_t  wcstoimax(const wchar_t *, wchar_t **, int);
uintmax_t wcstoumax(const wchar_t *, wchar_t **, int);*/

#endif
