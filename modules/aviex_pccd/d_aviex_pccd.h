/*
 * Name:    d_aviex_pccd.h
 *
 * Purpose: MX driver header for the Aviex PCCD series of CCD detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2011, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AVIEX_PCCD_H__
#define __D_AVIEX_PCCD_H__

#include "d_aviex_pccd_170170.h"
#include "d_aviex_pccd_4824.h"
#include "d_aviex_pccd_16080.h"
#include "d_aviex_pccd_9785.h"

/* Values for 'aviex_pccd_flags'. */

#define MXF_AVIEX_PCCD_SUPPRESS_DESCRAMBLING		0x1
#define MXF_AVIEX_PCCD_USE_DETECTOR_HEAD_SIMULATOR	0x2

#define MXF_AVIEX_PCCD_CAMERA_IS_MASTER			0x8
#define MXF_AVIEX_PCCD_USE_TEST_PATTERN			0x10
#define MXF_AVIEX_PCCD_USE_BIAS_LOOKUP_TABLE		0x20

#define MXF_AVIEX_PCCD_TEST_DEZINGER			0x100
#define MXF_AVIEX_PCCD_SAVE_RAW_FRAME			0x200
#define MXF_AVIEX_PCCD_SKIP_FIRST_CORRECTION_FRAME	0x400

#define MXF_AVIEX_PCCD_USE_INT32_CORRECTION		0x1000
#define MXF_AVIEX_PCCD_USE_DOUBLE_CORRECTION		0x2000

#define MXF_AVIEX_PCCD_ENABLE_MONITOR_CALLBACKS		0x10000
#define MXF_AVIEX_PCCD_ENABLE_SERIAL_DEBUGGING		0x20000

#define MXF_AVIEX_PCCD_ENABLE_MX_AUTOMATIC_OFFSET	0x100000

/*---*/

#define MXF_AVIEX_PCCD_MAXIMUM_DETECTOR_HEAD_FRAMES	255

/* Values for 'aviex_pccd_type'. */

#define MXT_AD_PCCD_16080	16080
#define MXT_AD_PCCD_170170	170170
#define MXT_AD_PCCD_4824	4824
#define MXT_AD_PCCD_9785	9785

/* Values for the 'mx_automatic_offset_edge' flag. */

#define MXF_AUTOMATIC_OFFSET_NONE		0
#define MXF_AUTOMATIC_OFFSET_AT_ROW_EDGE	1
#define MXF_AUTOMATIC_OFFSET_AT_COLUMN_EDGE	2

/*-------------------------------------------------------------*/

typedef struct {
	int size;	/* Register size in bytes, typically 2 for 16-bits. */
	unsigned long value;
	mx_bool_type read_only;
	mx_bool_type write_only;
	mx_bool_type power_of_two;
	unsigned long minimum;
	unsigned long maximum;
	char *name;
} MX_AVIEX_PCCD_REGISTER;

typedef struct {
	MX_AREA_DETECTOR *area_detector;
	struct mx_aviex_pccd *aviex_pccd;

	MX_CALLBACK_MESSAGE *callback_message;
} MX_AVIEX_PCCD_MONITOR_MEASUREMENT;

typedef struct mx_aviex_pccd {
	MX_RECORD *record;
	unsigned long aviex_pccd_type;

	MX_RECORD *video_input_record;
	MX_RECORD *camera_link_record;
	MX_RECORD *camera_head_trigger_record;
	MX_RECORD *external_arm_record;
	unsigned long initial_trigger_mode;
	unsigned long aviex_pccd_flags;
	char geometrical_spline_filename[MXU_FILENAME_LENGTH+1];
	char geometrical_mask_filename[MXU_FILENAME_LENGTH+1];
	char linearity_lookup_table_filename[MXU_FILENAME_LENGTH+1];

	mx_bool_type buffer_overrun;
	mx_bool_type use_top_half_of_detector;

	mx_bool_type first_dh_command;

	mx_bool_type sequence_in_progress;

	MX_AVIEX_PCCD_MONITOR_MEASUREMENT *monitor;
	double monitor_callback_interval;

	mx_bool_type multiframe_needs_extra_frame;

	double exposure_and_gap_step_size;	/* in seconds */

	long vinput_normal_framesize[2];
	long normal_binsize[2];

	MX_IMAGE_FRAME *raw_frame;

	MX_IMAGE_FRAME *temp_frame;

	MX_IMAGE_FRAME *geometrical_mask_frame;
	MX_IMAGE_FRAME *rebinned_geometrical_mask_frame;

	unsigned long num_linearity_lookup_table_entries;
	uint16_t *linearity_lookup_table;

	long old_framesize[2];

	long horiz_descramble_factor;
	long vert_descramble_factor;
	double pixel_clock_frequency;
	unsigned long num_ccd_taps;

	long num_sector_rows;
	long num_sector_columns;
	uint16_t ***sector_array;

	double *edge_average;

	mx_bool_type use_mx_automatic_offset;
	unsigned long mx_automatic_offset_edge;
	unsigned long mx_automatic_offset_edge_size;

	mx_bool_type dh_offsets_writable;

	long num_registers;
	MX_AVIEX_PCCD_REGISTER *register_array;

	union {
		MX_AVIEX_PCCD_170170_DETECTOR_HEAD dh_170170;
		MX_AVIEX_PCCD_4824_DETECTOR_HEAD dh_4824;
		MX_AVIEX_PCCD_16080_DETECTOR_HEAD dh_16080;
		MX_AVIEX_PCCD_9785_DETECTOR_HEAD dh_9785;
	} u;
} MX_AVIEX_PCCD;

