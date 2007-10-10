/*
 * Name:    mx_ptz.h
 *
 * Purpose: MX header file for Pan/Tilt/Zoom camera supports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PTZ_H__
#define __MX_PTZ_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_PTZ_MAX_PARAMETERS		2

#define MXSF_PTZ_IS_BUSY		0x1

/* Flag values for mx_ptz_command(). */

#define MXF_PTZ_DRIVE_TYPE		0x1000

#define MXF_PTZ_PAN_LEFT		0x1001
#define MXF_PTZ_PAN_RIGHT		0x1002
#define MXF_PTZ_TILT_UP			0x1003
#define MXF_PTZ_TILT_DOWN		0x1004

#define MXF_PTZ_DRIVE_UPPER_LEFT	0x1005
#define MXF_PTZ_DRIVE_UPPER_RIGHT	0x1006
#define MXF_PTZ_DRIVE_LOWER_LEFT	0x1007
#define MXF_PTZ_DRIVE_LOWER_RIGHT	0x1008

#define MXF_PTZ_PAN_STOP		0x1009
#define MXF_PTZ_TILT_STOP		0x100a
#define MXF_PTZ_DRIVE_STOP		0x100b
#define MXF_PTZ_DRIVE_HOME		0x100c

#define MXF_PTZ_PAN_POSITION		0x1101
#define MXF_PTZ_PAN_DESTINATION		0x1102
#define MXF_PTZ_TILT_POSITION		0x1103
#define MXF_PTZ_TILT_DESTINATION	0x1104

#define MXF_PTZ_PAN_SPEED		0x1105
#define MXF_PTZ_TILT_SPEED		0x1106

#define MXF_PTZ_DRIVE_OFF		0x1801
#define MXF_PTZ_DRIVE_ON		0x1802

#define MXF_PTZ_ZOOM_TYPE		0x2000

#define MXF_PTZ_ZOOM_OUT		0x2001
#define MXF_PTZ_ZOOM_IN			0x2002
#define MXF_PTZ_ZOOM_STOP		0x2003

#define MXF_PTZ_ZOOM_POSITION		0x2101
#define MXF_PTZ_ZOOM_DESTINATION	0x2102

#define MXF_PTZ_ZOOM_SPEED		0x2103

#define MXF_PTZ_ZOOM_OFF		0x2801
#define MXF_PTZ_ZOOM_ON			0x2802

#define MXF_PTZ_FOCUS_TYPE		0x4000

#define MXF_PTZ_FOCUS_MANUAL		0x4001
#define MXF_PTZ_FOCUS_AUTO		0x4002
#define MXF_PTZ_FOCUS_FAR		0x4003
#define MXF_PTZ_FOCUS_NEAR		0x4004
#define MXF_PTZ_FOCUS_STOP		0x4005

#define MXF_PTZ_FOCUS_POSITION		0x4101
#define MXF_PTZ_FOCUS_DESTINATION	0x4102

#define MXF_PTZ_FOCUS_SPEED		0x4103

#define MXF_PTZ_FOCUS_OFF		0x4801
#define MXF_PTZ_FOCUS_ON		0x4802

/* Flag values for mx_ptz_get_status(). */

#define MXSF_PTZ_PAN_IS_BUSY		0x1
#define MXSF_PTZ_TILT_IS_BUSY		0x2
#define MXSF_PTZ_ZOOM_IS_BUSY		0x4
#define MXSF_PTZ_FOCUS_IS_BUSY		0x8

#define MXSF_PTZ_PAN_LEFT_LIMIT_HIT	0x10
#define MXSF_PTZ_PAN_RIGHT_LIMIT_HIT	0x20
#define MXSF_PTZ_TILT_TOP_LIMIT_HIT	0x40
#define MXSF_PTZ_TILT_BOTTOM_LIMIT_HIT	0x80

#define MXSF_PTZ_ZOOM_IN_LIMIT_HIT	0x100
#define MXSF_PTZ_ZOOM_OUT_LIMIT_HIT	0x200
#define MXSF_PTZ_FOCUS_NEAR_LIMIT_HIT	0x400
#define MXSF_PTZ_FOCUS_FAR_LIMIT_HIT	0x800

