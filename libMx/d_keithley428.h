/*
 * Name:    d_keithley428.h
 *
 * Purpose: Header file for MX driver for Keithley 428 current amplifiers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006, 2017, 2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KEITHLEY428_H__
#define __D_KEITHLEY428_H__

#include "mx_amplifier.h"

#define MXU_KEITHLEY428_FIRMWARE_VERSION_LENGTH 20

/* Values for 'keithley_flags'.
 *
 * All of the flag values below are to cope with the fact that
 * communication with a Keithley 428 is extremely slow.
 */

#define MXF_KEITHLEY428_BYPASS_ERROR_CHECK	0x1
#define MXF_KEITHLEY428_BYPASS_GET_STATUS	0x2
#define MXF_KEITHLEY428_BYPASS_STARTUP_SET	0x4

#define MXF_KEITHLEY428_DEBUG			0x1000

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE gpib_interface;
	unsigned long keithley_flags;

	char firmware_version[MXU_KEITHLEY428_FIRMWARE_VERSION_LENGTH+1];

	int bypass_get_gain_or_offset;		/* Private field. */
} MX_KEITHLEY428;

MX_API mx_status_type mxd_keithley428_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_keithley428_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_keithley428_open( MX_RECORD *record );
MX_API mx_status_type mxd_keithley428_close( MX_RECORD *record );

MX_API mx_status_type mxd_keithley428_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley428_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley428_get_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley428_set_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley428_get_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley428_set_time_constant(
						MX_AMPLIFIER *amplifier );

MX_API mx_status_type mxd_keithley428_command( MX_KEITHLEY428 *keithley428,
		char *command, char *response, int response_buffer_length,
		int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxd_keithley428_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_keithley428_amplifier_function_list;

extern long mxd_keithley428_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_keithley428_rfield_def_ptr;

#define MXD_KEITHLEY428_STANDARD_FIELDS \
  {-1, -1, "gpib_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY428, gpib_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "keithley_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY428, keithley_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "firmware_version", MXFT_STRING, \
  			NULL, 1, {MXU_KEITHLEY428_FIRMWARE_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY428, firmware_version), \
	{0}, NULL, 0 }

#endif /* __D_KEITHLEY428_H__ */
