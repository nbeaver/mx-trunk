/*
 * Name:    d_radicon_helios.h
 *
 * Purpose: MX header for the Molecular Biology Consortium's NOIR 1 detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_RADICON_HELIOS_H__
#define __D_RADICON_HELIOS_H__

#ifdef __cplusplus

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	MX_RECORD *external_trigger_record;

	mx_bool_type arm_signal_present;
	mx_bool_type acquisition_in_progress;

} MX_RADICON_HELIOS;

#define MXD_RADICON_HELIOS_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RADICON_HELIOS, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "external_trigger_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RADICON_HELIOS, external_trigger_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxd_radicon_helios_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_radicon_helios_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_radicon_helios_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_radicon_helios_open( MX_RECORD *record );

MX_API mx_status_type mxd_radicon_helios_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_radicon_helios_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_radicon_helios_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_radicon_helios_ad_function_list;

extern long mxd_radicon_helios_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_radicon_helios_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_RADICON_HELIOS_H__ */

