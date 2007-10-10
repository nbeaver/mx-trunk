/*
 * Name:    mx_hrt_debug.h
 *
 * Purpose: This header defines macros for debugging timing using the
 *          MX high resolution timing interface.  In principle, these
 *          could be done as inline functions on some platforms.
 *          However, implementing them as macros is more portable and
 *          should still not have a speed penalty.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_HRT_DEBUG_H__
#define __MX_HRT_DEBUG_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_hrt.h"

/* The following definitions are for general timing debugging. */

typedef struct {
	struct timespec start_time;
	struct timespec end_time;
	struct timespec time_difference;
	double time_in_seconds;
} MX_HRT_TIMING;

#define MX_HRT_START( x )						\
	do {								\
		(x).start_time = mx_high_resolution_time();		\
	} while (0)

#define MX_HRT_END( x )							\
	do {								\
		(x).end_time = mx_high_resolution_time();		\
									\
		(x).time_difference = mx_subtract_high_resolution_times(\
					(x).end_time, (x).start_time ); \
									\
		(x).time_in_seconds =				\
		    mx_convert_high_resolution_time_to_seconds(	\
			(x).time_difference );				\
	} while (0)

MX_API void
MX_HRT_RESULTS( MX_HRT_TIMING x,
		const char *fname,
		char *format,
		... ) MX_PRINTFLIKE( 3, 4 );

#define MX_HRT_TIME_BETWEEN_MEASUREMENTS(meas1, meas2, fname)		\
	do {								\
		struct timespec mx_hrt_time_between;			\
		double mx_hrt_time_between_in_seconds;		\
									\
		mx_hrt_time_between = mx_subtract_high_resolution_times(\
			(meas2).start_time, (meas1).end_time );		\
									\
		mx_hrt_time_between_in_seconds =			\
		  mx_convert_high_resolution_time_to_seconds(	\
			mx_hrt_time_between );				\
									\
		MX_DEBUG(-2,						\
		    ("%s: time between measurements = %g usec",		\
		    (fname), 1.0e6 * mx_hrt_time_between_in_seconds));	\
	} while (0)

/* The following definitions are for debugging RS-232 communication timing. */

typedef struct {
	size_t length;

	struct timespec start_time;
	struct timespec end_time;
	struct timespec time_difference;
	double time_in_seconds;
	double chars_per_second;

	int do_busy_wait;
	struct timespec busy_wait_start_time;
	struct timespec busy_wait_time_difference;
	double busy_wait_time_in_seconds;
	unsigned long busy_wait_retries;
} MX_HRT_RS232_TIMING;

#define MX_HRT_RS232_START_COMMAND( x, len )				\
	do {								\
		(x).do_busy_wait = FALSE;				\
		(x).length = (len);					\
		(x).start_time = mx_high_resolution_time();		\
	} while (0)

#define MX_HRT_RS232_END_COMMAND( x )					\
	do {								\
		(x).end_time = mx_high_resolution_time();		\
									\
		(x).time_difference = mx_subtract_high_resolution_times(\
					(x).end_time, (x).start_time ); \
									\
		(x).time_in_seconds =				\
		    mx_convert_high_resolution_time_to_seconds(	\
			(x).time_difference );				\
									\
		(x).chars_per_second = mx_divide_safely(	\
			(double)(x).length, (x).time_in_seconds ); \
	} while (0)

#define MX_HRT_RS232_COMMAND_RESULTS( x, cmd, fname )			\
	MX_DEBUG(-2,("%s: command '%s', time = %g usec, cps = %g",	\
	(fname), (cmd), 1.0e6 * (x).time_in_seconds, (x).chars_per_second))

/***/

#define MX_HRT_RS232_START_RESPONSE( x, rs232_record )			\
	do {								\
		/* Busy/wait if provided an rs232_record pointer. */	\
									\
		if ( (rs232_record) == NULL ) {				\
			(x).do_busy_wait = FALSE;			\
		} else {						\
			(x).do_busy_wait = TRUE;			\
									\
			(x).busy_wait_retries = 0;			\
									\
			(x).busy_wait_start_time =			\
				mx_high_resolution_time();		\
									\
			/* Loop until the first character arrives. */	\
									\
			while (1) {					\
			    unsigned long mx_hrt_num_input_bytes_available; \
			    mx_status_type mx_hrt_status;		\
									\
			    mx_hrt_status =				\
			    	mx_rs232_num_input_bytes_available(	\
				    	(rs232_record), 		\
				    &mx_hrt_num_input_bytes_available );\
			    						\
			    if ( mx_hrt_status.code != MXE_SUCCESS )	\
				    return mx_hrt_status;		\
					    				\
			    if ( mx_hrt_num_input_bytes_available > 0 )	\
				    break; /* Exit the while() loop. */	\
									\
			    (x).busy_wait_retries++;			\
			}						\
		}							\
									\
		(x).start_time = mx_high_resolution_time();		\
	} while (0)

#define MX_HRT_RS232_END_RESPONSE( x, len )				\
	do {								\
		(x).end_time = mx_high_resolution_time();		\
									\
		(x).length = (len);					\
									\
		if ( (x).do_busy_wait ) {				\
		    (x).busy_wait_time_difference =			\
			  mx_subtract_high_resolution_times(		\
			     (x).start_time, (x).busy_wait_start_time );\
									\
		    (x).busy_wait_time_in_seconds =		\
		      mx_convert_high_resolution_time_to_seconds(	\
			  (x).busy_wait_time_difference );		\
		}							\
									\
		(x).time_difference = mx_subtract_high_resolution_times(\
					(x).end_time, (x).start_time ); \
									\
		(x).time_in_seconds =				\
		    mx_convert_high_resolution_time_to_seconds(	\
			(x).time_difference );				\
									\
		(x).chars_per_second = mx_divide_safely(	\
			(double)(x).length, (x).time_in_seconds ); \
	} while (0)

#define MX_HRT_RS232_RESPONSE_RESULTS( x, resp, fname )			\
	if ( (x).do_busy_wait ) {					\
		MX_DEBUG(-2,						\
		("%s: busy/wait time = %g usec, retries = %lu",		\
		  (fname), 1.0e6 * (x).busy_wait_time_in_seconds,	\
		  (x).busy_wait_retries));				\
	}								\
									\
	MX_DEBUG(-2,("%s: response '%s', time = %g usec, cps = %g",	\
	(fname), (resp), 1.0e6 * (x).time_in_seconds, (x).chars_per_second))

#ifdef __cplusplus
}
#endif

#endif /* __MX_HRT_DEBUG_H__ */
