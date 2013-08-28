/*
 * Name:    mx_image_noir.h
 *
 * Purpose: Information used to generate NOIR format image files.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2012-2013 Illinois Institute of Technology
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

#define MXU_MAX_IMAGE_NOIR_STRING_LENGTH	250

typedef struct {
	long datatype;
	union {
		double double_value;
		long long_value;
		char string_value[ MXU_MAX_IMAGE_NOIR_STRING_LENGTH+1 ];
	} u;
} MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE;

typedef struct {
	MX_RECORD *mx_imaging_device_record;

	char detector_name_for_header[MXU_RECORD_NAME_LENGTH+1];

	/*---*/

	MX_FILE_MONITOR *file_monitor;

	unsigned long static_header_length;
	char *static_header_text;

	/*---*/

	unsigned long dynamic_header_num_records;
	MX_RECORD **dynamic_header_record_array;
	MX_IMAGE_NOIR_DYNAMIC_HEADER_VALUE *dynamic_header_value_array;
	

	unsigned long dynamic_header_alias_dimension_array[3];
	char ***dynamic_header_alias_array;

} MX_IMAGE_NOIR_INFO;

MX_API mx_status_type mx_image_noir_setup( MX_RECORD *mx_imaging_device_record,
				char *detector_name_for_header,
				char *dynamic_header_template_name,
				char *static_header_file_name,
				MX_IMAGE_NOIR_INFO **image_noir_info_ptr );

MX_API mx_status_type mx_image_noir_update(
				MX_IMAGE_NOIR_INFO *image_noir_info );

MX_API mx_status_type mx_image_noir_write_header( FILE *file,
				MX_IMAGE_NOIR_INFO *image_noir_info );

#ifdef __cplusplus
}
#endif

#endif /* __MX_IMAGE_NOIR_H__ */

