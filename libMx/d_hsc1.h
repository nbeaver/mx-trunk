/*
 * Name:    d_hsc1.h
 *
 * Purpose: Header file for the MX driver for the X-ray Instrumentation
 *          Associates HSC-1 Huber slit controller.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2010, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#ifndef __D_HSC1_H__
#define __D_HSC1_H__

#include "i_hsc1.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *hsc1_interface_record;
	unsigned long module_number;
	char motor_name;
} MX_HSC1_MOTOR;

MX_API mx_status_type mxd_hsc1_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_hsc1_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_hsc1_open( MX_RECORD *record );

MX_API mx_status_type mxd_hsc1_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_hsc1_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_hsc1_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_hsc1_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_hsc1_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_hsc1_raw_home_command( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_hsc1_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_hsc1_motor_function_list;

extern long mxd_hsc1_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_hsc1_rfield_def_ptr;

#define MXD_HSC1_STANDARD_FIELDS \
  {-1, -1, "hsc1_interface_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HSC1_MOTOR, hsc1_interface_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "module_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HSC1_MOTOR, module_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "motor_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HSC1_MOTOR, motor_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \

#endif /* __D_HSC1_H__ */
