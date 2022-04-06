/*
 * Name:    d_os_clock.h
 *
 * Purpose: Header file for MX driver to read from OS-based time-of-day clocks.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_OS_CLOCK_H__
#define __D_OS_CLOCK_H__

#include "mx_clock.h"

/* ==== OS clock data structure ==== */

typedef struct {
	int dummy;
} MX_OS_CLOCK;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_os_clock_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_os_clock_get_timespec( MX_CLOCK *clock );

extern MX_RECORD_FUNCTION_LIST mxd_os_clock_record_function_list;
extern MX_CLOCK_FUNCTION_LIST mxd_os_clock_clock_function_list;

extern long mxd_os_clock_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_os_clock_rfield_def_ptr;

#endif /* __D_OS_CLOCK_H__ */
