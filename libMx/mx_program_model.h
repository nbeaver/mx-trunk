/*
 * Name:    mx_program_model.h
 *
 * Purpose: Describes the native programming model for the current platform.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PROGRAM_MODEL_H__
#define __MX_PROGRAM_MODEL_H__

/* See http://www.unix.org/version2/whatsnew/lp64_wp.html for a discussion
 * of the various programming models.  In 2006, the most common programming
 * models are:
 *
 *     ILP32 for most 32-bit systems
 *     LP64  for most 64-bit Unix-like systems
 *     LLP64 for Win64 systems
 *
 * Of the others, LP32 was the programming model for Win16, while ILP64 is
 * apparently used by 64-bit Cray systems.
 */

#define MX_PROGRAM_MODEL_LP32   0x10  /* int=16, long=32, ptr=32 */

#define MX_PROGRAM_MODEL_ILP32  0x20  /* int=32, long=32, ptr=32 */
#define MX_PROGRAM_MODEL_LLP64  0x24  /* int=32, long=32, ptr=64, long long=64*/

#define MX_PROGRAM_MODEL_LP64   0x40  /* int=32, long=64, ptr=64 */
#define MX_PROGRAM_MODEL_ILP64  0x41  /* int=64, long=64, ptr=64 */

#define MX_PROGRAM_MODEL_UNKNOWN  0x80000000

/*---*/

/* Determine the native programming model. */

/* FIXME: We have to include a real implementation here. */

#include <limits.h>

/* WARNING: The test using UINT_MAX is not foolproof.  When porting to
 * a new platform, you must verify that this check does the right thing.
 * This should only be an issue if you are on a machine where 'int'
 * does _not_ use the native word size.
 */

#if ( UINT_MAX == 4294967295U )
#  define MX_PROGRAM_MODEL    MX_PROGRAM_MODEL_ILP32
#else
#  define MX_PROGRAM_MODEL    MX_PROGRAM_MODEL_LP64
#endif

#define MX_WORDSIZE	( MX_PROGRAM_MODEL & ~0xf )

#endif /* __MX_PROGRAM_MODEL_H__ */
