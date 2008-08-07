/*
 * Name:    mx_scaler.h
 *
 * Purpose: MX system header file for scalers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SCALER_H__
#define __MX_SCALER_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Values for the scaler flags field. */

#define MXF_SCL_SUBTRACT_DARK_CURRENT		0x1
#define MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT	0x2

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
	                    * to this scaler.
	                    */
	long value;
	long raw_value;             /* Without dark current subtraction. */

	double dark_current;        /* Expressed in counts per second. */

	unsigned long scaler_flags;

	char timer_record_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	MX_RECORD *timer_record;

	long parameter_type;

	mx_bool_type clear;
	mx_bool_type overflow_set;
	mx_bool_type busy;
	mx_bool_type stop;

	long mode;
} MX_SCALER;

#define MXLV_SCL_VALUE			6001
#define MXLV_SCL_RAW_VALUE		6002
#define MXLV_SCL_DARK_CURRENT		6003
#define MXLV_SCL_CLEAR			6004
#define MXLV_SCL_OVERFLOW_SET		6005
#define MXLV_SCL_BUSY			6006
#define MXLV_SCL_STOP			6007
#define MXLV_SCL_MODE			6008

#define MX_SCALER_STANDARD_FIELDS \
  {MXLV_SCL_VALUE, -1, "value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, value), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_POLL)}, \
  \
  {MXLV_SCL_RAW_VALUE, -1, "raw_value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, raw_value), \
	{0}, NULL, MXFF_POLL }, \
  \
  {MXLV_SCL_DARK_CURRENT, -1, "dark_current", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, dark_current), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scaler_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, scaler_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "timer_record_name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH},\
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, timer_record_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_SCL_CLEAR, -1, "clear", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, clear), \
	{0}, NULL, 0 }, \
  \
  {MXLV_SCL_OVERFLOW_SET, -1, "overflow_set", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, overflow_set), \
	{0}, NULL, 0 }, \
  \
  {MXLV_SCL_BUSY, -1, "busy", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, busy), \
	{0}, NULL, MXFF_POLL }, \
  \
  {MXLV_SCL_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, stop), \
	{0}, NULL, 0 }, \
  \
  {MXLV_SCL_MODE, -1, "mode", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_SCALER, mode), \
	{0}, NULL, 0 }

/*
 * The structure type MX_SCALER_FUNCTION_LIST contains a list of 
 * pointers to functions that vary from scaler type to scaler type.
 */

typedef struct {
	mx_status_type ( *clear ) ( MX_SCALER *scaler );
	mx_status_type ( *overflow_set ) ( MX_SCALER *scaler );
	mx_status_type ( *read ) ( MX_SCALER *scaler );
	mx_status_type ( *read_raw ) ( MX_SCALER *scaler );
	mx_status_type ( *scaler_is_busy ) ( MX_SCALER *scaler );
	mx_status_type ( *start ) ( MX_SCALER *scaler );
	mx_status_type ( *stop ) ( MX_SCALER *scaler );
	mx_status_type ( *get_parameter ) ( MX_SCALER *scaler );
	mx_status_type ( *set_parameter ) ( MX_SCALER *scaler );
	mx_status_type ( *set_modes_of_associated_counters )
					( MX_SCALER *scaler );
} MX_SCALER_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_scaler_get_pointers( MX_RECORD *scaler_record,
	MX_SCALER **scaler, MX_SCALER_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API_PRIVATE mx_status_type mx_scaler_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mx_scaler_clear( MX_RECORD *scaler_record );

MX_API mx_status_type mx_scaler_overflow_set( MX_RECORD *scaler_record,
						mx_bool_type *overflow_set);

MX_API mx_status_type mx_scaler_read( MX_RECORD *scaler_record, long *value );

MX_API mx_status_type mx_scaler_read_raw( MX_RECORD *scaler_record,
							long *value );

MX_API mx_status_type mx_scaler_is_busy( MX_RECORD *scaler_record,
						mx_bool_type *busy );

MX_API mx_status_type mx_scaler_start( MX_RECORD *scaler_record,
						long preset_count );

MX_API mx_status_type mx_scaler_stop( MX_RECORD *scaler_record,
						long *present_value );

MX_API mx_status_type mx_scaler_get_mode( MX_RECORD *scaler_record, long *mode);

MX_API mx_status_type mx_scaler_set_mode( MX_RECORD *scaler_record, long mode);

MX_API mx_status_type mx_scaler_set_modes_of_associated_counters(
						MX_RECORD *scaler_record );

MX_API mx_status_type mx_scaler_get_dark_current( MX_RECORD *scaler_record,
							double *dark_current );

MX_API mx_status_type mx_scaler_set_dark_current( MX_RECORD *scaler_record,
							double dark_current );

MX_API mx_status_type mx_scaler_default_get_parameter_handler(
							MX_SCALER *scaler );

MX_API mx_status_type mx_scaler_default_set_parameter_handler(
							MX_SCALER *scaler );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SCALER_H__ */
