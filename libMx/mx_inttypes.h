/*
 * Name:     mx_inttypes.h
 *
 * Purpose:  This header file declares printf() and scanf() macros
 *           for processing the data types defined by the C99 standard
 *           in <inttypes.h>.  It also includes "mx_stdint.h".
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2007, 2014-2015, 2021, 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_INTTYPES_H__
#define __MX_INTTYPES_H__

#include "mx_stdint.h"

/* The non-standard build targets are listed first. */

/*=======================================================================*/
#if defined(OS_DJGPP) || defined(OS_RTEMS) || defined(OS_ECOS) \
	|| defined(OS_UNIXWARE) \
	|| ( defined(__OpenBSD__) && ( !defined(PRId8) ) )

#define PRId8	"hhd"
#define PRIu8	"hhu"
#define PRIx8	"hhx"

#define SCNd8	"hhd"
#define SCNu8	"hhu"
#define SCNx8	"hhx"

#define PRId16	"hd"
#define PRIu16	"hu"
#define PRIx16	"hx"

#define SCNd16	"hd"
#define SCNu16	"hu"
#define SCNx16	"hx"

#define PRId32	"d"
#define PRIu32	"u"
#define PRIx32	"x"

#define SCNd32	"d"
#define SCNu32	"u"
#define SCNx32	"x"

#define PRId64	"lld"
#define PRIu64  "llu"
#define PRIx64  "llx"

#define SCNd64	"lld"
#define SCNu64	"llu"
#define SCNx64	"llx"

/*=======================================================================*/
#elif defined(OS_IRIX) || defined(OS_VXWORKS) || defined(OS_VMS)

#define PRId8	"hd"
#define PRIu8	"hu"
#define PRIx8	"hx"

#define SCNd8	"uunsupported"
#define SCNu8	"uunsupported"
#define SCNx8	"uunsupported"

#define PRId16	"hd"
#define PRIu16	"hu"
#define PRIx16	"hx"

#define SCNd16	"hd"
#define SCNu16	"hu"
#define SCNx16	"hx"

#define PRId32	"d"
#define PRIu32	"u"
#define PRIx32	"x"

#define SCNd32	"d"
#define SCNu32	"u"
#define SCNx32	"x"

#define PRId64	"lld"
#define PRIu64  "llu"
#define PRIx64  "llx"

#define SCNd64	"lld"
#define SCNu64	"llu"
#define SCNx64	"llx"

/*=======================================================================*/
#elif defined(OS_TRU64) && \
    ( defined(__GNUC__) || (defined(__DECC_VER) && (__DECC_VER < 60590000)) )

#define PRId8	"hd"
#define PRIu8	"hu"
#define PRIx8	"hx"

#define SCNd8	"uunsupported"
#define SCNu8	"uunsupported"
#define SCNx8	"uunsupported"

#define PRId16	"hd"
#define PRIu16	"hu"
#define PRIx16	"hx"

#define SCNd16	"hd"
#define SCNu16	"hu"
#define SCNx16	"hx"

#define PRId32	"d"
#define PRIu32	"u"
#define PRIx32	"x"

#define SCNd32	"d"
#define SCNu32	"u"
#define SCNx32	"x"

#define PRId64	"ld"
#define PRIu64  "lu"
#define PRIx64  "lx"

#define SCNd64	"ld"
#define SCNu64	"lu"
#define SCNx64	"lx"

/*=======================================================================*/
#elif defined(OS_WIN32) \
	&& ( defined(__BORLANDC__) \
	   || (defined(_MSC_VER) && (_MSC_VER < 1900)) )

#define PRId8	"hd"
#define PRIu8	"hu"
#define PRIx8	"hx"

#define SCNd8	"!unsupported"
#define SCNu8	"!unsupported"
#define SCNx8	"!unsupported"

#define PRId16	"hd"
#define PRIu16	"hu"
#define PRIx16	"hx"

#define SCNd16	"hd"
#define SCNu16	"hu"
#define SCNx16	"hx"

#define PRId32	"d"
#define PRIu32	"u"
#define PRIx32	"x"

#define SCNd32	"d"
#define SCNu32	"u"
#define SCNx32	"x"

#define PRId64	"I64d"
#define PRIu64  "I64u"
#define PRIx64  "I64x"

#define SCNd64	"I64d"
#define SCNu64	"I64u"
#define SCNx64	"I64x"

/*=======================================================================*/

#elif ( defined(MX_GLIBC_VERSION) && (MX_GLIBC_VERSION < 2001000L) )

#define PRId8	"hd"
#define PRIu8	"hu"
#define PRIx8	"hx"

/* Note that the '*' character will cause the compiler to throw away
 * the value produced by the conversion.  The sscanf() function for
 * this ancient version of Glibc does not provide a way of converting
 * an 8-bit value as anything other than a signed or unsigne 'char'.
 */

#define SCNd8	"c"
#define SCNu8	"c"
#define SCNx8	"c"

#define PRId16	"hd"
#define PRIu16	"hu"
#define PRIx16	"hx"

#define SCNd16	"hd"
#define SCNu16	"hu"
#define SCNx16	"hx"

#define PRId32	"d"
#define PRIu32	"u"
#define PRIx32	"x"

#define SCNd32	"d"
#define SCNu32	"u"
#define SCNx32	"x"

#define PRId64	"Ld"
#define PRIu64  "Lu"
#define PRIx64  "Lx"

#define SCNd64	"Ld"
#define SCNu64	"Lu"
#define SCNx64	"Lx"

/*=======================================================================*/
#else
   /* Most build targets should be able to use a vendor provided <inttypes.h>.*/

#  if defined(OS_MACOSX)
     /* MacOS X needs this defined to get correct 8-bit and 64-bit macros */
#     ifndef __STDC_LIBRARY_SUPPORTED__
#        define __STDC_LIBRARY_SUPPORTED__
#     endif
#  endif

#  if defined(__GLIBC__)
     /* Versions of Glibc that support ISO C99 state that you must define
      * the macro __STDC_FORMAT_MACROS in order to enable the macros
      * PRId64, SCNd64, etc.
      */

#     define __STDC_FORMAT_MACROS
#  endif

#  include <inttypes.h>

   /* Some build targets do not define all of the macros. */

#  if ( defined(OS_HPUX) && !defined(__LP64__) )

#     define PRId64	"lld"
#     define PRIu64	"llu"
#     define PRIx64	"llx"

#     define SCNd64	"lld"
#     define SCNu64	"llu"
#     define SCNx64	"llx"

#  endif

#endif

#endif /* __MX_INTTYPES_H__ */

