/*
 * Name:    d_aviex_pccd_16080.h
 *
 * Purpose: Header file definitions that are specific to the Aviex
 *          PCCD-16080 detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AVIEX_PCCD_16080_H__
#define __D_AVIEX_PCCD_16080_H__

/*-------------------------------------------------------------*/

struct mx_aviex_pccd;

typedef struct {
	unsigned long base;
} MX_AVIEX_PCCD_16080_DETECTOR_HEAD;

extern long mxd_aviex_pccd_16080_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aviex_pccd_16080_rfield_def_ptr;

/*-------------------------------------------------------------*/

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_descramble_raw_data( uint16_t *,
					uint16_t ***, long, long );

MX_API_PRIVATE mx_status_type
mxd_aviex_pccd_16080_descramble_streak_camera( MX_AREA_DETECTOR *,
						struct mx_aviex_pccd *,
						MX_IMAGE_FRAME *,
						MX_IMAGE_FRAME * );

/*-------------------------------------------------------------*/

#define MXD_AVIEX_PCCD_16080_STANDARD_FIELDS \
  {-1, -1, "dh_base", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AVIEX_PCCD, u.dh_16080.base), \
	{0}, NULL, 0}

#endif /* __D_AVIEX_PCCD_16080_H__ */

