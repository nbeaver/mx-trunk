/*
 * Name:    mx_sample_changer.h
 *
 * Purpose: MX system header file for experimental sample changers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SAMPLE_CHANGER_H__
#define __MX_SAMPLE_CHANGER_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_SAMPLE_HOLDER_NAME_LENGTH	80

#define MX_CHG_NO_SAMPLE_HOLDER		""
#define MX_CHG_NO_SAMPLE_ID		(-1)

/* Sample changer status flags. */

#define MXSF_CHG_SAMPLE_MOUNTED			0x1
#define MXSF_CHG_SAMPLE_GRABBED			0x2
#define MXSF_CHG_SAMPLE_HOLDER_SELECTED		0x4

#define MXSF_CHG_OPERATION_IN_PROGRESS		0x10
#define MXSF_CHG_INITIALIZATION_IN_PROGRESS	0x20
#define MXSF_CHG_SHUTDOWN_IN_PROGRESS		0x40
#define MXSF_CHG_IDLE_IN_PROGRESS		0x80

#define MXSF_CHG_FAULT				0x100
#define MXSF_CHG_USER_ABORT			0x200

#define MXSF_CHG_ERROR				0x80000000

#define MXSF_CHG_IS_BUSY  \
	( MXSF_CHG_OPERATION_IN_PROGRESS \
	  | MXSF_CHG_INITIALIZATION_IN_PROGRESS \
	  | MXSF_CHG_SHUTDOWN_IN_PROGRESS \
	  | MXSF_CHG_IDLE_IN_PROGRESS )

#define MXSF_CHG_ERROR_BITMASK \
	( MXSF_CHG_FAULT | MXSF_CHG_USER_ABORT | MXSF_CHG_ERROR )

#define MXF_CHG_CONTROL_MODE_LOCAL	0x1
#define MXF_CHG_CONTROL_MODE_REMOTE	0x2

typedef struct {
	MX_RECORD *record;

	char current_sample_holder[MXU_SAMPLE_HOLDER_NAME_LENGTH + 1];
	long current_sample_id;
	mx_bool_type sample_is_mounted;

	char requested_sample_holder[MXU_SAMPLE_HOLDER_NAME_LENGTH + 1];
	long requested_sample_id;

	unsigned long control_mode;
	unsigned long status;
	unsigned long changer_flags;

	long parameter_type;

	mx_bool_type initialize;
	mx_bool_type shutdown;
	mx_bool_type mount_sample;
	mx_bool_type unmount_sample;
	mx_bool_type grab_sample;
	mx_bool_type ungrab_sample;
	mx_bool_type select_sample_holder;
	mx_bool_type unselect_sample_holder;
	mx_bool_type soft_abort;
	mx_bool_type immediate_abort;
	mx_bool_type idle;
	mx_bool_type reset;

	mx_bool_type cooldown;
	mx_bool_type deice;

} MX_SAMPLE_CHANGER;

#define MXLV_CHG_CONTROL_MODE			18001
#define MXLV_CHG_STATUS				18002
#define MXLV_CHG_CHANGER_FLAGS			18003
#define MXLV_CHG_INITIALIZE			18004
#define MXLV_CHG_SHUTDOWN			18005
#define MXLV_CHG_MOUNT_SAMPLE			18006
#define MXLV_CHG_UNMOUNT_SAMPLE			18007
#define MXLV_CHG_GRAB_SAMPLE			18008
#define MXLV_CHG_UNGRAB_SAMPLE			18009
#define MXLV_CHG_SELECT_SAMPLE_HOLDER		18010
#define MXLV_CHG_UNSELECT_SAMPLE_HOLDER		18011
#define MXLV_CHG_SOFT_ABORT			18012
#define MXLV_CHG_IMMEDIATE_ABORT		18013
#define MXLV_CHG_IDLE				18014
#define MXLV_CHG_RESET				18015
#define MXLV_CHG_COOLDOWN			18016
#define MXLV_CHG_DEICE				18017

