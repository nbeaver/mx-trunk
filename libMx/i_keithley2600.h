/*
 * Name:    i_keithley2600.h
 *
 * Purpose: Header file for the Keithley 2600 series of SourceMeters.
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

#ifndef __I_KEITHLEY2600_H__
#define __I_KEITHLEY2600_H__

/* Values for the 'keithley2600_flags' field. */

#define MXF_KEITHLEY2600_DEBUG_RS232			0x1

/*---*/

#define MXU_KEITHLEY2600_COMMAND_LENGTH			80
#define MXU_KEITHLEY2600_RESPONSE_LENGTH		80

#define MXU_KEITHLEY2600_SIGNAL_TYPE_NAME_LENGTH	40

#define MXU_KEITHLEY2600_MODEL_NAME_LENGTH		20
#define MXU_KEITHLEY2600_SERIAL_NUMBER_LENGTH		20
#define MXU_KEITHLEY2600_FIRMWARE_VERSION_LENGTH	20

/*---*/

typedef struct {
	MX_RECORD *record;

	char rs232_record_name[MXU_RECORD_NAME_LENGTH+1];
	unsigned long keithley2600_flags;

	MX_RECORD *rs232_record;
	char model_name[MXU_KEITHLEY2600_MODEL_NAME_LENGTH+1];
	char serial_number[MXU_KEITHLEY2600_SERIAL_NUMBER_LENGTH+1];
	char firmware_version[MXU_KEITHLEY2600_FIRMWARE_VERSION_LENGTH+1];
} MX_KEITHLEY2600;

#define MXI_KEITHLEY2600_STANDARD_FIELDS \
  {-1, -1, "rs232_record_name", MXFT_STRING, NULL, \
	  			1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2600, rs232_record_name), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "keithley2600_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2600, keithley2600_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "model_name", MXFT_STRING, NULL, \
	  			1, {MXU_KEITHLEY2600_MODEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2600, model_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "serial_number", MXFT_STRING, NULL, \
	  			1, {MXU_KEITHLEY2600_SERIAL_NUMBER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2600, serial_number), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "firmware_version", MXFT_STRING, NULL, \
	  			1, {MXU_KEITHLEY2600_FIRMWARE_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2600, firmware_version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}

MX_API mx_status_type mxi_keithley2600_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxi_keithley2600_open( MX_RECORD *record );
MX_API mx_status_type mxi_keithley2600_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxi_keithley2600_command(
				MX_KEITHLEY2600 *keithley2600,
				char *command,
				char *response,
				unsigned long max_response_length,
				unsigned long keithley2600_flags );

extern MX_RECORD_FUNCTION_LIST mxi_keithley2600_record_function_list;
extern MX_RECORD_FUNCTION_LIST mxi_keithley2600_record_function_list;

extern long mxi_keithley2600_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_keithley2600_rfield_def_ptr;

#endif /* __I_KEITHLEY2600_H__ */

