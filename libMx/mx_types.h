/*
 * Name:     mx_types.h
 *
 * Purpose:  This header file defines data structures that are operating 
 *           system dependent and hardware dependent.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_TYPES_H__
#define __MX_TYPES_H__

/* Create typedefs for quantities that have to be a particular number
 * of bits long.
 */

#if defined(OS_LINUX) || defined(OS_AIX) || defined(OS_SOLARIS) \
 || defined(OS_SUNOS4) || defined(OS_IRIX) || defined(OS_HPUX) \
 || defined(OS_WIN32) || defined(OS_VMS) || defined(OS_DJGPP) \
 || defined(OS_MACOSX) || defined(OS_BSD) || defined(OS_QNX) \
 || defined(OS_CYGWIN) || defined(OS_RTEMS) || defined(OS_VXWORKS)

#define MX_SIZE_CHAR_SHORT_LONG

#else
#error "mx_types.h not yet configured for this OS type."
#endif

/* ---------------------------------------- */

#ifdef MX_SIZE_CHAR_SHORT_LONG

#define __MX_SINT8__	signed char
#define __MX_SINT16__	signed short
#define __MX_SINT32__	signed long
#define __MX_UINT8__	unsigned char
#define __MX_UINT16__	unsigned short
#define __MX_UINT32__	unsigned long

#endif /* MX_SIZE_CHAR_SHORT_LONG */

/* ---------------------------------------- */

typedef __MX_SINT8__	mx_sint8_type;
typedef __MX_SINT16__	mx_sint16_type;
typedef __MX_SINT32__	mx_sint32_type;
typedef __MX_UINT8__	mx_uint8_type;
typedef __MX_UINT16__	mx_uint16_type;
typedef __MX_UINT32__	mx_uint32_type;

#endif /* __MX_TYPES_H__ */
