/*
 * Name:     mx_list_head.h
 *
 * Purpose:  Header file to support list head records.
 *
 * Author:   William Lavender
 *
 * NOTE:     The MX_LIST_HEAD structure itself is defined in mx_record.h
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2006-2010, 2012-2019, 2022-2023
 *    Illinois Institute of Technology
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

#define MXLV_LHD_DEBUG_LEVEL			1001
#define MXLV_LHD_STATUS				1002
#define MXLV_LHD_REPORT				1003
#define MXLV_LHD_REPORT_ALL			1004
#define MXLV_LHD_UPDATE_ALL			1005
#define MXLV_LHD_SUMMARY			1006
#define MXLV_LHD_SHOW_RECORD_LIST		1007
#define MXLV_LHD_FIELDDEF			1008
#define MXLV_LHD_SHOW_HANDLE			1009
#define MXLV_LHD_SHOW_CALLBACKS			1010
#define MXLV_LHD_SHOW_CALLBACK_ID		1011
#define MXLV_LHD_BREAKPOINT_NUMBER		1012
#define MXLV_LHD_NUMBERED_BREAKPOINT_STATUS	1013
#define MXLV_LHD_BREAKPOINT			1014
#define MXLV_LHD_CRASH				1015
#define MXLV_LHD_DEBUGGER_PID			1016
#define MXLV_LHD_SHOW_OPEN_FDS			1017
#define MXLV_LHD_CALLBACKS_ENABLED		1018
#define MXLV_LHD_CFLAGS				1019
#define MXLV_LHD_VM_REGION			1020
#define MXLV_LHD_POSIX_TIME			1021
#define MXLV_LHD_REVISION_NUMBER		1022
#define MXLV_LHD_REVISION_STRING		1023
#define MXLV_LHD_BRANCH_LABEL			1024
#define MXLV_LHD_VERSION_STRING			1025
#define MXLV_LHD_SHOW_FIELD			1026
#define MXLV_LHD_SHOW_SOCKET_HANDLERS		1027
#define MXLV_LHD_SHOW_SOCKET_ID			1028
#define MXLV_LHD_SHORT_ERROR_CODES		1029
#define MXLV_LHD_SHOW_THREAD_LIST		1030
#define MXLV_LHD_SHOW_THREAD_INFO		1031
#define MXLV_LHD_SHOW_THREAD_STACK		1032

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
  {-1, -1, "network_debug_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, network_debug_flags),\
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
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "mx_version_time", MXFT_UINT64, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, mx_version_time), \
	{0}, NULL, MXFF_READ_ONLY}, \
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
  {MXLV_LHD_UPDATE_ALL, -1, "update_all", MXFT_STRING, NULL, \
	  				1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, update_all), \
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
  {MXLV_LHD_SHOW_SOCKET_HANDLERS, -1, "show_socket_handlers", \
					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
			offsetof(MX_LIST_HEAD, show_socket_handlers), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_SOCKET_ID, -1, "show_socket_id", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_socket_id), \
	{sizeof(unsigned long)}, NULL, 0}, \
  \
  {MXLV_LHD_SHORT_ERROR_CODES, -1, "short_error_codes", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, short_error_codes), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_BREAKPOINT_NUMBER, -1, "breakpoint_number", \
		MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, breakpoint_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_NUMBERED_BREAKPOINT_STATUS, -1, "numbered_breakpoint_status", \
		MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
		offsetof(MX_LIST_HEAD, numbered_breakpoint_status), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_BREAKPOINT, -1, "breakpoint", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, breakpoint), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_CRASH, -1, "crash", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, crash), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_DEBUGGER_PID, -1, "debugger_pid", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, debugger_pid), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_OPEN_FDS, -1, "show_open_fds", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_open_fds), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_CALLBACKS_ENABLED, -1, \
			"callbacks_enabled", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, callbacks_enabled), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_LHD_CFLAGS, -1, "cflags", MXFT_STRING, NULL, 1, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, cflags), \
	{sizeof(char)}, NULL, (MXFF_VARARGS|MXFF_READ_ONLY) }, \
  \
  {MXLV_LHD_VM_REGION, -1, "vm_region", MXFT_HEX, NULL, 1, {2}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, vm_region), \
	{sizeof(unsigned long)}, NULL, 0}, \
  \
  {MXLV_LHD_POSIX_TIME, -1, "posix_time", MXFT_UINT64, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, posix_time), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_LHD_REVISION_NUMBER, -1, "mx_revision_number", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, mx_revision_number),\
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_LHD_REVISION_STRING, -1, "mx_revision_string", \
		MXFT_STRING, NULL, 1, {MXU_REVISION_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, mx_revision_string), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_LHD_BRANCH_LABEL, -1, "mx_branch_label", \
		MXFT_STRING, NULL, 1, {MXU_REVISION_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, mx_branch_label), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_LHD_VERSION_STRING, -1, "mx_version_string", \
		MXFT_STRING, NULL, 1, {MXU_REVISION_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, mx_version_string), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_LHD_SHOW_FIELD, -1, "show_field", \
		MXFT_STRING, NULL, 1, {MXU_RECORD_FIELD_NAME_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_field), \
	{sizeof(char)}, NULL, 0}, \
  \
  {-1, -1, "is_server", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, is_server), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "plotting_enabled", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, plotting_enabled), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "default_precision", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, default_precision), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "default_data_format", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
	  		offsetof(MX_LIST_HEAD, default_data_format), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "server_protocols_active", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
	  		offsetof(MX_LIST_HEAD, server_protocols_active), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "num_server_records", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
	  		offsetof(MX_LIST_HEAD, num_server_records), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "num_poll_callbacks", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
	  		offsetof(MX_LIST_HEAD, num_poll_callbacks), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "poll_callback_interval", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
	  		offsetof(MX_LIST_HEAD, poll_callback_interval), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "socket_multiplexer_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
			offsetof(MX_LIST_HEAD, socket_multiplexer_type), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "max_network_dump_bytes", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, \
			offsetof(MX_LIST_HEAD, max_network_dump_bytes), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_THREAD_LIST, -1, "show_thread_list", MXFT_HEX, NULL, 0, {0},\
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_thread_list), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_THREAD_INFO, -1, "show_thread_info", MXFT_HEX, NULL, 0, {0},\
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_thread_info), \
	{0}, NULL, 0}, \
  \
  {MXLV_LHD_SHOW_THREAD_STACK, -1, "show_thread_stack", MXFT_HEX, NULL, 0, {0},\
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, show_thread_stack), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "thread_stack_signal", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_LIST_HEAD, thread_stack_signal),\
	{0}, NULL, MXFF_READ_ONLY }

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
