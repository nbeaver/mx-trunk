/*
 * Name:    mx_timer.h
 *
 * Purpose: MX system header file for timers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004, 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_TIMER_H__
#define __MX_TIMER_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
                            * to this timer.
                            */
	double value;

	mx_bool_type busy;
	mx_bool_type stop;
	mx_bool_type clear;

	long mode;
	double last_measurement_time;
} MX_TIMER;

#define MXLV_TIM_VALUE			7001
#define MXLV_TIM_BUSY			7002
#define MXLV_TIM_STOP			7003
#define MXLV_TIM_CLEAR			7004
#define MXLV_TIM_MODE			7005
#define MXLV_TIM_LAST_MEASUREMENT_TIME	7006

#define MX_TIMER_STANDARD_FIELDS \
  {MXLV_TIM_VALUE, -1, "value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_TIMER, value), \
	{0}, NULL, MXFF_POLL }, \
  \
  {MXLV_TIM_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_TIMER, busy), \
	{0}, NULL, MXFF_POLL }, \
  \
  {MXLV_TIM_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_TIMER, stop), \
	{0}, NULL, 0 }, \
  \
  {MXLV_TIM_CLEAR, -1, "clear", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_TIMER, clear), \
	{0}, NULL, 0 }, \
  \
  {MXLV_TIM_MODE, -1, "mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_TIMER, mode), \
	{0}, NULL, 0 }, \
  \
  {MXLV_TIM_LAST_MEASUREMENT_TIME, -1, "last_measurement_time", \
  						MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_TIMER, last_measurement_time), \
	{0}, NULL, 0 }

typedef struct {
	mx_status_type ( *timer_is_busy )( MX_TIMER *timer );
	mx_status_type ( *start )( MX_TIMER *timer );
	mx_status_type ( *stop )( MX_TIMER *timer );
	mx_status_type ( *clear ) ( MX_TIMER *timer );
	mx_status_type ( *read )( MX_TIMER *timer );
	mx_status_type ( *get_mode ) ( MX_TIMER *timer );
	mx_status_type ( *set_mode ) ( MX_TIMER *timer );
	mx_status_type ( *set_modes_of_associated_counters )
						( MX_TIMER *timer );
	mx_status_type ( *get_last_measurement_time ) ( MX_TIMER *timer );
} MX_TIMER_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_timer_get_pointers( MX_RECORD *timer_record,
	MX_TIMER **timer, MX_TIMER_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API_PRIVATE mx_status_type mx_timer_finish_record_initialization(
						MX_RECORD *record);

MX_API mx_status_type mx_timer_is_busy( MX_RECORD *timer_record,
						mx_bool_type *busy );

MX_API mx_status_type mx_timer_start( MX_RECORD *timer_record, double seconds);

MX_API mx_status_type mx_timer_stop( MX_RECORD *timer_record,
						double *seconds_left );

MX_API mx_status_type mx_timer_clear( MX_RECORD *timer_record );

MX_API mx_status_type mx_timer_read( MX_RECORD *timer_record, double *seconds );

MX_API mx_status_type mx_timer_get_mode( MX_RECORD *timer_record, long *mode );

MX_API mx_status_type mx_timer_set_mode( MX_RECORD *timer_record, long mode );

MX_API mx_status_type mx_timer_set_modes_of_associated_counters(
						MX_RECORD *timer_record );

MX_API mx_status_type mx_timer_get_last_measurement_time(
						MX_RECORD *timer_record,
						double *last_measurement_time );

extern MX_RECORD_FUNCTION_LIST mx_timer_record_function_list;

#ifdef __cplusplus
}
#endif

#endif /* __MX_TIMER_H__ */

