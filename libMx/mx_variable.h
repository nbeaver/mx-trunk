/*
 * Name:    mx_variable.h
 *
 * Purpose: Header file for generic variable superclass support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#ifndef __MX_VARIABLE_H__
#define __MX_VARIABLE_H__

typedef struct {
	MX_RECORD *record; /* Pointer to the MX_RECORD structure that points
			    * to this MX_VARIABLE structure.
			    */
	mx_length_type num_dimensions;
	mx_length_type *dimension;

	void *pointer_to_value;
} MX_VARIABLE;

typedef struct {
	mx_status_type ( *send_value )( MX_VARIABLE *variable );
	mx_status_type ( *receive_value )( MX_VARIABLE *variable );
} MX_VARIABLE_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_variable_initialize_type( long record_type );

MX_API_PRIVATE mx_status_type
		mx_variable_fixup_varargs_record_field_defaults(
		MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array,
		mx_length_type num_record_fields );

MX_API mx_status_type mx_send_variable( MX_RECORD *record );
MX_API mx_status_type mx_receive_variable( MX_RECORD *record );

MX_API mx_status_type mx_get_variable_parameters( MX_RECORD *record,
					mx_length_type *num_dimensions,
					mx_length_type **dimension_array,
					long *field_type,
					void **pointer_to_value );

MX_API mx_status_type mx_get_variable_pointer( MX_RECORD *record,
						void **pointer_to_value );

/* The following functions only work for 1-dimensional array variables. */

MX_API mx_status_type mx_get_1d_array_by_name( MX_RECORD *record_list,
					char *record_name,
					long field_type,
					mx_length_type *num_elements,
					void **pointer_to_value );

MX_API mx_status_type mx_get_1d_array( MX_RECORD *record,
					long field_type,
					mx_length_type *num_elements,
					void **pointer_to_value );

MX_API mx_status_type mx_set_1d_array( MX_RECORD *record,
					long field_type,
					mx_length_type num_elements,
					void *pointer_to_value );

/*---*/

MX_API mx_status_type mx_get_int32_variable_by_name(
					MX_RECORD *record_list,
					char *record_name,
					int32_t *int32_value );

MX_API mx_status_type mx_get_uint32_variable_by_name(
					MX_RECORD *record_list,
					char *record_name,
					uint32_t *uint32_value );

MX_API mx_status_type mx_get_double_variable_by_name(
					MX_RECORD *record_list,
					char *record_name,
					double *double_value );

MX_API mx_status_type mx_get_string_variable_by_name(
					MX_RECORD *record_list,
					char *record_name,
					char *string_value,
					size_t max_string_length );

/*---*/

MX_API mx_status_type mx_get_char_variable( MX_RECORD *record,
					char *char_value );

MX_API mx_status_type mx_get_uchar_variable( MX_RECORD *record,
					unsigned char *uchar_value );

MX_API mx_status_type mx_get_int8_variable( MX_RECORD *record,
					int8_t *int8_value );

MX_API mx_status_type mx_get_uint8_variable( MX_RECORD *record,
					uint8_t *uint8_value );

MX_API mx_status_type mx_get_int16_variable( MX_RECORD *record,
					int16_t *int16_value );

MX_API mx_status_type mx_get_uint16_variable( MX_RECORD *record,
					uint16_t *uint16_value );

MX_API mx_status_type mx_get_int32_variable( MX_RECORD *record,
					int32_t *int32_value );

MX_API mx_status_type mx_get_uint32_variable( MX_RECORD *record,
					uint32_t *uint32_value );

MX_API mx_status_type mx_get_int64_variable( MX_RECORD *record,
					int64_t *int64_value );

MX_API mx_status_type mx_get_uint64_variable( MX_RECORD *record,
					uint64_t *uint64_value );

MX_API mx_status_type mx_get_float_variable( MX_RECORD *record,
					float *float_value );

MX_API mx_status_type mx_get_double_variable( MX_RECORD *record,
					double *double_value );

MX_API mx_status_type mx_get_string_variable( MX_RECORD *record,
					char *string_value,
					size_t max_string_length );

/*---*/

MX_API mx_status_type mx_set_char_variable( MX_RECORD *record,
					char char_value );

MX_API mx_status_type mx_set_uchar_variable( MX_RECORD *record,
					unsigned char uchar_value );

MX_API mx_status_type mx_set_int8_variable( MX_RECORD *record,
					int8_t int8_value );

MX_API mx_status_type mx_set_uint8_variable( MX_RECORD *record,
					uint8_t uint8_value );

MX_API mx_status_type mx_set_int16_variable( MX_RECORD *record,
					int16_t int16_value );

MX_API mx_status_type mx_set_uint16_variable( MX_RECORD *record,
					uint16_t uint16_value );

MX_API mx_status_type mx_set_int32_variable( MX_RECORD *record,
					int32_t int32_value );

MX_API mx_status_type mx_set_uint32_variable( MX_RECORD *record,
					uint32_t uint32_value );

MX_API mx_status_type mx_set_int64_variable( MX_RECORD *record,
					int64_t int64_value );

MX_API mx_status_type mx_set_uint64_variable( MX_RECORD *record,
					uint64_t uint64_value );

MX_API mx_status_type mx_set_float_variable( MX_RECORD *record,
					float float_value );

MX_API mx_status_type mx_set_double_variable( MX_RECORD *record,
					double double_value );

MX_API mx_status_type mx_set_string_variable( MX_RECORD *record,
					char *string_value );

/*---*/

/* The following definitions of MXA_XXX_SIZEOF assume that
 * MXU_FIELD_MAX_DIMENSIONS is equal to 8.
 */

