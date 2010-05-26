/*
 * Name:    i_linkam_t9x.h
 *
 * Purpose: Header file for the Linkam T92, T93, T94, and T95 series
 *          of temperature controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LINKAM_T9X_H__
#define __I_LINKAM_T9X_H__

#define MXT_LINKAM_T9X_IS_T92		92
#define MXT_LINKAM_T9X_IS_T93		93
#define MXT_LINKAM_T9X_IS_T94		94
#define MXT_LINKAM_T9X_IS_T95		95

#define MXU_LINKAM_T9X_MODULE_NAME_LENGTH    20

/*---*/

typedef struct {
	MX_RECORD *record;
	MX_RECORD *rs232_record;
	unsigned char status_byte;
	unsigned char error_byte;
	unsigned char pump_byte;
	unsigned char general_status;
	double temperature;

	long module_type;
	char module_type_name[MXU_LINKAM_T9X_MODULE_NAME_LENGTH+1];

	mx_bool_type moves_are_relative;
} MX_LINKAM_T9X;

#define MXI_LINKAM_T9X_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINKAM_T9X, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_type_name", MXFT_STRING, NULL, \
				1, {MXU_LINKAM_T9X_MODULE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINKAM_T9X, module_type_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_linkam_t9x_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_linkam_t9x_open( MX_RECORD *record );

MX_API mx_status_type mxi_linkam_t9x_command(
					MX_LINKAM_T9X *linkam_t9x,
					char *command,
					char *response,
					size_t response_buffer_length,
					mx_bool_type debug_flag );

MX_API mx_status_type mxi_linkam_t9x_get_status( MX_LINKAM_T9X *linkam_t9x,
						mx_bool_type debug_flag );

MX_API mx_status_type mxi_linkam_t9x_set_motor_status_from_error_byte(
						MX_LINKAM_T9X *linkam_t9x,
						MX_RECORD *motor_record );

extern MX_RECORD_FUNCTION_LIST mxi_linkam_t9x_record_function_list;

extern long mxi_linkam_t9x_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_linkam_t9x_rfield_def_ptr;

#endif /* __I_LINKAM_T9X_H__ */