#define MX_SAMPLE_CHANGER_STANDARD_FIELDS \
  {-1, -1, "current_sample_holder", MXFT_STRING, \
	  			NULL, 1, {MXU_SAMPLE_HOLDER_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT,  \
			offsetof(MX_SAMPLE_CHANGER, current_sample_holder), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {-1, -1, "current_sample_id", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, current_sample_id), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, \
				"requested_sample_holder", MXFT_STRING, \
	  			NULL, 1, {MXU_SAMPLE_HOLDER_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_SAMPLE_CHANGER, requested_sample_holder), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {-1, -1, "requested_sample_id", \
					MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_SAMPLE_CHANGER, requested_sample_id), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_CONTROL_MODE, -1, "control_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, control_mode), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, status), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_CHANGER_FLAGS, -1, "changer_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, changer_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_CHG_INITIALIZE, -1, "initialize", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, initialize), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_SHUTDOWN, -1, "shutdown", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, shutdown), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_MOUNT_SAMPLE, -1, "mount_sample", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, mount_sample), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_UNMOUNT_SAMPLE, -1, "unmount_sample", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, unmount_sample), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_GRAB_SAMPLE, -1, "grab_sample", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, grab_sample), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_UNGRAB_SAMPLE, -1, "ungrab_sample", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, ungrab_sample), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_SELECT_SAMPLE_HOLDER, -1, "select_sample_holder", \
	  					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_SAMPLE_CHANGER, select_sample_holder), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_UNSELECT_SAMPLE_HOLDER, -1, "unselect_sample_holder", \
	  					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
			offsetof(MX_SAMPLE_CHANGER, unselect_sample_holder), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_SOFT_ABORT, -1, "soft_abort", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, soft_abort), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_IMMEDIATE_ABORT, -1, "immediate_abort", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, immediate_abort), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_IDLE, -1, "idle", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, idle), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_RESET, -1, "reset", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, reset), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_COOLDOWN, -1, "cooldown", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, cooldown), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CHG_DEICE, -1, "deice", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SAMPLE_CHANGER, deice), \
	{0}, NULL, 0 }

typedef struct {
	mx_status_type ( *initialize ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *shutdown ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *mount_sample ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *unmount_sample ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *grab_sample ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *ungrab_sample ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *select_sample_holder ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *unselect_sample_holder ) (MX_SAMPLE_CHANGER *changer);
	mx_status_type ( *soft_abort ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *immediate_abort ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *idle ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *reset ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *get_status ) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *get_parameter) ( MX_SAMPLE_CHANGER *changer );
	mx_status_type ( *set_parameter) ( MX_SAMPLE_CHANGER *changer );
} MX_SAMPLE_CHANGER_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_sample_changer_get_pointers(
	MX_RECORD *record,
	MX_SAMPLE_CHANGER **changer,
	MX_SAMPLE_CHANGER_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_sample_changer_initialize( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_shutdown( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_mount_sample( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_unmount_sample( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_grab_sample( MX_RECORD *record,
						long sample_id );

MX_API mx_status_type mx_sample_changer_ungrab_sample( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_select_sample_holder(
						MX_RECORD *record,
						char *sample_holder_name );

MX_API mx_status_type mx_sample_changer_unselect_sample_holder(
						MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_get_sample_holder( MX_RECORD *record,
						size_t maximum_name_length,
						char *sample_holder_name );

MX_API mx_status_type mx_sample_changer_get_sample_id( MX_RECORD *record,
						long *sample_id );

MX_API mx_status_type mx_sample_changer_set_sample_id( MX_RECORD *record,
						long sample_id );

MX_API mx_status_type mx_sample_changer_soft_abort( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_immediate_abort( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_idle( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_reset( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_get_status( MX_RECORD *record,
						unsigned long *status );

MX_API mx_status_type mx_sample_changer_is_busy( MX_RECORD *record,
						mx_bool_type *busy );

MX_API mx_status_type mx_sample_changer_default_get_parameter_handler(
					MX_SAMPLE_CHANGER *changer );

MX_API mx_status_type mx_sample_changer_default_set_parameter_handler(
					MX_SAMPLE_CHANGER *changer );

/* The following are probably only of interest for crystallography
 * sample changers.
 */

MX_API mx_status_type mx_sample_changer_cooldown( MX_RECORD *record );

MX_API mx_status_type mx_sample_changer_deice( MX_RECORD *record );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SAMPLE_CHANGER_H__ */

