/*
 * Name:     mx_stdint.h
 *
 * Purpose:  This header file declares sets of integer types having
 *           specified widths using the names from the <stdint.h>
 *           header file of the C99 standard.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_STDINT_H__
#define __MX_STDINT_H__

/* Determine the native word size first. */

#include <limits.h>

/* WARNING: The test using UINT_MAX is not foolproof.  When porting to
 * a new platform, you must verify that this check does the right thing.
 * This should only be an issue if you are on a machine where 'int' 
 * does _not_ use the native word size.
 */

#if ( UINT_MAX == 4294967295U )
#  define MX_WORDSIZE	32
#else
#  define MX_WORDSIZE	64
#endif

/* The non-standard build targets are listed first. */

/*=======================================================================*/

#if 0

/* At present I do not make definitions for the 'least' and 'fast' variants. */

typedef char			int8_t;
typedef short			int16_t;
typedef int			int32_t;
#  if MX_WORDSIZE == 64
typedef long			int64_t;
#  else
typedef long long		int64_t;
#  endif

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
#  if MX_WORDSIZE == 64
typedef unsigned long		uint64_t;
#  else
typedef unsigned long long	uint64_t;
#  endif

/*---*/

typedef int64_t			intmax_t;
typedef uint64_t		uintmax_t;

/*=======================================================================*/

#elif defined( OS_VXWORKS )

#  include <types/vxTypes.h>

#  if MX_WORDSIZE == 64
typedef long			int64_t;
typedef unsigned long		uint64_t;
#  else
typedef long long		int64_t;
typedef unsigned long long	uint64_t;
#  endif

typedef int64_t			intmax_t;
typedef uint64_t		uintmax_t;

/*=======================================================================*/

#else
   /* Most targets should be able to use a vendor provided <stdint.h>. */

#  include <stdint.h>

#endif

/*=======================================================================*/

/* Define limit macros if they have not yet been defined.  We assume that
 * they are not yet defined if INT32_MIN does not exist.
 */

#if !defined(INT32_MIN)

   /* C99 states that these macros should only be defined in C++ if they
    * are specifically requested.
    */

#  if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)

      /* WARNING: The following assumes twos complement integer arithmetic. */

#     if MX_WORDSIZE == 64
#        define __INT64_C(c)	c ## L
#        define __UINT64_C(c)	c ## UL
#     else
#        define __INT64_C(c)	c ## LL
#        define __UINT64_C(c)	c ## ULL
#     endif

#     define INT8_MIN		(-128)
#     define INT16_MIN		(-32768)
#     define INT32_MIN		(-2147483648)
#     define INT64_MIN		(-__INT64_C(9223372036854775807)-1)

#     define INT8_MAX		(127)
#     define INT16_MAX		(32767)
#     define INT32_MAX		(2147483647)
#     define INT64_MAX		(__INT64_C(9223372036854775807))

#     define UINT8_MAX		(255)
#     define UINT16_MAX		(65535)
#     define UINT32_MAX		(4294967295U)
#     define UINT64_MAX		(__UINT64_C(18446744073709551615))

#     if MX_WORDSIZE == 64
#        define INTPTR_MIN	INT64_MIN
#        define INTPTR_MAX	INT64_MAX
#        define UINTPTR_MAX	UINT64_MAX
#     else
#        define INTPTR_MIN	INT32_MIN
#        define INTPTR_MAX	INT32_MAX
#        define UINTPTR_MAX	UINT32_MAX
#     endif

#     define INTMAX_MIN		INT64_MIN
#     define INTMAX_MAX		INT64_MAX
#     define UINTMAX_MAX	UINT64_MAX

#  endif
#endif /* !defined INT32_MIN) */

#endif /* __MX_STDINT_H__ */
