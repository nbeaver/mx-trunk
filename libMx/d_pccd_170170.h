/*
 * Name:    d_pccd_170170.h
 *
 * Purpose: MX driver header for the Aviex PCCD-170170 CCD detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PCCD_170170_H__
#define __D_PCCD_170170_H__

/* Values for 'pccd_170170_flags'. */

#define MXF_PCCD_170170_SUPPRESS_DESCRAMBLING		0x1
#define MXF_PCCD_170170_USE_DETECTOR_HEAD_SIMULATOR	0x2

#define MXF_PCCD_170170_CAMERA_IS_MASTER		0x8
#define MXF_PCCD_170170_USE_TEST_PATTERN		0x10

#define MXF_PCCD_170170_TEST_DEZINGER			0x100

/* Scale factors for converting raw frame dimensions
 * into user frame dimensions.
 */

#define MXF_PCCD_170170_HORIZ_SCALE	4
#define MXF_PCCD_170170_VERT_SCALE	4

typedef struct {
	unsigned long value;
	mx_bool_type read_only;
	mx_bool_type power_of_two;
	unsigned long minimum;
	unsigned long maximum;
} MX_PCCD_170170_REGISTER;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	MX_RECORD *camera_link_record;
	MX_RECORD *internal_trigger_record;
	unsigned long initial_trigger_mode;
	unsigned long pccd_170170_flags;

	long vinput_normal_framesize[2];
	long normal_binsize[2];

	MX_IMAGE_FRAME *raw_frame;

	MX_IMAGE_FRAME *temp_frame;

	long old_framesize[2];

	uint16_t ***sector_array;

	long num_registers;
	MX_PCCD_170170_REGISTER *register_array;

	unsigned long dh_base;

	unsigned long dh_control;
	unsigned long dh_overscanned_pixels_per_line;
	unsigned long dh_physical_lines_in_quadrant;
	unsigned long dh_physical_pixels_in_quadrant;

	unsigned long dh_lines_read_in_quadrant;
	unsigned long dh_pixels_read_in_quadrant;

	unsigned long dh_shutter_delay_time;
	unsigned long dh_exposure_time;
	unsigned long dh_readout_delay_time;
	unsigned long dh_frames_per_sequence;
	unsigned long dh_gap_time;
	unsigned long dh_exposure_multiplier;
	unsigned long dh_gap_multiplier;
	unsigned long dh_controller_fpga_version;

	unsigned long dh_line_binning;
	unsigned long dh_pixel_binning;
	unsigned long dh_subframe_size;
	unsigned long dh_subimages_per_read;
	unsigned long dh_streak_mode_lines;

	unsigned long dh_comm_fpga_version;

	unsigned long dh_offset_a1;
	unsigned long dh_offset_a2;
	unsigned long dh_offset_a3;
	unsigned long dh_offset_a4;
	unsigned long dh_offset_b1;
	unsigned long dh_offset_b2;
	unsigned long dh_offset_b3;
	unsigned long dh_offset_b4;
	unsigned long dh_offset_c1;
	unsigned long dh_offset_c2;
	unsigned long dh_offset_c3;
	unsigned long dh_offset_c4;
	unsigned long dh_offset_d1;
	unsigned long dh_offset_d2;
	unsigned long dh_offset_d3;
	unsigned long dh_offset_d4;
} MX_PCCD_170170;

#define MXLV_PCCD_170170_DH_BASE		100000

#define MXLV_PCCD_170170_DH_CONTROL 		(MXLV_PCCD_170170_DH_BASE + 0)

#define MXLV_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE \
				 		(MXLV_PCCD_170170_DH_BASE + 1)

#define MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT \
				 		(MXLV_PCCD_170170_DH_BASE + 2)

#define MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT \
				 		(MXLV_PCCD_170170_DH_BASE + 3)


#define MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT \
				 		(MXLV_PCCD_170170_DH_BASE + 5)

#define MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT \
				 		(MXLV_PCCD_170170_DH_BASE + 6)