#define MXA_STRING_SIZEOF \
	{ sizeof(char), sizeof(char *), \
	sizeof(char **), sizeof(char ***), \
	sizeof(char ****), sizeof(char *****), \
	sizeof(char ******), sizeof(char *******) }

#define MXA_INT8_SIZEOF \
	{ sizeof(int8_t), sizeof(int8_t *), \
	sizeof(int8_t **), sizeof(int8_t ***), \
	sizeof(int8_t ****), sizeof(int8_t *****), \
	sizeof(int8_t ******), sizeof(int8_t *******) }

#define MXA_UINT8_SIZEOF \
	{ sizeof(uint8_t), sizeof(uint8_t *), \
	sizeof(uint8_t **), sizeof(uint8_t ***), \
	sizeof(uint8_t ****), sizeof(uint8_t *****), \
	sizeof(uint8_t ******), sizeof(uint8_t *******) }

#define MXA_INT16_SIZEOF \
	{ sizeof(int16_t), sizeof(int16_t *), \
	sizeof(int16_t **), sizeof(int16_t ***), \
	sizeof(int16_t ****), sizeof(int16_t *****), \
	sizeof(int16_t ******), sizeof(int16_t *******) }

#define MXA_UINT16_SIZEOF \
	{ sizeof(uint16_t), sizeof(uint16_t *), \
	sizeof(uint16_t **), sizeof(uint16_t ***), \
	sizeof(uint16_t ****), sizeof(uint16_t *****), \
	sizeof(uint16_t ******), sizeof(uint16_t *******) }

#define MXA_INT32_SIZEOF \
	{ sizeof(int32_t), sizeof(int32_t *), \
	sizeof(int32_t **), sizeof(int32_t ***), \
	sizeof(int32_t ****), sizeof(int32_t *****), \
	sizeof(int32_t ******), sizeof(int32_t *******) }

#define MXA_UINT32_SIZEOF \
	{ sizeof(uint32_t), sizeof(uint32_t *), \
	sizeof(uint32_t **), sizeof(uint32_t ***), \
	sizeof(uint32_t ****), sizeof(uint32_t *****), \
	sizeof(uint32_t ******), sizeof(uint32_t *******) }

#define MXA_INT64_SIZEOF \
	{ sizeof(int64_t), sizeof(int64_t *), \
	sizeof(int64_t **), sizeof(int64_t ***), \
	sizeof(int64_t ****), sizeof(int64_t *****), \
	sizeof(int64_t ******), sizeof(int64_t *******) }

#define MXA_UINT64_SIZEOF \
	{ sizeof(uint64_t), sizeof(uint64_t *), \
	sizeof(uint64_t **), sizeof(uint64_t ***), \
	sizeof(uint64_t ****), sizeof(uint64_t *****), \
	sizeof(uint64_t ******), sizeof(uint64_t *******) }

#define MXA_FLOAT_SIZEOF \
	{ sizeof(float), sizeof(float *), \
	sizeof(float **), sizeof(float ***), \
	sizeof(float ****), sizeof(float *****), \
	sizeof(float ******), sizeof(float *******) }

#define MXA_DOUBLE_SIZEOF \
	{ sizeof(double), sizeof(double *), \
	sizeof(double **), sizeof(double ***), \
	sizeof(double ****), sizeof(double *****), \
	sizeof(double ******), sizeof(double *******) }

#define MXA_HEX_SIZEOF      MXA_UINT32_SIZEOF

#define MXA_CHAR_SIZEOF     MXA_STRING_SIZEOF

#define MXA_UCHAR_SIZEOF \
	{ sizeof(unsigned char), sizeof(unsigned char *), \
	sizeof(unsigned char **), sizeof(unsigned char ***), \
	sizeof(unsigned char ****), sizeof(unsigned char *****), \
	sizeof(unsigned char ******), sizeof(unsigned char *******) }

#define MXA_RECORD_SIZEOF \
	{ sizeof(MX_RECORD *), sizeof(MX_RECORD **), \
	sizeof(MX_RECORD ***), sizeof(MX_RECORD ****), \
	sizeof(MX_RECORD *****), sizeof(MX_RECORD ******), \
	sizeof(MX_RECORD *******), sizeof(MX_RECORD ********) }

/* =========================================== */

#define MXLV_VAR_VALUE		1001

#define MX_VARIABLE_STANDARD_FIELDS \
  {-1, -1, "num_dimensions", MXFT_LENGTH, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, num_dimensions), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY)}, \
\
  {-1, -1, "dimension", MXFT_LENGTH, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, dimension), \
	{sizeof(mx_length_type)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_VARARGS | MXFF_READ_ONLY) }


/* =========================================== */

#define MX_STRING_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_STRING, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_STRING_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_CHAR_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_CHAR, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_CHAR_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_UCHAR_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_UCHAR, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_UCHAR_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_INT8_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_INT8, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_INT8_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_UINT8_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_UINT8, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_UINT8_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_INT16_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_INT16, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_INT16_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_UINT16_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_UINT16, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_UINT16_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_INT32_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_INT32, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_INT32_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_UINT32_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_UINT32, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_UINT32_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_INT64_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_INT64, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_INT64_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_UINT64_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_UINT64, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_UINT64_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_FLOAT_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_FLOAT, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_FLOAT_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_DOUBLE_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_DOUBLE, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_DOUBLE_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#define MX_RECORD_VARIABLE_STANDARD_FIELDS \
  {MXLV_VAR_VALUE, -1, "value", MXFT_RECORD, NULL, \
	MXU_VARARGS_LENGTH, {MXU_VARARGS_LENGTH}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof(MX_VARIABLE, pointer_to_value), \
	MXA_RECORD_SIZEOF, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS) }

#endif /* __MX_VARIABLE_H__ */

