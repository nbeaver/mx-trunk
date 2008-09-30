/*
 * Name:    d_aviex_pccd_170170.h
 *
 * Purpose: Header file definitions that are specific to the Aviex
 *          PCCD-170170 and PCCD-4824 detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AVIEX_PCCD_170170_H__
#define __D_AVIEX_PCCD_170170_H__

/*-------------------------------------------------------------*/

struct mx_aviex_pccd;

typedef struct {
	unsigned long base;

	unsigned long control;
	unsigned long overscanned_pixels_per_line;
	unsigned long physical_lines_in_quadrant;
	unsigned long physical_pixels_in_quadrant;

	unsigned long lines_read_in_quadrant;
	unsigned long pixels_read_in_quadrant;

	unsigned long initial_delay_time;
	unsigned long exposure_time;
	unsigned long readout_delay_time;
	unsigned long frames_per_sequence;
	unsigned long gap_time;
	unsigned long exposure_multiplier;
	unsigned long gap_multiplier;
	unsigned long controller_fpga_version;

	unsigned long line_binning;
	unsigned long pixel_binning;
	unsigned long subframe_size;
	unsigned long subimages_per_read;
	unsigned long streak_mode_lines;

	unsigned long comm_fpga_version;

	unsigned long offset_a1;
	unsigned long offset_a2;
	unsigned long offset_a3;
	unsigned long offset_a4;
	unsigned long offset_b1;
	unsigned long offset_b2;
	unsigned long offset_b3;
	unsigned long offset_b4;
	unsigned long offset_c1;
	unsigned long offset_c2;
	unsigned long offset_c3;
	unsigned long offset_c4;
	unsigned long offset_d1;
	unsigned long offset_d2;
	unsigned long offset_d3;
	unsigned long offset_d4;

	unsigned long detector_readout_mode;
	unsigned long readout_speed;
	unsigned long test_mode;
	unsigned long offset_correction;
	unsigned long exposure_mode;
	unsigned long linearization;
	unsigned long dummy_frame_valid;
	unsigned long shutter_disable;
	unsigned long over_exposure_warning;
} MX_AVIEX_PCCD_170170_DETECTOR_HEAD;

extern long mxd_aviex_pccd_170170_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aviex_pccd_170170_rfield_def_ptr;

extern long mxd_aviex_pccd_4824_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aviex_pccd_4824_rfield_def_ptr;