#define MXLV_PCCD_170170_DH_SHUTTER_DELAY_TIME	(MXLV_PCCD_170170_DH_BASE + 9)

#define MXLV_PCCD_170170_DH_EXPOSURE_TIME	(MXLV_PCCD_170170_DH_BASE + 10)

#define MXLV_PCCD_170170_DH_READOUT_DELAY_TIME  (MXLV_PCCD_170170_DH_BASE + 11)

#define MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE (MXLV_PCCD_170170_DH_BASE + 12)

#define MXLV_PCCD_170170_DH_GAP_TIME		(MXLV_PCCD_170170_DH_BASE + 13)

#define MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER	(MXLV_PCCD_170170_DH_BASE + 14)

#define MXLV_PCCD_170170_DH_GAP_MULTIPLIER	(MXLV_PCCD_170170_DH_BASE + 15)

#define MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION \
						(MXLV_PCCD_170170_DH_BASE + 16)


#define MXLV_PCCD_170170_DH_LINE_BINNING	(MXLV_PCCD_170170_DH_BASE + 18)

#define MXLV_PCCD_170170_DH_PIXEL_BINNING	(MXLV_PCCD_170170_DH_BASE + 19)

#define MXLV_PCCD_170170_DH_SUBFRAME_SIZE	(MXLV_PCCD_170170_DH_BASE + 20)

#define MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ	(MXLV_PCCD_170170_DH_BASE + 21)

#define MXLV_PCCD_170170_DH_STREAK_MODE_LINES	(MXLV_PCCD_170170_DH_BASE + 22)

#define MXLV_PCCD_170170_DH_OFFSET_A1		(MXLV_PCCD_170170_DH_BASE + 24)
#define MXLV_PCCD_170170_DH_OFFSET_A2		(MXLV_PCCD_170170_DH_BASE + 25)
#define MXLV_PCCD_170170_DH_OFFSET_A3		(MXLV_PCCD_170170_DH_BASE + 26)
#define MXLV_PCCD_170170_DH_OFFSET_A4		(MXLV_PCCD_170170_DH_BASE + 27)
#define MXLV_PCCD_170170_DH_OFFSET_B1		(MXLV_PCCD_170170_DH_BASE + 28)
#define MXLV_PCCD_170170_DH_OFFSET_B2		(MXLV_PCCD_170170_DH_BASE + 29)
#define MXLV_PCCD_170170_DH_OFFSET_B3		(MXLV_PCCD_170170_DH_BASE + 30)
#define MXLV_PCCD_170170_DH_OFFSET_B4		(MXLV_PCCD_170170_DH_BASE + 31)
#define MXLV_PCCD_170170_DH_OFFSET_C1		(MXLV_PCCD_170170_DH_BASE + 32)
#define MXLV_PCCD_170170_DH_OFFSET_C2		(MXLV_PCCD_170170_DH_BASE + 33)
#define MXLV_PCCD_170170_DH_OFFSET_C3		(MXLV_PCCD_170170_DH_BASE + 34)
#define MXLV_PCCD_170170_DH_OFFSET_C4		(MXLV_PCCD_170170_DH_BASE + 35)
#define MXLV_PCCD_170170_DH_OFFSET_D1		(MXLV_PCCD_170170_DH_BASE + 36)
#define MXLV_PCCD_170170_DH_OFFSET_D2		(MXLV_PCCD_170170_DH_BASE + 37)
#define MXLV_PCCD_170170_DH_OFFSET_D3		(MXLV_PCCD_170170_DH_BASE + 38)
#define MXLV_PCCD_170170_DH_OFFSET_D4		(MXLV_PCCD_170170_DH_BASE + 39)

#define MXLV_PCCD_170170_DH_COMM_FPGA_VERSION	(MXLV_PCCD_170170_DH_BASE + 116)


#define MX_PCCD_170170_NUM_REGISTERS \
	(MXLV_PCCD_170170_DH_COMM_FPGA_VERSION - MXLV_PCCD_170170_DH_BASE + 1)

