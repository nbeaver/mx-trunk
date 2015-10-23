/*
 * Name:    d_merlin_medipix.h
 *
 * Purpose: Header file for the Merlin Medipix series of detectors
 *          from Quantum Detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MERLIN_MEDIPIX_H__
#define __D_MERLIN_MEDIPIX_H__

#define MXF_MERLIN_DEBUG_COMMAND_PORT	0x1
#define MXF_MERLIN_DEBUG_DATA_PORT	0x2

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	unsigned long command_port;
	unsigned long data_port;
	unsigned long merlin_flags;

	unsigned long merlin_software_version;

	MX_SOCKET *command_socket;
	MX_SOCKET *data_socket;

	MX_THREAD *monitor_thread;

	unsigned long acquisition_header_length;
	char *acquisition_header;

	unsigned long image_data_length;
	char *image_data;

	/* The following values are managed via MX atomic ops. */

	int32_t total_num_frames_at_start;
	int32_t total_num_frames;
} MX_MERLIN_MEDIPIX;


#define MXD_MERLIN_MEDIPIX_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "command_port", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, command_port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "data_port", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, data_port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "merlin_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, merlin_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "merlin_software_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, merlin_software_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "acquisition_header_length", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MERLIN_MEDIPIX, acquisition_header_length), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "acquisition_header", MXFT_STRING, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, acquisition_header), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }, \
  \
  {-1, -1, "image_data_length", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, image_data_length), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "image_data", MXFT_STRING, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MERLIN_MEDIPIX, image_data), \
	{sizeof(char)}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS) }

MX_API mx_status_type mxd_merlin_medipix_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_merlin_medipix_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_merlin_medipix_open( MX_RECORD *record );

MX_API mx_status_type mxd_merlin_medipix_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_get_extended_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_transfer_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_load_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_save_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_copy_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_set_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_merlin_medipix_measure_correction(
							MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_merlin_medipix_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_merlin_medipix_ad_function_list;

extern long mxd_merlin_medipix_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_merlin_medipix_rfield_def_ptr;

MX_API mx_status_type mxd_merlin_medipix_command( MX_MERLIN_MEDIPIX *pilatus,
					char *command,
					char *response,
					size_t response_buffer_length );

#endif /* __D_MERLIN_MEDIPIX_H__ */

