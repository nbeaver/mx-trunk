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

#define MXF_PCCD_170170_USE_SIMULATOR	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	MX_RECORD *camera_link_record;
	unsigned long initial_trigger_mode;
	unsigned long pccd_170170_flags;

	MX_IMAGE_FRAME *raw_frame;

	long old_framesize[2];

	uint16_t ***image_sector_array;

	unsigned short dh_base;
	unsigned short dh_test_mode;
	unsigned short dh_automatic_offset_correction;
	unsigned short dh_readout_speed;
	unsigned short dh_shutter_delay_time;
	unsigned short dh_exposure_time;
	unsigned short dh_readout_delay;
	unsigned short dh_pause_time;
	unsigned short dh_exposure_source;
	unsigned short dh_exposure_mode;
	unsigned short dh_frames_per_sequence;
	unsigned short dh_exposure_multiplier;
	unsigned short dh_pause_multiplier;
	unsigned short dh_column_binning;
	unsigned short dh_line_binning;
	unsigned short dh_subframe_size;
	unsigned short dh_subframes_stored_on_sensor;
	unsigned short dh_roix1;
	unsigned short dh_roix2;
	unsigned short dh_roiy1;
	unsigned short dh_roiy2;
	unsigned short dh_streak_mode_lines;
	unsigned short dh_offset_a1;
	unsigned short dh_offset_a2;
	unsigned short dh_offset_a3;
	unsigned short dh_offset_a4;
	unsigned short dh_offset_b1;
	unsigned short dh_offset_b2;
	unsigned short dh_offset_b3;
	unsigned short dh_offset_b4;
	unsigned short dh_offset_c1;
	unsigned short dh_offset_c2;
	unsigned short dh_offset_c3;
	unsigned short dh_offset_c4;
	unsigned short dh_offset_d1;
	unsigned short dh_offset_d2;
	unsigned short dh_offset_d3;
	unsigned short dh_offset_d4;
} MX_PCCD_170170;

#define MXF_PCCD_170170_HORIZ_SCALE	4
#define MXF_PCCD_170170_VERT_SCALE	4

#define MXLV_PCCD_170170_DH_BASE			12500
#define MXLV_PCCD_170170_DH_TEST_MODE			12501
#define MXLV_PCCD_170170_DH_AUTOMATIC_OFFSET_CORRECTION	12502
#define MXLV_PCCD_170170_DH_READOUT_SPEED		12503
#define MXLV_PCCD_170170_DH_SHUTTER_DELAY_TIME		12504
#define MXLV_PCCD_170170_DH_EXPOSURE_TIME		12505
#define MXLV_PCCD_170170_DH_READOUT_DELAY		12506
#define MXLV_PCCD_170170_DH_PAUSE_TIME			12507
#define MXLV_PCCD_170170_DH_EXPOSURE_SOURCE		12508
#define MXLV_PCCD_170170_DH_EXPOSURE_MODE		12509
#define MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE		12510
#define MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER		12511
#define MXLV_PCCD_170170_DH_PAUSE_MULTIPLIER		12512
#define MXLV_PCCD_170170_DH_COLUMN_BINNING		12513
#define MXLV_PCCD_170170_DH_LINE_BINNING		12514
#define MXLV_PCCD_170170_DH_SUBFRAME_SIZE		12515
#define MXLV_PCCD_170170_DH_SUBFRAMES_STORED_ON_SENSOR	12516
#define MXLV_PCCD_170170_DH_ROIX1			12517
#define MXLV_PCCD_170170_DH_ROIX2			12518
#define MXLV_PCCD_170170_DH_ROIY1			12519
#define MXLV_PCCD_170170_DH_ROIY2			12520
#define MXLV_PCCD_170170_DH_STREAK_MODE_LINES		12521
#define MXLV_PCCD_170170_DH_OFFSET_A1			12522
#define MXLV_PCCD_170170_DH_OFFSET_A2			12523
#define MXLV_PCCD_170170_DH_OFFSET_A3			12524
#define MXLV_PCCD_170170_DH_OFFSET_A4			12525
#define MXLV_PCCD_170170_DH_OFFSET_B1			12526
#define MXLV_PCCD_170170_DH_OFFSET_B2			12527
#define MXLV_PCCD_170170_DH_OFFSET_B3			12528
#define MXLV_PCCD_170170_DH_OFFSET_B4			12529
#define MXLV_PCCD_170170_DH_OFFSET_C1			12530
#define MXLV_PCCD_170170_DH_OFFSET_C2			12531
#define MXLV_PCCD_170170_DH_OFFSET_C3			12532
#define MXLV_PCCD_170170_DH_OFFSET_C4			12533
#define MXLV_PCCD_170170_DH_OFFSET_D1			12534
#define MXLV_PCCD_170170_DH_OFFSET_D2			12535
#define MXLV_PCCD_170170_DH_OFFSET_D3			12536
#define MXLV_PCCD_170170_DH_OFFSET_D4			12537

