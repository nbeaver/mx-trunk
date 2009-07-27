/*
 * Name:     mx_list_head.h
 *
 * Purpose:  Header file to support list head records.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2006-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_LIST_HEAD_H__
#define __MX_LIST_HEAD_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXLV_LHD_DEBUG_LEVEL		1001
#define MXLV_LHD_STATUS			1002
#define MXLV_LHD_REPORT			1003
#define MXLV_LHD_REPORT_ALL		1004
#define MXLV_LHD_SUMMARY		1005
#define MXLV_LHD_SHOW_RECORD_LIST	1006
#define MXLV_LHD_FIELDDEF		1007
#define MXLV_LHD_SHOW_HANDLE		1008
#define MXLV_LHD_SHOW_CALLBACKS		1009
#define MXLV_LHD_SHOW_CALLBACK_ID	1010
#define MXLV_LHD_BREAKPOINT		1011
#define MXLV_LHD_DEBUGGER_STARTED	1012

#define MXR_LIST_HEAD_STANDARD_FIELDS \
  {-1, -1, "list_is_active", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, list_is_active), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "fast_mode", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, fast_mode), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "allow_fast_mode", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, allow_fast_mode), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "network_debug", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, network_debug), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_DEBUG_LEVEL, -1, "debug_level", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, debug_level), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_STATUS, -1, "status", MXFT_STRING, NULL, \
	  				1, {MXU_FIELD_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, status), \
	{sizeof(char)}, NULL, 0}, \
  \
  {-1, -1, "mx_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, mx_version), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "num_records", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, num_records), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_LHD_REPORT, -1, "report", MXFT_STRING, NULL, \
	  				1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, report), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_LHD_REPORT_ALL, -1, "report_all", MXFT_STRING, NULL, \
	  				1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, report_all), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_LHD_SUMMARY, -1, "summary", MXFT_STRING, NULL, \
	  				1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, summary), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_RECORD_LIST, -1, "show_record_list", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_record_list), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_FIELDDEF, -1, "fielddef", MXFT_STRING, NULL, \
	  				1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, fielddef), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_HANDLE, -1, "show_handle", MXFT_ULONG, NULL, 1, {2}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_handle), \
	{sizeof(unsigned long)}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_CALLBACKS, -1, "show_callbacks", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_callbacks), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_CALLBACK_ID, -1, "show_callback_id", \
  					MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_callback_id), \
	{sizeof(unsigned long)}, NULL, 0}, \
  \
  {MXLV_LHD_BREAKPOINT, -1, "breakpoint", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, breakpoint), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_DEBUGGER_STARTED, -1, "debugger_started", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, debugger_started), \
	{0}, NULL, 0}

MX_API_PRIVATE mx_status_type mxr_create_list_head( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxr_list_head_print_structure( FILE *file,
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxr_list_head_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxr_list_head_finish_delayed_initialization(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxr_list_head_record_function_list;

extern long mxr_list_head_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxr_list_head_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __MX_LIST_HEAD_H__ */
