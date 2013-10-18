/*
 * Name:    v_biocat_toast_joystick.h
 *
 * Purpose: MX variable header for a driver to enable or disable
 *          the BioCAT "toast" device joystick.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_BIOCAT_TOAST_JOYSTICK_H__
#define __V_BIOCAT_TOAST_JOYSTICK_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *compumotor_interface_record;
	unsigned long num_joystick_axes;
} MX_BIOCAT_TOAST_JOYSTICK;


#define MXV_BIOCAT_TOAST_JOYSTICK_STANDARD_FIELDS \
  {-1, -1, "compumotor_interface_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_BIOCAT_TOAST_JOYSTICK, compumotor_interface_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_joystick_axes", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BIOCAT_TOAST_JOYSTICK, num_joystick_axes), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }

MX_API_PRIVATE mx_status_type
		mxv_biocat_toast_joystick_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_biocat_toast_joystick_send_variable(
							MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_biocat_toast_joystick_receive_variable(
							MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST
			mxv_biocat_toast_joystick_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST
			mxv_biocat_toast_joystick_variable_function_list;

extern long mxv_biocat_toast_joystick_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_biocat_toast_joystick_rfield_def_ptr;

#endif /* __V_BIOCAT_TOAST_JOYSTICK_H__ */

