/*
 * Name:    d_radicon_xineos.h
 *
 * Purpose: MX driver header for the Radicon Xineos series of detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RADICON_XINEOS_H__
#define __D_RADICON_XINEOS_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	unsigned long initial_trigger_mode;
} MX_RADICON_XINEOS;


#define MXD_RADICON_XINEOS_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_XINEOS, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "initial_trigger_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_XINEOS, initial_trigger_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}


MX_API mx_status_type mxd_radicon_xineos_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_radicon_xineos_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_radicon_xineos_open( MX_RECORD *record );

MX_API mx_status_type mxd_radicon_xineos_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_xineos_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_radicon_xineos_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_xineos_ad_function_list;

extern long mxd_radicon_xineos_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_radicon_xineos_rfield_def_ptr;

#endif /* __D_RADICON_XINEOS_H__ */

