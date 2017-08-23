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

	MX_RECORD *imaging_record;
	long *num_fix_regions_ptr;
	long ***fix_region_array_ptr;

	MX_RECORD_FIELD *fix_region_array_field;
} MX_FIX_REGIONS;

#define MX_FIX_REGIONS_STANDARD_FIELDS \
  {-1, -1, "imaging_record", MXFT_RECORD, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_FIX_REGIONS, imaging_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

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

