/*
 * Name:    d_bluice_timer.h
 *
 * Purpose: Header file for Blu-Ice controlled timers.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BLUICE_TIMER_H__
#define __D_BLUICE_TIMER_H__

#include "mx_timer.h"

/* ==== Blu-Ice timer data structure ==== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
	char bluice_name[MXU_BLUICE_NAME_LENGTH+1];
	char dhs_server_name[MXU_BLUICE_NAME_LENGTH+1];

	long num_ion_chambers;
	MX_BLUICE_FOREIGN_ION_CHAMBER **foreign_ion_chamber_array;

	int measurement_in_progress;
} MX_BLUICE_TIMER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_bluice_timer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bluice_timer_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_bluice_timer_finish_delayed_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_bluice_timer_is_busy( MX_TIMER *timer );
MX_API mx_status_type mxd_bluice_timer_start( MX_TIMER *timer );
MX_API mx_status_type mxd_bluice_timer_stop( MX_TIMER *timer );

extern MX_RECORD_FUNCTION_LIST mxd_bluice_timer_record_function_list;
extern MX_TIMER_FUNCTION_LIST mxd_bluice_timer_timer_function_list;

extern long mxd_bluice_timer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bluice_timer_rfield_def_ptr;

#define MXD_BLUICE_TIMER_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_BLUICE_TIMER, bluice_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_TIMER, bluice_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "dhs_server_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_TIMER, dhs_server_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }



#endif /* __D_BLUICE_TIMER_H__ */