/* In the definition of the MXD_AVIEX_PCCD_LOOKUP() macro,
 *
 *   lt = lookup table array
 *   rd = raw frame data array
 *   tap = CCD tap number.
 */

#define MXD_AVIEX_PCCD_LOOKUP( lt, rd, tap ) \
	( (lt)[ (tap) * 65536L + (rd)[(tap)] ] )

/*-------------------------------------------------------------*/

#define MXLV_AVIEX_PCCD_GEOMETRICAL_MASK_FILENAME	50000
#define MXLV_AVIEX_PCCD_MONITOR_CALLBACK_INTERVAL	50001
#define MXLV_AVIEX_PCCD_DH_OFFSETS_WRITABLE		50002

/*----*/

#define MXLV_AVIEX_PCCD_DH_BASE		100000
#define MXLV_AVIEX_PCCD_DH_PSEUDO_BASE	200000

/*----*/

#define MXD_AVIEX_PCCD_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_link_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, camera_link_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_head_trigger_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, camera_head_trigger_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "external_arm_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, external_arm_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "initial_trigger_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, initial_trigger_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "aviex_pccd_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, aviex_pccd_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "geometrical_spline_filename", MXFT_STRING, NULL, \
  			1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, geometrical_spline_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_AVIEX_PCCD_GEOMETRICAL_MASK_FILENAME, \
  		-1, "geometrical_mask_filename", MXFT_STRING, NULL, \
  			1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, geometrical_mask_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "linearity_lookup_table_filename", MXFT_STRING, NULL, \
  			1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, linearity_lookup_table_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  \
  {-1, -1, "buffer_overrun", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, buffer_overrun), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "use_top_half_of_detector", MXFT_BOOL, NULL, 0, {0}, \
      MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, use_top_half_of_detector), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_MONITOR_CALLBACK_INTERVAL, \
		-1, "monitor_callback_interval", MXFT_DOUBLE, NULL, 0, {0}, \
   MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, monitor_callback_interval), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "multiframe_needs_extra_frame", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, multiframe_needs_extra_frame), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "num_sector_rows", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, num_sector_rows), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "num_sector_columns", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, num_sector_columns), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "edge_average", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, edge_average), \
	{0}, NULL, (MXFF_READ_ONLY | MXFF_VARARGS)}, \
  \
  {-1, -1, "use_mx_automatic_offset", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, use_mx_automatic_offset),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "mx_automatic_offset_edge", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, mx_automatic_offset_edge),\
	{0}, NULL, 0}, \
  \
  {-1, -1, "mx_automatic_offset_edge_size", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, mx_automatic_offset_edge_size),\
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_DH_OFFSETS_WRITABLE, -1, \
			"dh_offsets_writable", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, dh_offsets_writable), \
	{0}, NULL, 0}

MX_API mx_status_type mxd_aviex_pccd_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_aviex_pccd_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_aviex_pccd_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_aviex_pccd_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_aviex_pccd_open( MX_RECORD *record );
MX_API mx_status_type mxd_aviex_pccd_close( MX_RECORD *record );
MX_API mx_status_type mxd_aviex_pccd_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_aviex_pccd_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_aviex_pccd_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_set_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_measure_correction( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_aviex_pccd_geometrical_correction(
						MX_AREA_DETECTOR *ad,
						MX_IMAGE_FRAME *frame );

MX_API mx_status_type mxd_aviex_pccd_camera_link_command(
					MX_AVIEX_PCCD *aviex_pccd,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag );

MX_API mx_status_type mxd_aviex_pccd_read_register(
					MX_AVIEX_PCCD *aviex_pccd,
					unsigned long register_address,
					unsigned long *register_value );

MX_API mx_status_type mxd_aviex_pccd_write_register(
					MX_AVIEX_PCCD *aviex_pccd,
					unsigned long register_address,
					unsigned long register_value );

MX_API_PRIVATE mx_status_type mxd_aviex_pccd_init_register(
					MX_AVIEX_PCCD *aviex_pccd,
					long register_index,
					mx_bool_type use_low_byte_name,
					int register_size,
					unsigned long register_value,
					mx_bool_type read_only,
					mx_bool_type write_only,
					mx_bool_type power_of_two,
					unsigned long minimum,
					unsigned long maximum );

MX_API_PRIVATE mx_status_type mxd_aviex_pccd_check_value(
					MX_AVIEX_PCCD *aviex_pccd,
					unsigned long register_address,
					unsigned long register_value,
					MX_AVIEX_PCCD_REGISTER **register_ptr );

/* The following functions are exported for the testing of the
 * descrambling algorithm and should not be used in normal programs.
 */

MX_API_PRIVATE mx_status_type mxd_aviex_pccd_alloc_sector_array(
					uint16_t ****sector_array_ptr,
					long row_framesize,
					long column_framesize,
					long sector_width,
					long sector_height,
					long num_sector_rows,
					long num_sector_columns,
					uint16_t *image_data );

MX_API_PRIVATE void mxd_aviex_pccd_free_sector_array(uint16_t ***sector_array);

/*----*/

extern MX_RECORD_FUNCTION_LIST mxd_aviex_pccd_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_aviex_pccd_ad_function_list;

#endif /* __D_AVIEX_PCCD_H__ */

