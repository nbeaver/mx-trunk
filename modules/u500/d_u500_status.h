/*
 * Name:    d_u500_status.h
 *
 * Purpose: Header file for reading status words from U500 motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_U500_STATUS_H__
#define __D_U500_STATUS_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *u500_record;
	long board_number;
	unsigned short status_type;
} MX_U500_STATUS;

#define MXD_U500_STATUS_STANDARD_FIELDS \
  {-1, -1, "u500_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_U500_STATUS, u500_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "board_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_STATUS, board_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "status_type", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500_STATUS, status_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_u500_status_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_u500_status_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_u500_status_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_u500_status_digital_input_function_list;

extern long mxd_u500_status_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_u500_status_rfield_def_ptr;

#endif /* __D_U500_STATUS_H__ */

