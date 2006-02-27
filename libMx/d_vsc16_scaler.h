/*
 * Name:    d_vsc16_scaler.h
 *
 * Purpose: Header file for an MX driver to control a Joerger VSC-16
 *          counter channel as a scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_VSC16_SCALER_H__
#define __D_VSC16_SCALER_H__

typedef struct {
	MX_RECORD *vsc16_record;
	long counter_number;
} MX_VSC16_SCALER;

#define MXD_VSC16_SCALER_STANDARD_FIELDS \
  {-1, -1, "vsc16_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VSC16_SCALER, vsc16_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "counter_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VSC16_SCALER, counter_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_vsc16_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_vsc16_scaler_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_vsc16_scaler_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_vsc16_scaler_open( MX_RECORD *record );

MX_API mx_status_type mxd_vsc16_scaler_clear( MX_SCALER *scaler );
MX_API mx_status_type mxd_vsc16_scaler_overflow_set( MX_SCALER *scaler );
MX_API mx_status_type mxd_vsc16_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_vsc16_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_vsc16_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_vsc16_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_vsc16_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_vsc16_scaler_set_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_vsc16_scaler_set_modes_of_associated_counters(
							MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_vsc16_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_vsc16_scaler_scaler_function_list;

extern long mxd_vsc16_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_vsc16_scaler_rfield_def_ptr;

#endif /* __D_VSC16_SCALER_H__ */

