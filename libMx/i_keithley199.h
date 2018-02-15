/*
 * Name:    i_keithley199.h
 *
 * Purpose: Header file for MX driver for Keithley 199 series dmm/scanners.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_KEITHLEY199_H__
#define __I_KEITHLEY199_H__

#define MXU_KEITHLEY199_FIRMWARE_VERSION_LENGTH	20

/* Values for 'keithley_flags'.
 *
 * All of the flag values below are to cope with the fact that
 * communication with a Keithley 199 is extremely slow.
 */

#define MXF_KEITHLEY199_BYPASS_ERROR_CHECK      0x1
#define MXF_KEITHLEY199_BYPASS_GET_STATUS       0x2
#define MXF_KEITHLEY199_BYPASS_STARTUP_SET      0x4

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE gpib_interface;
	unsigned long keithley_flags;

	char firmware_version[MXU_KEITHLEY199_FIRMWARE_VERSION_LENGTH+1];

	long last_measurement_type;
} MX_KEITHLEY199;

/* 'measurement_type' values. */

#define MXT_KEITHLEY199_INVALID	(-1)
#define MXT_KEITHLEY199_UNKNOWN	0
#define MXT_KEITHLEY199_DCV		1
#define MXT_KEITHLEY199_ACV		2
#define MXT_KEITHLEY199_DCI		3
#define MXT_KEITHLEY199_ACI		4
#define MXT_KEITHLEY199_OHMS_2		5
#define MXT_KEITHLEY199_OHMS_4		6
#define MXT_KEITHLEY199_FREQ		7
#define MXT_KEITHLEY199_PERIOD		8
#define MXT_KEITHLEY199_TEMP		9
#define MXT_KEITHLEY199_CONT		10
#define MXT_KEITHLEY199_DIODE		11

MX_API mx_status_type mxi_keithley199_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley199_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley199_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley199_open( MX_RECORD *record );
MX_API mx_status_type mxi_keithley199_close( MX_RECORD *record );

MX_API mx_status_type mxi_keithley199_command( MX_KEITHLEY199 *keithley199,
		char *command, char *response, int max_response_length,
		int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_keithley199_record_function_list;

extern long mxi_keithley199_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_keithley199_rfield_def_ptr;

#define MXI_KEITHLEY199_STANDARD_FIELDS \
  {-1, -1, "gpib_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY199, gpib_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "keithley_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY199, keithley_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "firmware_version", MXFT_STRING, \
			NULL, 1, {MXU_KEITHLEY199_FIRMWARE_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY199, firmware_version), \
	{0}, NULL, 0 }

#endif /* __I_KEITHLEY199_H__ */
