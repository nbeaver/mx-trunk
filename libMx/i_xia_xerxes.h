/*
 * Name:   i_xia_xerxes.h
 *
 * Purpose: Header for MX driver for a generic interface to the X-Ray 
 *          Instrumentation Associates XerXes library used by 
 *          the DXP-2X multichannel analyzer.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_XIA_XERXES_H__
#define __I_XIA_XERXES_H__

/* Define the data structures used by the XIA XerXes driver. */

#define MXU_XIA_XERXES_MODULE_TYPE_LENGTH	20

#define MXU_XIA_XERXES_MAX_MODULES		4

#if ( HAVE_XIA_HANDEL && IS_MX_DRIVER )

typedef struct {
	MX_RECORD *record;

	char module_type[ MXU_XIA_XERXES_MODULE_TYPE_LENGTH + 1 ];
	char xiasystems_cfg[ MXU_FILENAME_LENGTH + 1 ];
	char config_filename[ MXU_FILENAME_LENGTH + 1 ];

	unsigned long num_mcas;
	MX_RECORD **mca_record_array;
	int *detector_channel_array;

	double last_measurement_interval;
} MX_XIA_XERXES;

#endif /* HAVE_XIA_HANDEL && IS_MX_DRIVER */

/* The following flags are used by the "PRESET" MCA parameter. */

#define MXF_XIA_PRESET_NONE		0
#define MXF_XIA_PRESET_REAL_TIME	1
#define MXF_XIA_PRESET_LIVE_TIME	2
#define MXF_XIA_PRESET_OUTPUT_EVENTS	3
#define MXF_XIA_PRESET_INPUT_COUNTS	4

/* Label values for the xia_xerxes driver. */

#define MXLV_XIA_XERXES_CONFIG_FILENAME		2001

#define MXI_XIA_XERXES_STANDARD_FIELDS \
  {-1, -1, "module_type", MXFT_STRING, NULL, \
			1, {MXU_XIA_XERXES_MODULE_TYPE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_XERXES, module_type), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "xiasystems_cfg", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_XERXES, xiasystems_cfg), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_XIA_XERXES_CONFIG_FILENAME, -1, "config_filename", \
				MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_XERXES, config_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_mcas", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_XIA_XERXES, num_mcas ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "last_measurement_interval", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_XIA_XERXES, last_measurement_interval),\
	{0}, NULL, 0 }

MX_API mx_status_type mxi_xia_xerxes_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_xia_xerxes_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_xia_xerxes_open( MX_RECORD *record );
MX_API mx_status_type mxi_xia_xerxes_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxi_xia_xerxes_special_processing_setup(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_xia_xerxes_record_function_list;

extern long mxi_xia_xerxes_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_xia_xerxes_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_xia_xerxes_restore_configuration( MX_RECORD *record,
					char *configuration_filename,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_is_busy( MX_MCA *mca,
					mx_bool_type *busy_flag,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_read_parameter( MX_MCA *mca,
					char *parameter_name,
					unsigned long *value_ptr,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_write_parameter( MX_MCA *mca,
					char *parameter_name,
					unsigned long value,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_write_parameter_to_all_channels(
					MX_MCA *mca,
					char *parameter_name,
					unsigned long value,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_start_run( MX_MCA *mca,
					mx_bool_type clear_flag,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_stop_run( MX_MCA *mca,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_read_spectrum( MX_MCA *mca,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_read_statistics( MX_MCA *mca,
					int debug_flag );

MX_API mx_status_type mxi_xia_xerxes_get_baseline_array( MX_MCA *mca,
					int debug_flag );

/*------*/

MX_API const char *mxi_xia_xerxes_strerror( int xia_status_code );

#endif /* __I_XIA_XERXES_H__ */
