/*
 * Name:    d_eiger.h
 *
 * Purpose: Header file for the Dectris Eiger series of detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EIGER_H__
#define __D_EIGER_H__

#define MXU_EIGER_SIMPLON_VERSION_LENGTH	20
#define MXU_EIGER_DESCRIPTION_LENGTH		40
#define MXU_EIGER_URL_PREFIX_LENGTH		80

#define MXU_EIGER_KEY_NAME_LENGTH		40
#define MXU_EIGER_KEY_VALUE_LENGTH		80

#define MXU_EIGER_KEY_TYPE_LENGTH		20

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	char simplon_version[MXU_EIGER_SIMPLON_VERSION_LENGTH+1];
	unsigned long eiger_flags;
	MX_RECORD *photon_energy_record;
	char description[MXU_EIGER_DESCRIPTION_LENGTH+1];
	char url_prefix[MXU_EIGER_URL_PREFIX_LENGTH+1];

	char key_name[MXU_EIGER_KEY_NAME_LENGTH+1];
	char key_value[MXU_EIGER_KEY_VALUE_LENGTH+1];

	char key_type[MXU_EIGER_KEY_TYPE_LENGTH+1];

	MX_HTTP *http;
} MX_EIGER;

#define MXLV_EIGER_KEY_VALUE			94001

#define MXD_EIGER_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "simplon_version", MXFT_STRING, \
			NULL, 1, {MXU_EIGER_SIMPLON_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, simplon_version), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "eiger_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, eiger_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "photon_energy_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, photon_energy_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "description", MXFT_STRING, NULL, 1, {MXU_EIGER_DESCRIPTION_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, description), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "url_prefix", MXFT_STRING, NULL, 1, {MXU_EIGER_URL_PREFIX_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, url_prefix), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "key_name", MXFT_STRING, NULL, 1, {MXU_EIGER_KEY_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, key_name), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_EIGER_KEY_VALUE, -1, "key_value", MXFT_STRING, NULL, \
	  				1, {MXU_EIGER_KEY_VALUE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, key_value), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {-1, -1, "key_type", MXFT_STRING, NULL, 1, {MXU_EIGER_KEY_TYPE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EIGER, key_type), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_eiger_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_eiger_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_eiger_open( MX_RECORD *record );
MX_API mx_status_type mxd_eiger_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxd_eiger_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_get_last_and_total_frame_numbers(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_get_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_load_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_save_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_copy_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_set_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_eiger_measure_correction( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_eiger_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_eiger_ad_function_list;

extern long mxd_eiger_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_eiger_rfield_def_ptr;

#endif /* __D_EIGER_H__ */

