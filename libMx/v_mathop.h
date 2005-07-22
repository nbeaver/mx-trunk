/*
 * Name:    v_mathop.h
 *
 * Purpose: Header file for simple mathematical operations on a list of records.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_MATHOP_H__
#define __V_MATHOP_H__

/* Mathop operation types. */

#define MXF_MATHOP_ADD			1
#define MXF_MATHOP_SUBTRACT		2
#define MXF_MATHOP_MULTIPLY		3
#define MXF_MATHOP_DIVIDE		4
#define MXF_MATHOP_NEGATE		5

#define MXF_MATHOP_SQUARE		11
#define MXF_MATHOP_SQUARE_ROOT		12

#define MXF_MATHOP_SINE			101
#define MXF_MATHOP_COSINE		102
#define MXF_MATHOP_TANGENT		103

#define MXF_MATHOP_ARC_SINE		111
#define MXF_MATHOP_ARC_COSINE		112
#define MXF_MATHOP_ARC_TANGENT		113

#define MXF_MATHOP_EXPONENTIAL		121
#define MXF_MATHOP_LOGARITHM		122
#define MXF_MATHOP_LOG10		123

#define MXU_MATHOP_NAME_LENGTH	40

/* mathop_flags bitmasks. */

#define MXF_MATHOP_READ_ONLY			0x1
#define MXF_MATHOP_WRITE_ONLY_REDEFINES_VALUE	0x2

typedef struct {
	long num_items;
	long item_to_change;
	unsigned long mathop_flags;
	char operation_name[ MXU_MATHOP_NAME_LENGTH + 1 ];
	char **item_array;

	int operation_type;
	MX_RECORD **record_array;
	double *value_array;

	int has_been_initialized;
} MX_MATHOP_VARIABLE;

#define MX_MATHOP_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "num_items", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MATHOP_VARIABLE, num_items), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "item_to_change", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MATHOP_VARIABLE, item_to_change), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "mathop_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MATHOP_VARIABLE, mathop_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "operation_name", MXFT_STRING, NULL, 1, {MXU_MATHOP_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MATHOP_VARIABLE, operation_name), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "item_array", MXFT_STRING, \
			NULL, 2, {MXU_VARARGS_LENGTH, MXU_MATHOP_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MATHOP_VARIABLE, item_array), \
	{sizeof(char), sizeof(char *)}, NULL, \
				(MXFF_IN_DESCRIPTION | MXFF_VARARGS ) }

MX_API_PRIVATE mx_status_type mxv_mathop_initialize_type( long );
MX_API_PRIVATE mx_status_type mxv_mathop_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_mathop_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_mathop_delete_record( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_mathop_dummy_function( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_mathop_send_variable( MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_mathop_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_mathop_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_mathop_variable_function_list;

extern long mxv_mathop_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_mathop_rfield_def_ptr;

#endif /* __V_MATHOP_H__ */