#define MXD_PCCD_170170_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_link_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, camera_link_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "internal_trigger_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, internal_trigger_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "initial_trigger_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, initial_trigger_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pccd_170170_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, pccd_170170_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  \
  {MXLV_PCCD_170170_DH_BASE, -1, "dh_base", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_base), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_CONTROL, \
		-1, "dh_control", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_control), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE, \
	    -1, "dh_overscanned_pixels_per_line", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_overscanned_pixels_per_line), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT, \
		-1, "dh_physical_lines_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_physical_lines_in_quadrant), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT, \
		-1, "dh_physical_pixels_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_physical_pixels_in_quadrant), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_LINES_READ_IN_QUADRANT, \
		-1, "dh_lines_read_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_lines_read_in_quadrant), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT, \
		-1, "dh_pixels_read_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_pixels_read_in_quadrant), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SHUTTER_DELAY_TIME, \
		-1, "dh_shutter_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_shutter_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_TIME, \
		-1, "dh_exposure_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_READOUT_DELAY_TIME, \
		-1, "dh_readout_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_readout_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE, \
		-1, "dh_frames_per_sequence", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_frames_per_sequence), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_GAP_TIME, \
		-1, "dh_gap_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_gap_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER, \
		-1, "dh_exposure_multiplier", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_multiplier), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_GAP_MULTIPLIER, \
		-1, "dh_gap_multiplier", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_gap_multiplier), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_CONTROLLER_FPGA_VERSION, \
		-1, "dh_controller_fpga_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_controller_fpga_version), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_LINE_BINNING, \
		-1, "dh_line_binning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_line_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_PIXEL_BINNING, \
		-1, "dh_pixel_binning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_pixel_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SUBFRAME_SIZE, \
		-1, "dh_subframe_size", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_subframe_size), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SUBIMAGES_PER_READ, \
		-1, "dh_subimages_per_read", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_subimages_per_read), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_STREAK_MODE_LINES, \
		-1, "dh_streak_mode_lines", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_streak_mode_lines), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A1, \
		-1, "dh_offset_a1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A2, \
		-1, "dh_offset_a2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A3, \
		-1, "dh_offset_a3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a3), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A4, \
		-1, "dh_offset_a4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a4), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B1, \
		-1, "dh_offset_b1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B2, \
		-1, "dh_offset_b2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B3, \
		-1, "dh_offset_b3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b3), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B4, \
		-1, "dh_offset_b4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b4), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C1, \
		-1, "dh_offset_c1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C2, \
		-1, "dh_offset_c2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C3, \
		-1, "dh_offset_c3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c3), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C4, \
		-1, "dh_offset_c4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c4), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D1, \
		-1, "dh_offset_d1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D2, \
		-1, "dh_offset_d2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D3, \
		-1, "dh_offset_d3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d3), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D4, \
		-1, "dh_offset_d4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d4), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_COMM_FPGA_VERSION, \
		-1, "dh_comm_fpga_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_comm_fpga_version), \
	{0}, NULL, 0}

MX_API mx_status_type mxd_pccd_170170_initialize_type( long record_type );
MX_API mx_status_type mxd_pccd_170170_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_open( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_close( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_pccd_170170_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pccd_170170_set_parameter( MX_AREA_DETECTOR *ad );

MX_API mx_status_type mxd_pccd_170170_camera_link_command(
					MX_PCCD_170170 *pccd_170170,
					char *command,
					char *response,
					size_t max_response_length,
					int debug_flag );

MX_API mx_status_type mxd_pccd_170170_read_register(
					MX_PCCD_170170 *pccd_170170,
					unsigned long register_address,
					unsigned long *register_value );

MX_API mx_status_type mxd_pccd_170170_write_register(
					MX_PCCD_170170 *pccd_170170,
					unsigned long register_address,
					unsigned long register_value );

extern MX_RECORD_FUNCTION_LIST mxd_pccd_170170_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_pccd_170170_ad_function_list;

extern long mxd_pccd_170170_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pccd_170170_rfield_def_ptr;

#endif /* __D_PCCD_170170_H__ */

