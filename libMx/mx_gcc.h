/*
 * Name:    mx_gcc.h
 *
 * Purpose: Definitions used with GCC related software packages.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_GCC_H__
#define __MX_GCC_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)

/* The MX_GCC_VERSION macro encodes the major, minor, and patchlevel
 * version numbers of GCC into a single number that can be used in
 * preprocessor tests.
 *
 * Section 3.7.2 of the GNU C preprocessor manual suggests defining
 * a macro like this.  I have no idea as to why they did not go ahead
 * and do that themselves.
 */

#define MX_GCC_VERSION \
	(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ )

#endif /* __GNUC__ */


#if defined(__GLIBC__)

#define MX_GLIBC_VERSION \
	(__GLIBC__ * 10000 + __GLIBC_MINOR__ * 100 )

#endif /* __GLIBC__ */

#ifdef __cplusplus
}
#endif

#endif /* __MX_GCC_H__ */

