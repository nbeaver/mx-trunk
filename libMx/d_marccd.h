/*
 * Name:    d_marccd.h
 *
 * Purpose: MX driver header for MarCCD remote control mode.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MARCCD_H__
#define __D_MARCCD_H__

typedef struct {
	MX_RECORD *record;

	long dummy;
} MX_MARCCD;

#define MXD_MARCCD_STANDARD_FIELDS \
  {-1, -1, "dummy", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARCCD, dummy), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_marccd_initialize_type( long record_type );
MX_API mx_status_type mxd_marccd_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_marccd_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_marccd_open( MX_RECORD *record );
MX_API mx_status_type mxd_marccd_close( MX_RECORD *record );
MX_API mx_status_type mxd_marccd_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_marccd_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_get_extended_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_marccd_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_marccd_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_marccd_ad_function_list;

extern long mxd_marccd_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_marccd_rfield_def_ptr;

#endif /* __D_MARCCD_H__ */

