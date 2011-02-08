/*
 * Name:    d_pleora_iport_dio.h
 *
 * Purpose: MX digital input and output drivers for a Pleora iPORT
 *          video capture device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PLEORA_IPORT_DIO_H__
#define __D_PLEORA_IPORT_DIO_H__

#ifdef __cplusplus

#define MXU_PLEORA_IPORT_DIO_NAME_LENGTH	4

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pleora_iport_vinput_record;
	char line_name[ MXU_PLEORA_IPORT_DIO_NAME_LENGTH+1 ];

	unsigned long line_id;

} MX_PLEORA_IPORT_DIGITAL_INPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pleora_iport_vinput_record;
	char line_name[ MXU_PLEORA_IPORT_DIO_NAME_LENGTH+1 ];

	unsigned long line_id;

} MX_PLEORA_IPORT_DIGITAL_OUTPUT;

#define MXD_PLEORA_IPORT_DINPUT_STANDARD_FIELDS \
  {-1, -1, "pleora_iport_vinput_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	 offsetof(MX_PLEORA_IPORT_DIGITAL_INPUT, pleora_iport_vinput_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "line_name", MXFT_STRING, NULL, \
			1, {MXU_PLEORA_IPORT_DIO_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_PLEORA_IPORT_DIGITAL_INPUT, line_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#define MXD_PLEORA_IPORT_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "pleora_iport_vinput_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	 offsetof(MX_PLEORA_IPORT_DIGITAL_OUTPUT, pleora_iport_vinput_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "line_name", MXFT_STRING, NULL, \
			1, {MXU_PLEORA_IPORT_DIO_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_PLEORA_IPORT_DIGITAL_OUTPUT, line_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

/* Input interface functions. */

MX_API mx_status_type mxd_pleora_iport_dinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pleora_iport_dinput_open( MX_RECORD *record );

MX_API mx_status_type mxd_pleora_iport_dinput_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_pleora_iport_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_pleora_iport_dinput_digital_input_function_list;

extern long mxd_pleora_iport_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pleora_iport_dinput_rfield_def_ptr;

/* Output interface functions. */

MX_API mx_status_type mxd_pleora_iport_doutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pleora_iport_doutput_open( MX_RECORD *record );

MX_API mx_status_type mxd_pleora_iport_doutput_write(
					MX_DIGITAL_OUTPUT *voutput );

extern MX_RECORD_FUNCTION_LIST mxd_pleora_iport_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_pleora_iport_doutput_digital_output_function_list;

extern long mxd_pleora_iport_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pleora_iport_doutput_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_PLEORA_IPORT_DIO_H__ */

