/*
 * Name:    mx_clock.h
 *
 * Purpose: MX system header to read the current time from a Time-Of-Day clock.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2022-2023 Illinois Institute of Technology
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

#define MXU_CLK_STRING_LENGTH		40

typedef struct {
	MX_RECORD *record;

	uint64_t timespec[2];		/* in struct timespec units */
	uint64_t timespec_offset[2];
	double time;			/* in seconds */
	double time_offset;

	long parameter_type;

	mx_bool_type set_offset_to_current_time;

	char timestamp[MXU_CLK_STRING_LENGTH+1];

	long utc_offset;		/* in seconds */

	char timezone_name[MXU_CLK_STRING_LENGTH+1];
	char epoch_name[MXU_CLK_STRING_LENGTH+1];

	struct timespec timespec_offset_struct;
} MX_CLOCK;

#define MXLV_CLK_TIMESPEC			8801
#define MXLV_CLK_TIMESPEC_OFFSET		8802
#define MXLV_CLK_TIME				8803
#define MXLV_CLK_TIME_OFFSET			8004
#define MXLV_CLK_SET_OFFSET_TO_CURRENT_TIME	8005
#define MXLV_CLK_TIMESTAMP			8006
#define MXLV_CLK_UTC_OFFSET			8007
#define MXLV_CLK_TIMEZONE_NAME			8008
#define MXLV_CLK_EPOCH_NAME			8009

#define MX_CLOCK_STANDARD_FIELDS \
  {MXLV_CLK_TIMESPEC, -1, "timespec", MXFT_UINT64, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, timespec), \
	{sizeof(uint64_t)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_CLK_TIMESPEC_OFFSET, -1, "timespec_offset", MXFT_UINT64, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, timespec_offset), \
	{sizeof(uint64_t)}, NULL, 0 }, \
  \
  {MXLV_CLK_TIME, -1, "time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, time), \
	{sizeof(double)}, NULL, (MXFF_READ_ONLY | MXFF_IN_SUMMARY) }, \
  \
  {MXLV_CLK_TIME_OFFSET, -1, "time_offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, time_offset), \
	{sizeof(double)}, NULL, 0 }, \
  \
  {MXLV_CLK_SET_OFFSET_TO_CURRENT_TIME, -1, "set_offset_to_current_time", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, set_offset_to_current_time), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CLK_TIMESTAMP, -1, "timestamp", MXFT_STRING, \
	  				NULL, 1, {MXU_CLK_STRING_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, timestamp), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_CLK_UTC_OFFSET, -1, "utc_offset", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, utc_offset), \
	{sizeof(double)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_CLK_TIMEZONE_NAME, -1, "timezone_name", MXFT_STRING, \
	  				NULL, 1, {MXU_CLK_STRING_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, timezone_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_CLK_EPOCH_NAME, -1, "epoch_name", MXFT_STRING, \
	  				NULL, 1, {MXU_CLK_STRING_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CLOCK, epoch_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }

typedef struct {
	mx_status_type ( *get_timespec )( MX_CLOCK *mx_clock );
	mx_status_type ( *get_parameter )( MX_CLOCK *mx_clock );
} MX_CLOCK_FUNCTION_LIST;

extern MX_RECORD_FUNCTION_LIST mx_clock_record_function_list;

MX_API_PRIVATE mx_status_type mx_clock_get_pointers( MX_RECORD *mx_clock_record,
	MX_CLOCK **mx_clock, MX_CLOCK_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_clock_finish_record_initialization(
						MX_RECORD *mx_clock_record );

MX_API mx_status_type mx_clock_default_get_parameter_handler(
						MX_CLOCK *mx_clock );

/*----*/

MX_API mx_status_type mx_clock_get_timespec( MX_RECORD *mx_clock_record,
						uint64_t *timespec );

MX_API mx_status_type mx_clock_set_timespec_offset( MX_RECORD *mx_clock_record,
						uint64_t *timespec_offset );

MX_API mx_status_type mx_clock_get_time( MX_RECORD *mx_clock_record,
						double *time_in_seconds );

MX_API mx_status_type mx_clock_set_time_offset( MX_RECORD *mx_clock_record,
						double offset_in_seconds );

MX_API mx_status_type mx_clock_set_offset_to_current_time(
						MX_RECORD *mx_clock_record );

/*----*/

MX_API mx_status_type mx_clock_get_timestamp( MX_RECORD *mx_clock_record,
						char *timestamp,
						size_t timestamp_length );

MX_API mx_status_type mx_clock_get_utc_offset( MX_RECORD *mx_clock_record,
						long *utc_offset );

MX_API mx_status_type mx_clock_get_timezone_name( MX_RECORD *mx_clock_record,
						char *timezone_name,
						size_t timezone_name_length );

MX_API mx_status_type mx_clock_get_epoch_name( MX_RECORD *mx_clock_record,
						char *epoch_name,
						size_t epoch_name_length );

#define mx_convert_clock_time_to_seconds( value ) \
	( (double) (value).tv_sec + 1.0e-9 * (double) (value).tv_nsec );

#ifdef __cplusplus
}
#endif

#endif /* __MX_CLOCK_H__ */

