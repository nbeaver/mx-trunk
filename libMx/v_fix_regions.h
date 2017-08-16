/*
 * Name:    v_fix_regions.h
 *
 * Purpose: Header file for setting up the fixing of image regions in
 *          an area detector image.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_FIX_REGIONS_H__
#define __V_FIX_REGIONS_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *image_record;
	long num_fix_regions;
	long **fix_region_array;
} MX_FIX_REGIONS;

#define MX_FIX_REGIONS_STANDARD_FIELDS \
  {-1, -1, "image_record", MXFT_RECORD, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIX_REGIONS, image_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_fix_regions", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIX_REGIONS, num_fix_regions), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "fix_region_array", MXFT_LONG, NULL, 2, {MXU_VARARGS_LENGTH, 5}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIX_REGIONS, fix_region_array), \
	{sizeof(long **)}, NULL, MXFF_VARARGS }

MX_API_PRIVATE mx_status_type mxv_fix_regions_initialize_driver(
							MX_DRIVER *driver );
MX_API_PRIVATE mx_status_type mxv_fix_regions_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_fix_regions_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_fix_regions_send_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_fix_regions_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_fix_regions_variable_function_list;

extern long mxv_fix_regions_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_fix_regions_rfield_def_ptr;

#endif /* __V_FIX_REGIONS_H__ */

