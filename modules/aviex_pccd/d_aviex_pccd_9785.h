/*
 * Name:    d_aviex_pccd_9785.h
 *
 * Purpose: Header file definitions that are specific to the Aviex
 *          PCCD-9785 detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2008, 2011, 2015, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AVIEX_PCCD_9785_H__
#define __D_AVIEX_PCCD_9785_H__

/*-------------------------------------------------------------*/

struct mx_aviex_pccd;

typedef struct {
	unsigned long base;

	unsigned long control;
	unsigned long physical_lines_in_quadrant;
	unsigned long physical_pixels_per_line_in_quadrant;

	unsigned long lines_read_in_quadrant;
	unsigned long pixels_read_in_quadrant;

	unsigned long exposure_time;
	unsigned long readout_delay_time;
	unsigned long frames_per_sequence;
	unsigned long gap_time;
	unsigned long controller_fpga_version;

	unsigned long line_binning;
	unsigned long pixel_binning;
	unsigned long subframe_size;
	unsigned long subimages_per_read;
	unsigned long streak_mode_lines;

	unsigned long offset_w1;
	unsigned long offset_x1;
	unsigned long offset_y1;
	unsigned long offset_z1;
	unsigned long offset_w2;
	unsigned long offset_x2;
	unsigned long offset_y2;
	unsigned long offset_z2;

	unsigned long detector_readout_mode;
	unsigned long readout_speed;
	unsigned long test_mode;
	unsigned long offset_correction;
	unsigned long exposure_mode;
	unsigned long linearization;
	unsigned long horizontal_test_pattern;
	unsigned long shutter_output_disabled;
} MX_AVIEX_PCCD_9785_DETECTOR_HEAD;

extern long mxd_aviex_pccd_9785_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aviex_pccd_9785_rfield_def_ptr;

