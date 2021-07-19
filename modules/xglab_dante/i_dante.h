/*
 * Name:   i_dante.h
 *
 * Purpose: Interface driver header for the XGLab DANTE MCA.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020-2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DANTE_H__
#define __I_DANTE_H__

#include "mx_pipe.h"
#include "mx_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_DANTE_MAX_VERSION_LENGTH		20

#define MXU_DANTE_MAX_CALLBACK_DATA_LENGTH	20

#define MXU_DANTE_MAX_CHAIN_ID_LENGTH		80

#define MX_DANTE_VERSION( major, minor, update, extra ) \
	( 1000000 * (major) + 10000 * (minor) + 100 * (update) + (extra) )

/* The following flags are used by the 'dante_flags' field. */

#define MXF_DANTE_SHOW_DEVICES			0x1
#define MXF_DANTE_SHOW_DANTE_DLL_FILENAME	0x2

#define MXF_DANTE_SET_BOARDS_TO_0xFF		0x1000

/* The following are for debugging purposes only. */

#define MXF_DANTE_TRACE_CALLS			0x10000
#define MXF_DANTE_TRACE_CALLBACKS		0x20000
#define MXF_DANTE_TRACE_MCA_CALLS		0x40000

#define MXF_DANTE_INTERCEPT_STDOUT		0x100000
#define MXF_DANTE_INTERCEPT_STDERR		0x200000

/* The following are operating modes for the Dante MCA as reported
 * by the 'dante_mode' field.  At present, only normal and mapping mode
 * are supported.
 */

#define MXF_DANTE_NORMAL_MODE			1
#define MXF_DANTE_WAVEFORM_MODE			2
#define MXF_DANTE_LIST_MODE			3
#define MXF_DANTE_LIST_WAVE_MODE		4
#define MXF_DANTE_MAPPING_MODE			5

/* Dante chain master structure. */

typedef struct {
	/* Pointer back to the top level MX_DANTE structure. */
	struct dante_struct *dante;

	char chain_id[MXU_DANTE_MAX_CHAIN_ID_LENGTH+1];
	unsigned long num_boards;
	struct dante_configuration *dante_configuration;

	struct {
		double time;
		unsigned long energy_bins;
	} single_spectrum;

	struct {
		double time;
		unsigned long points;
		unsigned long energy_bins;
	} mapping;

	struct {
		double time;
	} timestamp;
	
	struct {
		unsigned long decimation;

		/* Note: The XML file misspells length as 'lenght'. */
		unsigned long length;
	} wave;
} MX_DANTE_CHAIN_MASTER;

typedef struct {
	struct {
		double time;
		unsigned long energy_bins;
	} single_spectrum;

	struct {
		double time;
		unsigned long points;
		unsigned long energy_bins;
	} mapping;

	struct {
		double time;
	} timestamp;

	struct {
		unsigned long decimation;

		/* Note: The XML file misspells length as 'lenght'. */
		unsigned length;
	} wave;

} MX_DANTE_COMMON_CONFIG;

/* Top level structure for all Dante chains. */

typedef struct dante_struct {
	MX_RECORD *record;
	unsigned long dante_flags;
	unsigned long max_boards_per_chain;
	unsigned long num_mcas;
	double max_init_delay;
	double max_io_delay;
	unsigned long max_io_attempts;
	char config_filename[MXU_FILENAME_LENGTH+1];

	mx_bool_type load_config_file;
	mx_bool_type save_config_file;
	mx_bool_type configure;

	unsigned long dante_mode;

	char dante_version_string[MXU_DANTE_MAX_VERSION_LENGTH+1];
	unsigned long dante_version;
	unsigned long num_master_devices;
	MX_DANTE_CHAIN_MASTER *master;

	MX_DANTE_COMMON_CONFIG common;

	MX_RECORD **mca_record_array;

	mx_bool_type trace_calls;
	mx_bool_type trace_callbacks;
	mx_bool_type trace_mca_calls;

	char dante_dll_filename[MXU_FILENAME_LENGTH+1];

	/* The following are for debugging purposes only. */

#if defined(OS_WIN32)
	void *read_pipe_handle;

	FILE *read_pipe_file;
	FILE *write_output_file;

	MX_THREAD *filter_thread;
#endif

} MX_DANTE;

/* Values for 'configuration_flags'. */

#define MXF_DANTE_CONFIGURATION_DEBUG_PARAMETERS	0x1

#define MXF_DANTE_CONFIGURATION_DEBUG_FILENAMES		0x2

#ifdef __cplusplus

typedef struct dante_configuration {
	MX_DANTE_CHAIN_MASTER *chain;
	MX_RECORD *mca_record;

	struct configuration configuration;
	InputMode input_mode;
	GatingMode gating_mode;
	configuration_offset cfg_offset;
	unsigned long timestamp_delay;
	unsigned long baseline_offset;
	unsigned long calib_energies_bins[2];
	double calib_energies[2];
	unsigned long calib_channels;
	unsigned long calib_equation;

	unsigned long configuration_flags;
} MX_DANTE_CONFIGURATION;

#endif

extern uint32_t mxi_dante_callback_id;
extern uint32_t mxi_dante_callback_data[MXU_DANTE_MAX_CALLBACK_DATA_LENGTH];

extern int mxi_dante_wait_for_answer( uint32_t callback_id, MX_DANTE *dante );

#define MXLV_DANTE_LOAD_CONFIG_FILE	22001
#define MXLV_DANTE_SAVE_CONFIG_FILE	22002
#define MXLV_DANTE_CONFIGURE		22003
#define MXLV_DANTE_TRACE_CALLS		22004
#define MXLV_DANTE_TRACE_CALLBACKS	22005
#define MXLV_DANTE_TRACE_MCA_CALLS	22006

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
  {-1, -1, "max_io_delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, max_io_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "max_io_attempts", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, max_io_attempts), \
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
  \
  {MXLV_DANTE_TRACE_CALLS, -1, "trace_calls", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, trace_calls), \
	{0}, NULL, 0 }, \
  \
  {MXLV_DANTE_TRACE_CALLBACKS, -1, "trace_callbacks", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, trace_callbacks), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "dante_dll_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, dante_dll_filename), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_DANTE_TRACE_MCA_CALLS, -1, "trace_mca_calls", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, trace_mca_calls), \
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

MX_API mx_status_type mxi_dante_show_parameters_for_chain( MX_RECORD *record );

MX_API mx_status_type mxi_dante_load_config_file( MX_RECORD *record );

MX_API mx_status_type mxi_dante_save_config_file( MX_RECORD *record );

MX_API mx_status_type mxi_dante_configure( MX_RECORD *record );

MX_API mx_status_type mxi_dante_set_data_available_flag_for_chain(
						MX_RECORD *record,
						mx_bool_type flag_value );

#ifdef __cplusplus
MX_API mx_status_type mxi_dante_set_configuration_to_defaults(
			MX_DANTE_CONFIGURATION *mx_dante_configuration );
#endif

#ifdef __cplusplus
}
#endif

/* === Driver specific functions === */

#endif /* __I_DANTE_H__ */
