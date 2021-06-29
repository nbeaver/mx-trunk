/* 
 * Name:    d_dante_mca.h
 *
 * Purpose: MCA driver for the XGLab DANTE multichannel analyzers.
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

#ifndef __D_DANTE_MCA_H__
#define __D_DANTE_MCA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_mca.h"

#define MXU_DANTE_MCA_CHANNEL_NAME_LENGTH	80

#define MXU_DANTE_MCA_INPUT_MODE_NAME_LENGTH	20

#define MXU_DANTE_MCA_MAX_SPECTRUM_BINS		4096

/* The following flags are used by the 'dante_mca_flags' field. */

#define MXF_DANTE_MCA_VALIDATE_CONFIGURATION	0x10

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dante_record;
	char identifier[MXU_DANTE_MCA_CHANNEL_NAME_LENGTH+1];
	unsigned long board_number;
	char input_mode_name[MXU_DANTE_MCA_INPUT_MODE_NAME_LENGTH+1];
	unsigned long dante_mca_flags;

	unsigned long firmware_version;
	unsigned long firmware_type;

	unsigned long total_num_measurements;

	mx_bool_type configure;

	unsigned long fast_filter_thr;
	unsigned long energy_filter_thr;
	unsigned long energy_baseline_thr;
	double max_risetime;
	double gain;
	unsigned long peaking_time;
	unsigned long max_peaking_time;
	unsigned long flat_top;
	unsigned long edge_peaking_time;
	unsigned long edge_flat_top;
	unsigned long reset_recovery_time;
	double zero_peak_freq;
	unsigned long baseline_samples;
	mx_bool_type inverted_input;
	double time_constant;
	unsigned long base_offset;
	unsigned long overflow_recovery;
	unsigned long reset_threshold;
	double tail_coefficient;
	unsigned long other_param;

	/*---*/

	unsigned long max_attempts;
	unsigned long attempt_delay_ms;

	/*---*/

	MX_RECORD *child_mcs_record;
	long mca_record_array_index;

	uint64_t *spectrum_data;

#ifdef __cplusplus
	MX_DANTE_CONFIGURATION *mx_dante_configuration;
#endif
} MX_DANTE_MCA;

#define MXLV_DANTE_MCA_CONFIGURE	23001

#define MXD_DANTE_MCA_STANDARD_FIELDS \
  {-1, -1, "dante_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, dante_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "identifier", MXFT_STRING, NULL, \
	  		1, {MXU_DANTE_MCA_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, identifier ), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "board_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, board_number ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_mode_name", MXFT_STRING, NULL, \
	  		1, {MXU_DANTE_MCA_INPUT_MODE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, input_mode_name ), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "dante_mca_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, dante_mca_flags ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "firmware_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, firmware_version ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "firmware_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, firmware_type ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "total_num_measurements", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, total_num_measurements), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_DANTE_MCA_CONFIGURE, -1, "configure", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, configure ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "fast_filter_thr", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, fast_filter_thr ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "energy_filter_thr", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, energy_filter_thr ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "energy_baseline_thr", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, energy_baseline_thr ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "max_risetime", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, max_risetime ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "gain", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, gain ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "peaking_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, peaking_time ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "max_peaking_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, max_peaking_time ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "flat_top", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, flat_top ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "edge_peaking_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, edge_peaking_time ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "edge_flat_top", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, edge_flat_top ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "reset_recovery_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, reset_recovery_time ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "zero_peak_freq", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, zero_peak_freq ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "baseline_samples", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, baseline_samples ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "inverted_input", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, inverted_input ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "time_constant", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, time_constant ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "base_offset", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, base_offset ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "overflow_recovery", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, overflow_recovery ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "reset_threshold", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, reset_threshold ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "tail_coefficient", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, tail_coefficient ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "other_param", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, other_param ), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "max_attempts", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, max_attempts ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "attempt_delay_ms", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, attempt_delay_ms ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

MX_API mx_status_type mxd_dante_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_dante_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_open( MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_close( MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_finish_delayed_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_dante_mca_arm( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_trigger( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_set_parameter( MX_MCA *mca );

MX_API mx_status_type mxd_dante_mca_validate_configuration(
						MX_DANTE_MCA *dante_mca );

extern MX_RECORD_FUNCTION_LIST mxd_dante_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_dante_mca_mca_function_list;

extern long mxd_dante_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dante_mca_rfield_def_ptr;

MX_API mx_status_type mxd_dante_mca_configure( MX_DANTE_MCA *dante_mca,
						MX_DANTE_MCS *dante_mcs );

#ifdef __cplusplus
}
#endif

#endif /* __D_DANTE_MCA_H__ */
