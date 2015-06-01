/*
 * Name:    d_area_detector_timer.h
 *
 * Purpose: Header file for using an area detector timer as an MX timer device.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AREA_DETECTOR_TIMER_H__
#define __D_AREA_DETECTOR_TIMER_H__

#include "mx_timer.h"

/* ==== Area detector timer data structure ==== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *area_detector_record;
	mx_bool_type ignore_scan_measurement_time;
} MX_AREA_DETECTOR_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_area_detector_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_area_detector_timer_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_area_detector_timer_open( MX_RECORD *record );
MX_API mx_status_type mxd_area_detector_timer_close( MX_RECORD *record );

MX_API mx_status_type mxd_area_detector_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_area_detector_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_area_detector_timer_stop( MX_TIMER *timer );
MX_API mx_status_type mxd_area_detector_timer_read( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_area_detector_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_area_detector_timer_timer_function_list;

extern long mxd_area_detector_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_area_detector_timer_rfield_def_ptr;

#define MXD_AREA_DETECTOR_TIMER_STANDARD_FIELDS \
  {-1, -1, "area_detector_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AREA_DETECTOR_TIMER, area_detector_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "ignore_scan_measurement_time", MXFT_BOOL, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AREA_DETECTOR_TIMER, ignore_scan_measurement_time),\
        {0}, NULL, MXFF_IN_DESCRIPTION } \

#endif /* __D_AREA_DETECTOR_TIMER_H__ */

