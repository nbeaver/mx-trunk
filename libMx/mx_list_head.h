/*
 * Name:     mx_list_head.h
 *
 * Purpose:  Header file to support list head records.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_LIST_HEAD_H__
#define __MX_LIST_HEAD_H__

#define MXLV_LHD_STATUS		101

#define MXR_LIST_HEAD_STANDARD_FIELDS \
  {-1, -1, "list_is_active", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, list_is_active), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "fast_mode", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, fast_mode), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "allow_fast_mode", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, allow_fast_mode), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_STATUS, -1, "status", MXFT_STRING, NULL, \
	  				1, {MXU_FIELD_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, status), \
	{sizeof(char)}, NULL, 0}, \
  \
  {-1, -1, "mx_version", MXFT_UINT32, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, mx_version), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "num_records", MXFT_LENGTH, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, num_records), \
	{0}, NULL, 0}

MX_API_PRIVATE mx_status_type mxr_create_list_head( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxr_list_head_print_structure(
					FILE *file, MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxr_list_head_record_function_list;

extern long mxr_list_head_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxr_list_head_rfield_def_ptr;

#endif /* __MX_LIST_HEAD_H__ */
