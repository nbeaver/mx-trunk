/*
 * Name:     mx_stdint.h
 *
 * Purpose:  This header file declares printf() and scanf() macros
 *           for processing the data types defined by the C99 standard
 *           in <inttypes.h>.  It also includes "mx_stdint.h".
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

#ifndef __MX_INTTYPES_H__
#define __MX_INTTYPES_H__

#include "mx_stdint.h"

/* The non-standard build targets are listed first. */

/*=======================================================================*/

#if 0

/*=======================================================================*/

#elif defined(OS_WIN32) && defined(_MSC_VER)

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
#else
   /* Most build targets should be able to use a vendor provided <inttypes.h>.*/

#  include <inttypes.h>

#endif

#endif /* __MX_INTTYPES_H__ */

