/*
 * Name:    mx_hrt.h
 *
 * Purpose: This header defines functions that manipulate times shorter
 *          than the scheduling interval for the operating system.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2003-2005, 2007, 2009, 2021-2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_HRT_H__
#define __MX_HRT_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MX_HRT_ZERO	{{0},{0}}

/* mx_high_resolution_time_init() initializes internal data structures
 * that are used by mx_high_resolution_time() and mx_udelay() below.
 */

MX_API void mx_high_resolution_time_init( void );

/* mx_high_resolution_time() returns the current time in a 'struct timespec'
 * with nanosecond resolution using the highest internal resolution timer
 * available.  On x86 Linux, this uses the RDTSC instruction (where available),
 * while on Win32 this uses QueryPerformanceCounter().  On platforms without
 * high resolution timers, this function falls back to using scheduler
 * clock ticks.
 */

MX_API struct timespec mx_high_resolution_time( void );

/*---*/

MX_API double mx_get_hrt_counter_ticks_per_second( void );

/* mx_high_resolution_time_as_double() returns the current time as
 * a double in units of seconds.
 */

MX_API double mx_high_resolution_time_as_double( void );

/* mx_udelay() attempts to wait for the specified number of microseconds
 * _without_ explicitly giving control back to the operating systems like
 * the sleep functions do.  If this is not possible, it falls back to 
 * using mx_usleep().
 */

MX_API void mx_udelay( unsigned long microseconds );

/*---*/

/* The following functions are not meant to be invoked from MX 
 * application programs.
 */

MX_API_PRIVATE int mx_high_resolution_time_has_hardware_clock( void );

MX_API_PRIVATE struct timespec mx_high_resolution_time_using_clock_ticks(void);

MX_API_PRIVATE double mx_high_resolution_time_init_from_file( void );

#ifdef __cplusplus
}
#endif

#endif /* __MX_HRT_H__ */
