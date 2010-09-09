/*
 * Name:    d_am9513_scaler.h
 *
 * Purpose: Header file for MX scaler driver to operate Am9513 counters
 *          as scalers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AM9513_SCALER_H__
#define __D_AM9513_SCALER_H__

#include "mx_scaler.h"

/* ===== Am9513 scaler data structure ===== */

typedef struct {
	long num_counters;
	MX_INTERFACE *am9513_interface_array;
	long gating_control;
	long count_source;
} MX_AM9513_SCALER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_am9513_scaler_initialize_type( long type );
MX_API mx_status_type mxd_am9513_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_am9513_scaler_finish_record_initialization(
							MX_RECORD *record);
MX_API mx_status_type mxd_am9513_scaler_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_am9513_scaler_open( MX_RECORD *record );
MX_API mx_status_type mxd_am9513_scaler_close( MX_RECORD *record );

MX_API mx_status_type mxd_am9513_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_am9513_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_am9513_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_am9513_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_am9513_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_am9513_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_am9513_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_am9513_scaler_set_parameter( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_am9513_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_am9513_scaler_scaler_function_list;

extern long mxd_am9513_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_am9513_scaler_rfield_def_ptr;

#define MXD_AM9513_SCALER_STANDARD_FIELDS \
  {-1, -1, "num_counters", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_SCALER, num_counters), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "am9513_interface_array", MXFT_INTERFACE, NULL, \
	1, {MXU_VARARGS_LENGTH}, MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AM9513_SCALER, am9513_interface_array), \
	{sizeof(MX_INTERFACE)}, \
		NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)},\
  \
  {-1, -1, "gating_control", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_SCALER, gating_control), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "count_source", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513_SCALER, count_source), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_AM9513_SCALER_H__ */

