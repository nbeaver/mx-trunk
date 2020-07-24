/*
 * Name:   i_dante.h
 *
 * Purpose: Interface driver header for the XGLab DANTE MCA.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DANTE_H__
#define __I_DANTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_DANTE_MAX_VERSION_LENGTH		20

#define MXU_DANTE_MAX_IDENTIFIER_LENGTH		16

#define MXU_DANTE_MAX_CALLBACK_DATA_LENGTH	20

#define MX_DANTE_VERSION( major, minor, update, extra ) \
	( 1000000 * (major) + 10000 * (minor) + 100 * (update) + (extra) )

#define MX_DANTE_MIN_VERSION			MX_DANTE_VERSION(3,4,0,3)

#define MX_DANTE_MAX_VERSION			MX_DANTE_VERSION(3,4,0,3)

/* The following flags are used by the 'dante_flags' field. */

#define MXF_DANTE_SHOW_DEVICES			0x1

/* The following are operating modes for the Dante MCA as reported
 * by the 'dante_mode' field.  At present, only normal and mapping mode
 * are supported.
 */

#define MXF_DANTE_NORMAL_MODE			1
#define MXF_DANTE_WAVEFORM_MODE			2
#define MXF_DANTE_LIST_MODE			3
#define MXF_DANTE_LIST_WAVE_MODE		4
#define MXF_DANTE_MAPPING_MODE			5

/* Define the data structures used by the Dante driver. */

typedef struct {
	MX_RECORD *record;
	unsigned long dante_flags;
	unsigned long max_boards_per_chain;
	unsigned long num_mcas;
	double max_init_delay;
	double max_board_delay;
	char config_filename[MXU_FILENAME_LENGTH+1];

	mx_bool_type load_config_file;
	mx_bool_type save_config_file;
	mx_bool_type configure;

	unsigned long dante_mode;

	char dante_version_string[MXU_DANTE_MAX_VERSION_LENGTH+1];
	unsigned long dante_version;
	unsigned long num_master_devices;

	MX_RECORD **mca_record_array;

	unsigned long *num_boards_for_chain;
	char **board_identifier;
} MX_DANTE;

#ifdef __cplusplus

typedef struct {
	struct configuration configuration;
	unsigned long offset;
	unsigned long timestamp_delay;
	InputMode input_mode;
	GatingMode gating_mode;
} MX_DANTE_CONFIGURATION;

#endif

extern uint32_t mxi_dante_callback_id;
extern uint32_t mxi_dante_callback_data[MXU_DANTE_MAX_CALLBACK_DATA_LENGTH];

extern int mxi_dante_wait_for_answer( uint32_t callback_id );

#define MXLV_DANTE_LOAD_CONFIG_FILE	22001
#define MXLV_DANTE_SAVE_CONFIG_FILE	22002
#define MXLV_DANTE_CONFIGURE		22003

#define MXI_DANTE_STANDARD_FIELDS \
  {-1, -1, "dante_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, dante_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "max_boards_per_chain", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, max_boards_per_chain), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "num_mcas", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, num_mcas), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "max_init_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, max_init_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "max_board_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, max_board_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "config_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, config_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_DANTE_LOAD_CONFIG_FILE, -1, "load_config_file", \
	  		MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, load_config_file), \
	{0}, NULL, 0 }, \
  \
  {MXLV_DANTE_SAVE_CONFIG_FILE, -1, "save_config_file", \
	  		MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, save_config_file), \
	{0}, NULL, 0 }, \
  \
  {MXLV_DANTE_CONFIGURE, -1, "configure", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, configure), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "dante_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, dante_mode), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "dante_version_string", MXFT_STRING, NULL, \
				1, {MXU_DANTE_MAX_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, dante_version_string), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "dante_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, dante_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "num_master_devices", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, num_master_devices), \
	{0}, NULL, 0 }, \

MX_API mx_status_type mxi_dante_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxi_dante_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_dante_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_dante_open( MX_RECORD *record );
MX_API mx_status_type mxi_dante_close( MX_RECORD *record );
MX_API mx_status_type mxi_dante_finish_delayed_initialization(
							MX_RECORD *record);
MX_API mx_status_type mxi_dante_special_processing_setup(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_dante_record_function_list;

extern long mxi_dante_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_dante_rfield_def_ptr;

MX_API mx_status_type mxi_dante_show_parameters( MX_RECORD *record );

MX_API mx_status_type mxi_dante_load_config_file( MX_RECORD *record );

MX_API mx_status_type mxi_dante_save_config_file( MX_RECORD *record );

MX_API mx_status_type mxi_dante_configure( MX_RECORD *record );

#ifdef __cplusplus
MX_API mx_status_type mxi_dante_set_configuration_to_defaults(
			MX_DANTE_CONFIGURATION *mx_dante_configuration );
#endif

#ifdef __cplusplus
}
#endif

/* === Driver specific functions === */

#endif /* __I_DANTE_H__ */
