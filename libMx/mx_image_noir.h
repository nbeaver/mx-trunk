/*
 * Name:    mx_image_noir.h
 *
 * Purpose: Information used to generate NOIR format image files.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_IMAGE_NOIR_H__
#define __MX_IMAGE_NOIR_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_record.h"
#include "mx_io.h"

typedef struct {
	MX_FILE_MONITOR *file_monitor;

	unsigned long static_header_length;
	char *static_header_text;

	MX_RECORD *mx_noir_records_list;

	MX_RECORD *energy_motor_record;
	MX_RECORD *wavelength_motor_record;
	MX_RECORD *beam_x_motor_record;
	MX_RECORD *beam_y_motor_record;
	MX_RECORD *oscillation_distance_record;

	double energy;
	double wavelength;
	double beam_x;
	double beam_y;
	double oscillation_distance;
} MX_IMAGE_NOIR_INFO;

MX_API mx_status_type mx_image_noir_setup( MX_RECORD *record_list,
				char *static_file_header_name,
				MX_IMAGE_NOIR_INFO **image_noir_info_ptr );

MX_API mx_status_type mx_image_noir_update(
				MX_IMAGE_NOIR_INFO *image_noir_info );

MX_API mx_status_type mx_image_noir_write_header( FILE *file,
				MX_IMAGE_NOIR_INFO *image_noir_info );

#ifdef __cplusplus
}
#endif

#endif /* __MX_IMAGE_NOIR_H__ */

