/*
 * Name:    d_databox_mcs.h 
 *
 * Purpose: Header file for Databox multichannel support.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DATABOX_MCS_H__
#define __D_DATABOX_MCS_H__

#include "mx_mcs.h"

/* ===== MX DATABOX mcs record data structures ===== */

#define MX_DATABOX_MCS_BUFFER_LENGTH	100

typedef struct {
	MX_RECORD *databox_record;
	double *motor_position_data;

	long measurement_index;
	int buffer_index;
	char buffer[ MX_DATABOX_MCS_BUFFER_LENGTH + 1 ];

	int buffer_status;
} MX_DATABOX_MCS;

#define MXF_DATABOX_MCS_BUFFER_IS_FILLING	0
#define MXF_DATABOX_MCS_BUFFER_CR_SEEN		1
#define MXF_DATABOX_MCS_BUFFER_COMPLETE		2

/* Define all of the interface functions. */

MX_API mx_status_type mxd_databox_mcs_initialize_type( long type );
MX_API mx_status_type mxd_databox_mcs_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_databox_mcs_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_databox_mcs_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_databox_mcs_print_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_databox_mcs_read_parms_from_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_mcs_write_parms_to_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_databox_mcs_open( MX_RECORD *record );
MX_API mx_status_type mxd_databox_mcs_close( MX_RECORD *record );

MX_API mx_status_type mxd_databox_mcs_start( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_read_timer( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_databox_mcs_set_parameter( MX_MCS *mcs );

MX_API mx_status_type mxd_databox_mcs_start_sequence( MX_RECORD *mcs_record,
						double destination,
						int perform_move );

extern MX_RECORD_FUNCTION_LIST mxd_databox_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_databox_mcs_mcs_function_list;

extern mx_length_type mxd_databox_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_databox_mcs_rfield_def_ptr;

#define MXD_DATABOX_MCS_STANDARD_FIELDS \
  {-1, -1, "databox_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX_MCS, databox_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_position_data", MXFT_DOUBLE, NULL, 1, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX_MCS, motor_position_data), \
	{sizeof(double)}, NULL, MXFF_VARARGS }

#endif /* __D_DATABOX_MCS_H__ */