/*-------------------------------------------------------------*/

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_descramble_raw_data( uint16_t *,
					uint16_t ***, long, long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_descramble_streak_camera( MX_AREA_DETECTOR *,
						struct mx_aviex_pccd *,
						MX_IMAGE_FRAME *,
						MX_IMAGE_FRAME * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_initialize_detector( MX_RECORD *,
					MX_AREA_DETECTOR *,
					struct mx_aviex_pccd *,
					MX_VIDEO_INPUT * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_get_pseudo_register( struct mx_aviex_pccd *,
					long, unsigned long * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_set_pseudo_register( struct mx_aviex_pccd *,
					long, unsigned long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_set_external_trigger_mode( struct mx_aviex_pccd *,
							mx_bool_type );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_set_binsize( MX_AREA_DETECTOR *, struct mx_aviex_pccd * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_set_sequence_start_delay( struct mx_aviex_pccd *,
							unsigned long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_configure_for_sequence( MX_AREA_DETECTOR *,
					struct mx_aviex_pccd * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_170170_compute_sequence_times( MX_AREA_DETECTOR *,
						struct mx_aviex_pccd * );

/*-------------------------------------------------------------*/

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_4824_descramble_raw_data( uint16_t *,
					uint16_t ***, long, long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_4824_descramble_streak_camera( MX_AREA_DETECTOR *,
						struct mx_aviex_pccd *,
						MX_IMAGE_FRAME *,
						MX_IMAGE_FRAME * );

/*-------------------------------------------------------------*/

/* Control register bit definitions. */

/* Test mode (bit 0) */

#define MXF_AVIEX_PCCD_170170_TEST_MODE_ON			0x1

/* Low noise-high speed (bit 1) */

#define MXF_AVIEX_PCCD_170170_HIGH_SPEED			0x2

/* Automatic offset correction (bit 2) */

#define MXF_AVIEX_PCCD_170170_AUTOMATIC_OFFSET_CORRECTION_ON	0x4

/* Internal/External trigger mode (bit 3) */

#define MXF_AVIEX_PCCD_170170_EXTERNAL_TRIGGER			0x8

/* Edge or duration trigger (bit 4) */

#define MXF_AVIEX_PCCD_170170_EXTERNAL_DURATION_TRIGGER		0x10

/* Detector readout mode (bits 5-6) */

#define MXF_AVIEX_PCCD_170170_DETECTOR_READOUT_MASK		0x60

/*---- Full frame is both bits 0. */

#define MXF_AVIEX_PCCD_170170_SUBIMAGE_MODE			0x20
#define MXF_AVIEX_PCCD_170170_STREAK_CAMERA_MODE		0x60

/* Linearization (bit 7) */

#define MXF_AVIEX_PCCD_170170_LINEARIZATION_ON			0x80

/* Offset correction mode (bit 8) */

#define MXF_AVIEX_PCCD_170170_UNBINNED_PIXEL_AVERAGING		0x100

/* Dummy frame valid pulse (bit 9).
 *
 * The dummy frame valid pulse is to work around a "feature"
 * of the EPIX XCLIB cameras which ignore the first Frame Valid
 * signal sent to them by default.
 */

#define MXF_AVIEX_PCCD_170170_DUMMY_FRAME_VALID			0x200

/* Shutter disable (bit 10). */

#define MXF_AVIEX_PCCD_170170_SHUTTER_DISABLE			0x400

/* Over-exposure warning (bit 11). */

#define MXF_AVIEX_PCCD_170170_OVER_EXPOSURE_WARNING		0x800

/*-------------------------------------------------------------*/

#define MXLV_AVIEX_PCCD_170170_DH_CONTROL	(MXLV_AVIEX_PCCD_DH_BASE + 0)

#define MXLV_AVIEX_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 1)

#define MXLV_AVIEX_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 2)

#define MXLV_AVIEX_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 3)


#define MXLV_AVIEX_PCCD_170170_DH_LINES_READ_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 5)

#define MXLV_AVIEX_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 6)


#define MXLV_AVIEX_PCCD_170170_DH_INITIAL_DELAY_TIME \
						(MXLV_AVIEX_PCCD_DH_BASE + 9)

#define MXLV_AVIEX_PCCD_170170_DH_EXPOSURE_TIME	\
						(MXLV_AVIEX_PCCD_DH_BASE + 10)

#define MXLV_AVIEX_PCCD_170170_DH_READOUT_DELAY_TIME \
						(MXLV_AVIEX_PCCD_DH_BASE + 11)

#define MXLV_AVIEX_PCCD_170170_DH_FRAMES_PER_SEQUENCE \
						(MXLV_AVIEX_PCCD_DH_BASE + 12)

#define MXLV_AVIEX_PCCD_170170_DH_GAP_TIME \
						(MXLV_AVIEX_PCCD_DH_BASE + 13)

#define MXLV_AVIEX_PCCD_170170_DH_EXPOSURE_MULTIPLIER \
						(MXLV_AVIEX_PCCD_DH_BASE + 14)

#define MXLV_AVIEX_PCCD_170170_DH_GAP_MULTIPLIER \
						(MXLV_AVIEX_PCCD_DH_BASE + 15)

#define MXLV_AVIEX_PCCD_170170_DH_CONTROLLER_FPGA_VERSION \
						(MXLV_AVIEX_PCCD_DH_BASE + 16)


#define MXLV_AVIEX_PCCD_170170_DH_LINE_BINNING \
						(MXLV_AVIEX_PCCD_DH_BASE + 18)

#define MXLV_AVIEX_PCCD_170170_DH_PIXEL_BINNING	\
						(MXLV_AVIEX_PCCD_DH_BASE + 19)

#define MXLV_AVIEX_PCCD_170170_DH_SUBFRAME_SIZE	\
						(MXLV_AVIEX_PCCD_DH_BASE + 20)

#define MXLV_AVIEX_PCCD_170170_DH_SUBIMAGES_PER_READ \
						(MXLV_AVIEX_PCCD_DH_BASE + 21)

#define MXLV_AVIEX_PCCD_170170_DH_STREAK_MODE_LINES \
						(MXLV_AVIEX_PCCD_DH_BASE + 22)

#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_A1	(MXLV_AVIEX_PCCD_DH_BASE + 124)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_A2	(MXLV_AVIEX_PCCD_DH_BASE + 125)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_A3	(MXLV_AVIEX_PCCD_DH_BASE + 126)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_A4	(MXLV_AVIEX_PCCD_DH_BASE + 127)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_B1	(MXLV_AVIEX_PCCD_DH_BASE + 128)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_B2	(MXLV_AVIEX_PCCD_DH_BASE + 129)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_B3	(MXLV_AVIEX_PCCD_DH_BASE + 130)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_B4	(MXLV_AVIEX_PCCD_DH_BASE + 131)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_C1	(MXLV_AVIEX_PCCD_DH_BASE + 132)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_C2	(MXLV_AVIEX_PCCD_DH_BASE + 133)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_C3	(MXLV_AVIEX_PCCD_DH_BASE + 134)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_C4	(MXLV_AVIEX_PCCD_DH_BASE + 135)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_D1	(MXLV_AVIEX_PCCD_DH_BASE + 136)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_D2	(MXLV_AVIEX_PCCD_DH_BASE + 137)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_D3	(MXLV_AVIEX_PCCD_DH_BASE + 138)
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_D4	(MXLV_AVIEX_PCCD_DH_BASE + 139)

#define MXLV_AVIEX_PCCD_170170_DH_COMM_FPGA_VERSION \
						(MXLV_AVIEX_PCCD_DH_BASE + 116)


#define MX_AVIEX_PCCD_170170_NUM_REGISTERS \
	(MXLV_AVIEX_PCCD_170170_DH_OFFSET_D4 - MXLV_AVIEX_PCCD_DH_BASE + 1)

/* Define some pseudo registers to manipulate individual bits
 * in the control register.
 */

#define MXLV_AVIEX_PCCD_170170_DH_DETECTOR_READOUT_MODE	200000
#define MXLV_AVIEX_PCCD_170170_DH_READOUT_SPEED		200001
#define MXLV_AVIEX_PCCD_170170_DH_TEST_MODE		200002
#define MXLV_AVIEX_PCCD_170170_DH_OFFSET_CORRECTION	200003
#define MXLV_AVIEX_PCCD_170170_DH_EXPOSURE_MODE		200004
#define MXLV_AVIEX_PCCD_170170_DH_LINEARIZATION		200005
#define MXLV_AVIEX_PCCD_170170_DH_DUMMY_FRAME_VALID	200006
#define MXLV_AVIEX_PCCD_170170_DH_SHUTTER_DISABLE	200007
#define MXLV_AVIEX_PCCD_170170_DH_OVER_EXPOSURE_WARNING	200008

#define MXD_AVIEX_PCCD_170170_STANDARD_FIELDS \
  {-1, -1, "dh_base", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_170170.base), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_CONTROL, \
		-1, "dh_control", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_170170.control), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OVERSCANNED_PIXELS_PER_LINE, \
	    -1, "dh_overscanned_pixels_per_line", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_AVIEX_PCCD, u.dh_170170.overscanned_pixels_per_line), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_PHYSICAL_LINES_IN_QUADRANT, \
		-1, "dh_physical_lines_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_AVIEX_PCCD, u.dh_170170.physical_lines_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_PHYSICAL_PIXELS_IN_QUADRANT, \
		-1, "dh_physical_pixels_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_AVIEX_PCCD, u.dh_170170.physical_pixels_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_LINES_READ_IN_QUADRANT, \
		-1, "dh_lines_read_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.lines_read_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_PIXELS_READ_IN_QUADRANT, \
		-1, "dh_pixels_read_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.pixels_read_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_INITIAL_DELAY_TIME, \
		-1, "dh_initial_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.initial_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_EXPOSURE_TIME, \
		-1, "dh_exposure_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.exposure_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_READOUT_DELAY_TIME, \
		-1, "dh_readout_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.readout_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_FRAMES_PER_SEQUENCE, \
		-1, "dh_frames_per_sequence", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.frames_per_sequence), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_GAP_TIME, \
		-1, "dh_gap_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.gap_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_EXPOSURE_MULTIPLIER, \
		-1, "dh_exposure_multiplier", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.exposure_multiplier), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_GAP_MULTIPLIER, \
		-1, "dh_gap_multiplier", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.gap_multiplier), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_CONTROLLER_FPGA_VERSION, \
		-1, "dh_controller_fpga_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.controller_fpga_version), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_LINE_BINNING, \
		-1, "dh_line_binning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.line_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_PIXEL_BINNING, \
		-1, "dh_pixel_binning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.pixel_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_SUBFRAME_SIZE, \
		-1, "dh_subframe_size", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.subframe_size), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_SUBIMAGES_PER_READ, \
		-1, "dh_subimages_per_read", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.subimages_per_read), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_STREAK_MODE_LINES, \
		-1, "dh_streak_mode_lines", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.streak_mode_lines), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_A1, \
		-1, "dh_offset_a1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_a1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_A2, \
		-1, "dh_offset_a2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_a2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_A3, \
		-1, "dh_offset_a3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_a3), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_A4, \
		-1, "dh_offset_a4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_a4), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_B1, \
		-1, "dh_offset_b1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_b1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_B2, \
		-1, "dh_offset_b2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_b2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_B3, \
		-1, "dh_offset_b3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_b3), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_B4, \
		-1, "dh_offset_b4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_b4), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_C1, \
		-1, "dh_offset_c1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_c1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_C2, \
		-1, "dh_offset_c2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_c2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_C3, \
		-1, "dh_offset_c3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_c3), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_C4, \
		-1, "dh_offset_c4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_c4), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_D1, \
		-1, "dh_offset_d1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_d1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_D2, \
		-1, "dh_offset_d2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_d2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_D3, \
		-1, "dh_offset_d3", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_d3), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_D4, \
		-1, "dh_offset_d4", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_d4), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_COMM_FPGA_VERSION, \
		-1, "dh_comm_fpga_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.comm_fpga_version), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  \
  {MXLV_AVIEX_PCCD_170170_DH_DETECTOR_READOUT_MODE, \
  		-1, "dh_detector_readout_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.detector_readout_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_READOUT_SPEED, \
  		-1, "dh_readout_speed", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.readout_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_TEST_MODE, \
  		-1, "dh_test_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.test_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OFFSET_CORRECTION, \
  		-1, "dh_offset_correction", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.offset_correction), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_EXPOSURE_MODE, \
  		-1, "dh_exposure_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.exposure_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_LINEARIZATION, \
  		-1, "dh_linearization", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.linearization), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_DUMMY_FRAME_VALID, \
  		-1, "dh_dummy_frame_valid", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.dummy_frame_valid), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_SHUTTER_DISABLE, \
  		-1, "dh_shutter_disable", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_170170.shutter_disable), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_170170_DH_OVER_EXPOSURE_WARNING, \
  		-1, "dh_over_exposure_warning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_170170.over_exposure_warning), \
	{0}, NULL, 0}

#endif /* __D_AVIEX_PCCD_170170_H__ */

