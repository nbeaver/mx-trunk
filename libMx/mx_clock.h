/*
 * Name:    mx_clock.h
 *
 * Purpose: MX system header to read the current time from a Time-Of-Day clock.
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

#ifndef __MX_CLOCK_H__
#define __MX_CLOCK_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record;

	uint64_t timespec[2];
	double seconds;
} MX_CLOCK;

#define MXLV_CLK_TIMESPEC	8801
#define MXLV_CLK_SECONDS	8802

#define MX_CLOCK_STANDARD_FIELDS \
  {MXLV_CLK_TIMESPEC, -1, "timespec", MXFT_UINT64, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, timespec), \
	{sizeof(uint64_t)}, NULL, (MXFF_READ_ONLY | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_CLK_SECONDS, -1, "seconds", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, seconds), \
	{sizeof(double)}, NULL, MXFF_READ_ONLY }

typedef struct {
	mx_status_type ( *get_timespec )( MX_CLOCK *clock );
} MX_CLOCK_FUNCTION_LIST;

extern MX_RECORD_FUNCTION_LIST mx_clock_record_function_list;

MX_API_PRIVATE mx_status_type mx_clock_get_pointers( MX_RECORD *clock_record,
	MX_CLOCK **clock, MX_CLOCK_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_clock_get_timespec( MX_RECORD *clock_record,
						uint64_t *timespec );

MX_API mx_status_type mx_clock_get_seconds( MX_RECORD *clock_record,
						double *seconds );

#define mx_convert_clock_time_to_seconds( value ) \
	( (double) (value).tv_sec + 1.0e-9 * (double) (value).tv_nsec );

MX_API mx_status_type mx_clock_get_time( struct timespec *timespec );

#ifdef __cplusplus
}
#endif

#endif /* __MX_CLOCK_H__ */

