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
 * Copyright 2006-2008, 2011 Illinois Institute of Technology
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
	unsigned long controller_fpga_version;

	unsigned long line_binning;
	unsigned long pixel_binning;
	unsigned long subframe_size;
	unsigned long subimages_per_read;
	unsigned long streak_mode_lines;

	unsigned long offset_w;
	unsigned long offset_x;
	unsigned long offset_y;
	unsigned long offset_z;

	unsigned long detector_readout_mode;
	unsigned long readout_speed;
	unsigned long test_mode;
	unsigned long offset_correction;
	unsigned long exposure_mode;
	unsigned long linearization;
	unsigned long dummy_frame_valid;
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

/*-------------------------------------------------------------*/

/* Control register bit definitions. */

/* Test mode (bit 0) */

#define MXF_AVIEX_PCCD_9785_TEST_MODE_ON			0x1

/* Low noise-high speed (bit 1) */

#define MXF_AVIEX_PCCD_9785_HIGH_SPEED			0x2

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
#define MXF_AVIEX_PCCD_9785_STREAK_CAMERA_MODE		0x60

/* Linearization (bit 7) */

#define MXF_AVIEX_PCCD_9785_LINEARIZATION_ON			0x80

/* Offset correction mode (bit 8) */

#define MXF_AVIEX_PCCD_9785_UNBINNED_PIXEL_AVERAGING		0x100

/* Dummy frame valid pulse (bit 9).
 *
 * The dummy frame valid pulse is to work around a "feature"
 * of the EPIX XCLIB cameras which ignore the first Frame Valid
 * signal sent to them by default.
 */

#define MXF_AVIEX_PCCD_9785_DUMMY_FRAME_VALID			0x200

/*-------------------------------------------------------------*/

#define MXLV_AVIEX_PCCD_9785_DH_CONTROL	(MXLV_AVIEX_PCCD_DH_BASE + 0)

#define MXLV_AVIEX_PCCD_9785_DH_OVERSCANNED_PIXELS_PER_LINE \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 1)

#define MXLV_AVIEX_PCCD_9785_DH_PHYSICAL_LINES_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 2)

#define MXLV_AVIEX_PCCD_9785_DH_PHYSICAL_PIXELS_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 3)


#define MXLV_AVIEX_PCCD_9785_DH_LINES_READ_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 5)

#define MXLV_AVIEX_PCCD_9785_DH_PIXELS_READ_IN_QUADRANT \
				 		(MXLV_AVIEX_PCCD_DH_BASE + 6)


#define MXLV_AVIEX_PCCD_9785_DH_INITIAL_DELAY_TIME \
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

#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_W	(MXLV_AVIEX_PCCD_DH_BASE + 24)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_X	(MXLV_AVIEX_PCCD_DH_BASE + 25)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_Y	(MXLV_AVIEX_PCCD_DH_BASE + 26)
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_Z	(MXLV_AVIEX_PCCD_DH_BASE + 27)


/* Define some pseudo registers to manipulate individual bits
 * in the control register.
 */

#define MXLV_AVIEX_PCCD_9785_DH_DETECTOR_READOUT_MODE	200000
#define MXLV_AVIEX_PCCD_9785_DH_READOUT_SPEED		200001
#define MXLV_AVIEX_PCCD_9785_DH_TEST_MODE		200002
#define MXLV_AVIEX_PCCD_9785_DH_OFFSET_CORRECTION	200003
#define MXLV_AVIEX_PCCD_9785_DH_EXPOSURE_MODE		200004
#define MXLV_AVIEX_PCCD_9785_DH_LINEARIZATION		200005
#define MXLV_AVIEX_PCCD_9785_DH_DUMMY_FRAME_VALID	200006

#define MXD_AVIEX_PCCD_9785_STANDARD_FIELDS \
  {-1, -1, "dh_base", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_9785.base), \
	{0}, NULL, 0}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_CONTROL, \
		-1, "dh_control", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_9785.control), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OVERSCANNED_PIXELS_PER_LINE, \
	    -1, "dh_overscanned_pixels_per_line", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_AVIEX_PCCD, u.dh_9785.overscanned_pixels_per_line), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_PHYSICAL_LINES_IN_QUADRANT, \
		-1, "dh_physical_lines_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_AVIEX_PCCD, u.dh_9785.physical_lines_in_quadrant), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_PHYSICAL_PIXELS_IN_QUADRANT, \
		-1, "dh_physical_pixels_in_quadrant", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_AVIEX_PCCD, u.dh_9785.physical_pixels_in_quadrant), \
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
  {MXLV_AVIEX_PCCD_9785_DH_INITIAL_DELAY_TIME, \
		-1, "dh_initial_delay_time", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.initial_delay_time), \
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
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_W, \
		-1, "dh_offset_w", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_w), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_X, \
		-1, "dh_offset_x", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_x), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_Y, \
		-1, "dh_offset_y", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_y), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_AVIEX_PCCD_9785_DH_OFFSET_Z, \
		-1, "dh_offset_z", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_AVIEX_PCCD, u.dh_9785.offset_z), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
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
  {MXLV_AVIEX_PCCD_9785_DH_DUMMY_FRAME_VALID, \
  		-1, "dh_dummy_frame_valid", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AVIEX_PCCD, u.dh_9785.dummy_frame_valid), \
	{0}, NULL, 0}

#endif /* __D_AVIEX_PCCD_9785_H__ */