#define MXSF_PTZ_PAN_FOLLOWING_ERROR	0x1000
#define MXSF_PTZ_TILT_FOLLOWING_ERROR	0x2000
#define MXSF_PTZ_ZOOM_FOLLOWING_ERROR	0x4000
#define MXSF_PTZ_FOCUS_FOLLOWING_ERROR	0x8000

#define MXSF_PTZ_PAN_DRIVE_FAULT	0x10000
#define MXSF_PTZ_TILT_DRIVE_FAULT	0x20000
#define MXSF_PTZ_ZOOM_DRIVE_FAULT	0x40000
#define MXSF_PTZ_FOCUS_DRIVE_FAULT	0x80000

#define MXSF_PTZ_HOME_SEARCH_SUCCEEDED	0x40000000
#define MXSF_PTZ_CONTROLLER_DISABLED	0x80000000

typedef struct {
	MX_RECORD *record;

	unsigned long ptz_flags;

	int parameter_type;
	unsigned long parameter_value[MXU_PTZ_MAX_PARAMETERS];

	unsigned long command;
	unsigned long status;

	long pan_position;
	long pan_destination;
	unsigned long pan_speed;

	long tilt_position;
	long tilt_destination;
	unsigned long tilt_speed;

	unsigned long zoom_position;
	unsigned long zoom_destination;
	unsigned long zoom_speed;
	unsigned long zoom_on;

	unsigned long focus_position;
	unsigned long focus_destination;
	unsigned long focus_speed;
	unsigned long focus_auto;
} MX_PAN_TILT_ZOOM;

#define MXLV_PTZ_COMMAND		1001
#define MXLV_PTZ_STATUS			1002
#define MXLV_PTZ_PAN_POSITION		1003
#define MXLV_PTZ_PAN_DESTINATION	1004
#define MXLV_PTZ_PAN_SPEED		1005
#define MXLV_PTZ_TILT_POSITION		1006
#define MXLV_PTZ_TILT_DESTINATION	1007
#define MXLV_PTZ_TILT_SPEED		1008
#define MXLV_PTZ_ZOOM_POSITION		1009
#define MXLV_PTZ_ZOOM_DESTINATION	1010
#define MXLV_PTZ_ZOOM_SPEED		1011
#define MXLV_PTZ_ZOOM_ON		1012
#define MXLV_PTZ_FOCUS_POSITION		1013
#define MXLV_PTZ_FOCUS_DESTINATION	1014
#define MXLV_PTZ_FOCUS_SPEED		1015
#define MXLV_PTZ_FOCUS_AUTO		1016

