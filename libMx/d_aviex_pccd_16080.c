/*
 * Name:    d_aviex_pccd_16080.c
 *
 * Purpose: Functions and definitions that are specific to the
 *          Aviex PCCD-16080 detector.
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

#define MXD_AVIEX_PCCD_16080_DEBUG_DESCRAMBLING        TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "d_aviex_pccd.h"

/*-------------------------------------------------------------------------*/

/* PCCD-16080 data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_aviex_pccd_16080_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_AVIEX_PCCD_STANDARD_FIELDS,
	MXD_AVIEX_PCCD_16080_STANDARD_FIELDS
};

long mxd_aviex_pccd_16080_num_record_fields
		= sizeof( mxd_aviex_pccd_16080_record_field_defaults )
		/ sizeof( mxd_aviex_pccd_16080_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aviex_pccd_16080_rfield_def_ptr
			= &mxd_aviex_pccd_16080_record_field_defaults[0];

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_descramble_raw_data( uint16_t *raw_frame_data,
					uint16_t ***image_sector_array,
					long i_framesize,
					long j_framesize )
{
	static const char fname[] =
			"mxd_aviex_pccd_16080_descramble_raw_data()";

	mx_warning(
	"%s: Descrambling is not yet implemented for PCCD-16080 detectors.",
		fname );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_aviex_pccd_16080_descramble_streak_camera( MX_AREA_DETECTOR *ad,
					MX_AVIEX_PCCD *aviex_pccd,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *raw_frame )
{
	static const char fname[] =
			"mxd_aviex_pccd_16080_descramble_streak_camera()";

	mx_warning("%s: Streak camera descrambling is not yet implemented "
		"for PCCD-16080 detectors.", fname );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/
