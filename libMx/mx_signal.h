/*
 * Name:    mx_signal.h
 *
 * Purpose: Header file for the MX equivalent to <signal.h>.  This set of
 *          functions encourages the use of sigaction() instead of signal().
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SIGNAL_H__
#define __MX_SIGNAL_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* First, we include the operating system's version of <signal.h>. */

#include <signal.h>

/*--------*/

MX_API void mx_standard_signal_error_handler( int signal_number,
						siginfo_t *siginfo,
						void *ucontext );

MX_API void mx_setup_standard_signal_error_handlers( void );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SIGNAL_H__ */