/*-------------------------------------------------------------*/

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_initialize_detector( MX_RECORD *,
					MX_AREA_DETECTOR *,
					struct mx_aviex_pccd *,
					MX_VIDEO_INPUT * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_get_pseudo_register( struct mx_aviex_pccd *,
					long, unsigned long * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_set_pseudo_register( struct mx_aviex_pccd *,
					long, unsigned long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_set_trigger_mode( struct mx_aviex_pccd *aviex_pccd,
                                        mx_bool_type external_trigger,
					mx_bool_type edge_triggered );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_set_binsize( MX_AREA_DETECTOR *, struct mx_aviex_pccd * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_descramble( uint16_t *, uint16_t ***, long, long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_linearity_descramble( uint16_t *,
					uint16_t ***, long, long, uint16_t * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_streak_camera_descramble( MX_AREA_DETECTOR *,
						struct mx_aviex_pccd *,
						MX_IMAGE_FRAME *,
						MX_IMAGE_FRAME * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_set_sequence_start_delay( struct mx_aviex_pccd *,
							unsigned long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_configure_for_sequence( MX_AREA_DETECTOR *,
					struct mx_aviex_pccd * );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_9785_offsets_writable( MX_AREA_DETECTOR *,
					struct mx_aviex_pccd * );

/*-------------------------------------------------------------*/

/* Control register bit definitions. */

/* Test mode uses bits 0 and 9. */

#define MXF_AVIEX_PCCD_9785_TEST_MODE_MASK			0x201

/* Test mode (bit 0) */

#define MXF_AVIEX_PCCD_9785_TEST_MODE_ON			0x1

/* Low noise-high speed (bit 1) */

#define MXF_AVIEX_PCCD_9785_HIGH_SPEED				0x2

/* Automatic offset correction (bit 2) */

#define MXF_AVIEX_PCCD_9785_AUTOMATIC_OFFSET_CORRECTION_ON	0x4

/* Internal/External trigger mode (bit 3) */

#define MXF_AVIEX_PCCD_9785_EXTERNAL_TRIGGER			0x8

/* Edge or duration trigger (bit 4) */

#define MXF_AVIEX_PCCD_9785_EXTERNAL_DURATION_TRIGGER		0x10

/* Detector readout mode (bits 5-6) */

#define MXF_AVIEX_PCCD_9785_DETECTOR_READOUT_MASK		0x60

/*---- Full frame is both bits 0. */

#define MXF_AVIEX_PCCD_9785_SUBIMAGE_MODE			0x20
#define MXF_AVIEX_PCCD_9785_STREAK_CAMERA_MODE			0x60

/* Linearization (bit 7) */

#define MXF_AVIEX_PCCD_9785_LINEARIZATION_ON			0x80

/* Offset correction mode (bit 8) */

#define MXF_AVIEX_PCCD_9785_CCD_POWER_OFF			0x100

/* Horizontal test pattern (bit 9).
 *
 * If test mode is turned on by bit 0, then if bit 9
 * is set, the detector head will generate a horizontal
 * test pattern instead of a vertical one.
 */

#define MXF_AVIEX_PCCD_9785_HORIZONTAL_TEST_PATTERN		0x200

/* Dark image mode (bit 10) */

#define MXF_AVIEX_PCCD_9785_SHUTTER_OUTPUT_DISABLED		0x400

/*-------------------------------------------------------------*/

#define MXLV_AVIEX_PCCD_9785_DH_CONTROL	(MXLV_AVIEX_PCCD_DH_BASE + 0)

#define MXLV_AVIEX_PCCD_9785_DH_PHYSICAL_LINES_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 2)

#define MXLV_AVIEX_PCCD_9785_DH_PHYSICAL_PIXELS_PER_LINE_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 3)


#define MXLV_AVIEX_PCCD_9785_DH_LINES_READ_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 5)

#define MXLV_AVIEX_PCCD_9785_DH_PIXELS_READ_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 6)


#define MXLV_AVIEX_PCCD_9785_DH_SHUTTER_DELAY_TIME \
						(MXLV_AVIEX_PCCD_DH_BASE + 9)

#define MXLV_AVIEX_PCCD_9785_DH_EXPOSURE_TIME	\
						(MXLV_AVIEX_PCCD_DH_BASE + 10)

#define MXLV_AVIEX_PCCD_9785_DH_READOUT_DELAY_TIME \
						(MXLV_AVIEX_PCCD_DH_BASE + 11)

#define MXLV_AVIEX_PCCD_9785_DH_FRAMES_PER_SEQUENCE \
						(MXLV_AVIEX_PCCD_DH_BASE + 12)

#define MXLV_AVIEX_PCCD_9785_DH_GAP_TIME \
						(MXLV_AVIEX_PCCD_DH_BASE + 13)


#define MXLV_AVIEX_PCCD_9785_DH_CONTROLLER_FPGA_VERSION \
						(MXLV_AVIEX_PCCD_DH_BASE + 16)


#define MXLV_AVIEX_PCCD_9785_DH_LINE_BINNING \
						(MXLV_AVIEX_PCCD_DH_BASE + 18)

#define MXLV_AVIEX_PCCD_9785_DH_PIXEL_BINNING	\
						(MXLV_AVIEX_PCCD_DH_BASE + 19)

#define MXLV_AVIEX_PCCD_9785_DH_SUBFRAME_SIZE	\
						(MXLV_AVIEX_PCCD_DH_BASE + 20)

#define MXLV_AVIEX_PCCD_9785_DH_SUBIMAGES_PER_READ \
						(MXLV_AVIEX_PCCD_DH_BASE + 21)

#define MXLV_AVIEX_PCCD_9785_DH_STREAK_MODE_LINES \
						(MXLV_AVIEX_PCCD_DH_BASE + 22)

#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_W1	(MXLV_AVIEX_PCCD_DH_BASE + 24)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_X1	(MXLV_AVIEX_PCCD_DH_BASE + 25)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_Y1	(MXLV_AVIEX_PCCD_DH_BASE + 26)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_Z1	(MXLV_AVIEX_PCCD_DH_BASE + 27)

#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_W2	(MXLV_AVIEX_PCCD_DH_BASE + 28)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_X2	(MXLV_AVIEX_PCCD_DH_BASE + 29)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_Y2	(MXLV_AVIEX_PCCD_DH_BASE + 30)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_Z2	(MXLV_AVIEX_PCCD_DH_BASE + 31)


/* Define some pseudo registers to manipulate individual bits
 * in the control register.
 */

#define MXLV_AVIEX_PCCD_9785_DH_DETECTOR_READOUT_MODE	200000
#define MXLV_AVIEX_PCCD_9785_DH_READOUT_SPEED		200001
#define MXLV_AVIEX_PCCD_9785_DH_TEST_MODE		200002
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_CORRECTION	200003
#define MXLV_AVIEX_PCCD_9785_DH_EXPOSURE_MODE		200004
#define MXLV_AVIEX_PCCD_9785_DH_LINEARIZATION		200005
#define MXLV_AVIEX_PCCD_9785_DH_HORIZONTAL_TEST_PATTERN 200006
#define MXLV_AVIEX_PCCD_9785_DH_SHUTTER_OUTPUT_DISABLED	200007

#define MXD_AVIEX_PCCD_9785_STANDARD_FIELDS \
  {-1, -1, "dh_base", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_9785.base), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_CONTROL, \
		-1, "dh_control", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_9785.control), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_PHYSICAL_LINES_IN_QUADRANT, \
		-1, "dh_physical_lines_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_AVIEX_PCCD, u.dh_9785.physical_lines_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_PHYSICAL_PIXELS_PER_LINE_IN_QUADRANT, \
      -1, "dh_physical_pixels_per_line_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
    offsetof(MX_AVIEX_PCCD, u.dh_9785.physical_pixels_per_line_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_LINES_READ_IN_QUADRANT, \
		-1, "dh_lines_read_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.lines_read_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_PIXELS_READ_IN_QUADRANT, \
		-1, "dh_pixels_read_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.pixels_read_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_READOUT_DELAY_TIME, \
		-1, "dh_readout_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.readout_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_EXPOSURE_TIME, \
		-1, "dh_exposure_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.exposure_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_READOUT_DELAY_TIME, \
		-1, "dh_readout_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.readout_delay_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_FRAMES_PER_SEQUENCE, \
		-1, "dh_frames_per_sequence", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.frames_per_sequence), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_GAP_TIME, \
		-1, "dh_gap_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.gap_time), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_CONTROLLER_FPGA_VERSION, \
		-1, "dh_controller_fpga_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.controller_fpga_version), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_LINE_BINNING, \
		-1, "dh_line_binning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.line_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_PIXEL_BINNING, \
		-1, "dh_pixel_binning", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.pixel_binning), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_SUBFRAME_SIZE, \
		-1, "dh_subframe_size", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.subframe_size), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_SUBIMAGES_PER_READ, \
		-1, "dh_subimages_per_read", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.subimages_per_read), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_STREAK_MODE_LINES, \
		-1, "dh_streak_mode_lines", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.streak_mode_lines), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_W1, \
		-1, "dh_offset_w1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_w1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_X1, \
		-1, "dh_offset_x1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_x1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_Y1, \
		-1, "dh_offset_y1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_y1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_Z1, \
		-1, "dh_offset_z1", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_z1), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_W2, \
		-1, "dh_offset_w2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_w2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_X2, \
		-1, "dh_offset_x2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_x2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_Y2, \
		-1, "dh_offset_y2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_y2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_Z2, \
		-1, "dh_offset_z2", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_z2), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_DETECTOR_READOUT_MODE, \
  		-1, "dh_detector_readout_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.detector_readout_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_READOUT_SPEED, \
  		-1, "dh_readout_speed", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.readout_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_TEST_MODE, \
  		-1, "dh_test_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.test_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_CORRECTION, \
  		-1, "dh_offset_correction", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_correction), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_EXPOSURE_MODE, \
  		-1, "dh_exposure_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.exposure_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_LINEARIZATION, \
  		-1, "dh_linearization", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.linearization), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_HORIZONTAL_TEST_PATTERN, \
  		-1, "dh_horizontal_test_pattern", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.horizontal_test_pattern), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_SHUTTER_OUTPUT_DISABLED, \
 	-1, "dh_shutter_output_disabled", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, \
				u.dh_9785.shutter_output_disabled), \
	{0}, NULL, 0}

#endif /* __D_AVIEX_PCCD_9785_H__ */