#define MX_PAN_TILT_ZOOM_STANDARD_FIELDS \
  {-1, -1, "ptz_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, ptz_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_PTZ_COMMAND, -1, "command", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, command), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_STATUS, -1, "status", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, status), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_PAN_POSITION, -1, "pan_position", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, pan_position), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_PTZ_PAN_DESTINATION, -1, "pan_destination", MXFT_LONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, pan_destination), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_PAN_SPEED, -1, "pan_speed", MXFT_ULONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, pan_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_TILT_POSITION, -1, "tilt_position", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, tilt_position), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_PTZ_TILT_DESTINATION, -1, "tilt_destination", MXFT_LONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, tilt_destination), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_TILT_SPEED, -1, "tilt_speed", MXFT_ULONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, tilt_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_ZOOM_POSITION, -1, "zoom_position", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, zoom_position), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_PTZ_ZOOM_DESTINATION, -1, "zoom_destination", MXFT_ULONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, zoom_destination), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_ZOOM_SPEED, -1, "zoom_speed", MXFT_ULONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, zoom_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_ZOOM_ON, -1, "zoom_on", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, zoom_on), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_FOCUS_POSITION, -1, "focus_position", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, focus_position), \
	{0}, NULL, MXFF_IN_SUMMARY}, \
  \
  {MXLV_PTZ_FOCUS_DESTINATION, -1, "focus_destination", \
	  					MXFT_ULONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, focus_destination), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_FOCUS_SPEED, -1, "focus_speed", MXFT_ULONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, focus_speed), \
	{0}, NULL, 0}, \
  \
  {MXLV_PTZ_FOCUS_AUTO, -1, "focus_auto", MXFT_ULONG, NULL, 0, {0},\
  	MXF_REC_CLASS_STRUCT, offsetof(MX_PAN_TILT_ZOOM, focus_auto), \
	{0}, NULL, 0}

/*
 * The structure type MX_PAN_TILT_ZOOM_FUNCTION_LIST contains
 * a list of pointers to functions that vary from driver to driver.
 */

typedef struct {
	mx_status_type ( *command ) ( MX_PAN_TILT_ZOOM *ptz );
	mx_status_type ( *get_status ) ( MX_PAN_TILT_ZOOM *ptz );
	mx_status_type ( *get_parameter ) ( MX_PAN_TILT_ZOOM *ptz );
	mx_status_type ( *set_parameter ) ( MX_PAN_TILT_ZOOM *ptz );
} MX_PAN_TILT_ZOOM_FUNCTION_LIST;

MX_API mx_status_type mx_ptz_command( MX_RECORD *ptz_record,
					unsigned long command );

MX_API mx_status_type mx_ptz_get_status( MX_RECORD *ptz_record,
					unsigned long *status );

/* ----- */

MX_API mx_status_type mx_ptz_get_parameter( MX_RECORD *ptz_record,
					int parameter_type,
					unsigned long *parameter_value );

MX_API mx_status_type mx_ptz_set_parameter( MX_RECORD *ptz_record,
					int parameter_type,
					unsigned long parameter_value );

MX_API mx_status_type mx_ptz_default_command_handler( MX_PAN_TILT_ZOOM *ptz );

MX_API mx_status_type mx_ptz_default_get_parameter_handler(
						MX_PAN_TILT_ZOOM *ptz );

MX_API mx_status_type mx_ptz_default_set_parameter_handler(
						MX_PAN_TILT_ZOOM *ptz );

/* ----- */

MX_API mx_status_type mx_ptz_pan_left( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_pan_right( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_pan_stop( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_get_pan( MX_RECORD *ptz_record,
					long *pan_value );

MX_API mx_status_type mx_ptz_set_pan( MX_RECORD *ptz_record,
					long pan_value );

MX_API mx_status_type mx_ptz_get_pan_speed( MX_RECORD *ptz_record,
					unsigned long *pan_speed );

MX_API mx_status_type mx_ptz_set_pan_speed( MX_RECORD *ptz_record,
					unsigned long pan_speed );

/* ----- */

MX_API mx_status_type mx_ptz_tilt_up( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_tilt_down( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_tilt_stop( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_get_tilt( MX_RECORD *ptz_record,
					long *tilt_value );

MX_API mx_status_type mx_ptz_set_tilt( MX_RECORD *ptz_record,
					long tilt_value );

MX_API mx_status_type mx_ptz_get_tilt_speed( MX_RECORD *ptz_record,
					unsigned long *tilt_speed );

MX_API mx_status_type mx_ptz_set_tilt_speed( MX_RECORD *ptz_record,
					unsigned long tilt_speed );

/* ----- */

MX_API mx_status_type mx_ptz_zoom_in( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_zoom_out( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_zoom_stop( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_get_zoom( MX_RECORD *ptz_record,
					unsigned long *zoom_value );

MX_API mx_status_type mx_ptz_set_zoom( MX_RECORD *ptz_record,
					unsigned long zoom_value );

MX_API mx_status_type mx_ptz_get_zoom_speed( MX_RECORD *ptz_record,
					unsigned long *zoom_speed );

MX_API mx_status_type mx_ptz_set_zoom_speed( MX_RECORD *ptz_record,
					unsigned long zoom_speed );

MX_API mx_status_type mx_ptz_zoom_off( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_zoom_on( MX_RECORD *ptz_record );

/* ----- */

MX_API mx_status_type mx_ptz_focus_manual( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_focus_auto( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_focus_far( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_focus_near( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_focus_stop( MX_RECORD *ptz_record );

MX_API mx_status_type mx_ptz_get_focus( MX_RECORD *ptz_record,
					unsigned long *focus_value );

MX_API mx_status_type mx_ptz_set_focus( MX_RECORD *ptz_record,
					unsigned long focus_value );

MX_API mx_status_type mx_ptz_get_focus_speed( MX_RECORD *ptz_record,
					unsigned long *focus_speed );

MX_API mx_status_type mx_ptz_set_focus_speed( MX_RECORD *ptz_record,
					unsigned long focus_speed );

#ifdef __cplusplus
}
#endif

#endif /* __MX_PTZ_H__ */

