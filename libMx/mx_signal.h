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
 * Copyright 2017-2018 Illinois Institute of Technology
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

/* On platforms that do not have a native sigaction() system call,
 * then we must define the necessary data structures ourself.
 */

#if ( defined(OS_WIN32) && defined(_MSC_VER) )

typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned long sigset_t;

union sigval {
	int sival_int;
	void * sival_ptr;
};

typedef struct {
	int si_signo;
	int si_errno;
	int si_code;
	pid_t si_pid;
	uid_t si_uid;
	void *si_addr;
	int si_status;
	long si_band;
	union sigval si_value;
} siginfo_t;

struct sigaction {
	void (*sa_handler)(int);
	void (*sa_sigaction)(int, siginfo_t *, void *);
	sigset_t sa_mask;
	int sa_flags;
};

#define SA_NOCLDSTOP	0x00000001
#define SA_NOCLDWAIT	0x00000002
#define SA_SIGINFO	0x00000004
#define SA_ONSTACK	0x08000000
#define SA_RESTART	0x10000000
#define SA_NODEFER	0x40000000
#define SA_RESETHAND	0x80000000

MX_API int sigaction( int signum,
		const struct sigaction *sa,
		struct sigaction *old_sa );

#endif /* OS_WIN32 */

/*--------*/

MX_API void mx_standard_signal_error_handler( int signal_number );

MX_API void mx_setup_standard_signal_error_handlers( void );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SIGNAL_H__ */