#define MXD_PCCD_170170_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_link_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, camera_link_record), \
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
  {MXLV_PCCD_170170_DH_TEST_MODE, \
		-1, "dh_test_mode", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_test_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_AUTOMATIC_OFFSET_CORRECTION, \
		-1, "dh_automatic_offset_correction", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_automatic_offset_correction), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_READOUT_SPEED, \
		-1, "dh_readout_speed", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_readout_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SHUTTER_DELAY_TIME, \
		-1, "dh_shutter_delay_time", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_shutter_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_TIME, \
		-1, "dh_exposure_time", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_READOUT_DELAY, \
		-1, "dh_readout_delay", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_readout_delay), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_PAUSE_TIME, \
		-1, "dh_pause_time", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_pause_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_SOURCE, \
		-1, "dh_exposure_source", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_source), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_MODE, \
		-1, "dh_exposure_mode", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_FRAMES_PER_SEQUENCE, \
		-1, "dh_frames_per_sequence", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_frames_per_sequence), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_EXPOSURE_MULTIPLIER, \
		-1, "dh_exposure_multiplier", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_exposure_multiplier), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_PAUSE_MULTIPLIER, \
		-1, "dh_pause_multiplier", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_pause_multiplier), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_COLUMN_BINNING, \
		-1, "dh_column_binning", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_column_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_LINE_BINNING, \
		-1, "dh_line_binning", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_line_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SUBFRAME_SIZE, \
		-1, "dh_subframe_size", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_subframe_size), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_SUBFRAMES_STORED_ON_SENSOR, \
		-1, "dh_subframes_stored_on_sensor", MXFT_USHORT, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PCCD_170170, dh_subframes_stored_on_sensor), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_ROIX1, -1, "dh_roix1", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_roix1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_ROIX2, -1, "dh_roix2", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_roix2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_ROIY1, -1, "dh_roiy1", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_roiy1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_ROIY2, -1, "dh_roiy2", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_roiy2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_STREAK_MODE_LINES, \
		-1, "dh_streak_mode_lines", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_streak_mode_lines), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A1, \
		-1, "dh_offset_a1", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A2, \
		-1, "dh_offset_a2", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A3, \
		-1, "dh_offset_a3", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a3), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_A4, \
		-1, "dh_offset_a4", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_a4), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B1, \
		-1, "dh_offset_b1", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B2, \
		-1, "dh_offset_b2", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B3, \
		-1, "dh_offset_b3", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b3), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_B4, \
		-1, "dh_offset_b4", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_b4), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C1, \
		-1, "dh_offset_c1", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C2, \
		-1, "dh_offset_c2", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C3, \
		-1, "dh_offset_c3", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c3), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_C4, \
		-1, "dh_offset_c4", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_c4), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D1, \
		-1, "dh_offset_d1", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d1), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D2, \
		-1, "dh_offset_d2", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d2), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D3, \
		-1, "dh_offset_d3", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d3), \
	{0}, NULL, 0}, \
  \
  {MXLV_PCCD_170170_DH_OFFSET_D4, \
		-1, "dh_offset_d4", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCCD_170170, dh_offset_d4), \
	{0}, NULL, 0},

MX_API mx_status_type mxd_pccd_170170_initialize_type( long record_type );
MX_API mx_status_type mxd_pccd_170170_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_open( MX_RECORD *record );
MX_API mx_status_type mxd_pccd_170170_close( MX_RECORD *record );
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
					unsigned short *register_value );

MX_API mx_status_type mxd_pccd_170170_write_register(
					MX_PCCD_170170 *pccd_170170,
					unsigned long register_address,
					unsigned short register_value );

MX_API mx_status_type mxd_pccd_170170_read_adc(
					MX_PCCD_170170 *pccd_170170,
					unsigned long adc_address,
					unsigned short *adc_value );

extern MX_RECORD_FUNCTION_LIST mxd_pccd_170170_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_pccd_170170_ad_function_list;

extern long mxd_pccd_170170_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pccd_170170_rfield_def_ptr;

#endif /* __D_PCCD_170170_H__ */

