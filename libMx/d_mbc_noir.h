/*
 * Name:    d_mbc_noir.h
 *
 * Purpose: MX header for the Molecular Biology Consortium's NOIR 1 detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MBC_NOIR_H__
#define __D_MBC_NOIR_H__

typedef struct {
	MX_RECORD *record;

	char epics_prefix[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char remote_filename_prefix[ MXU_FILENAME_LENGTH+1 ];
	char local_filename_prefix[ MXU_FILENAME_LENGTH+1 ];

	MX_EPICS_PV collect_angle_pv;
	MX_EPICS_PV collect_delta_pv;
	MX_EPICS_PV collect_distance_pv;
	MX_EPICS_PV collect_energy_pv;
	MX_EPICS_PV collect_expose_pv;
	MX_EPICS_PV collect_state_msg_pv;
	MX_EPICS_PV noir_command_pv;
	MX_EPICS_PV noir_command_trig_pv;
	MX_EPICS_PV noir_dir_pv;
	MX_EPICS_PV noir_dmd_pv;
	MX_EPICS_PV noir_err_msg_pv;
	MX_EPICS_PV noir_file_pv;
	MX_EPICS_PV noir_process_time_pv;
	MX_EPICS_PV noir_response_pv;
	MX_EPICS_PV noir_response_trig_pv;
	MX_EPICS_PV noir_snap_time_pv;
	MX_EPICS_PV noir_state_msg_pv;
	MX_EPICS_PV noir_store_msg_pv;
	MX_EPICS_PV noir_store_time_pv;

	mx_bool_type acquisition_in_progress;

} MX_MBC_NOIR;

#define MXD_MBC_NOIR_STANDARD_FIELDS \
  {-1, -1, "epics_prefix", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MBC_NOIR, epics_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "remote_filename_prefix", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MBC_NOIR, remote_filename_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "local_filename_prefix", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MBC_NOIR, local_filename_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_mbc_noir_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_mbc_noir_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mbc_noir_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mbc_noir_open( MX_RECORD *record );

MX_API mx_status_type mxd_mbc_noir_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mbc_noir_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mbc_noir_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mbc_noir_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mbc_noir_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mbc_noir_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mbc_noir_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mbc_noir_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mbc_noir_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_mbc_noir_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_mbc_noir_ad_function_list;

extern long mxd_mbc_noir_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mbc_noir_rfield_def_ptr;

#endif /* __D_MBC_NOIR_H__ */

