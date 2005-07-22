/*
 * Name:    i_pdi45.h
 *
 * Purpose: Header for MX driver for Prairie Digital, Inc. Model 45
 *          data acquisition and control module.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PDI45_H__
#define __I_PDI45_H__

/* Define the data structures used by a PDI Model 45 interface. */

#define MX_PDI45_NUM_DIGITAL_CHANNELS	8

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long io_type[ MX_PDI45_NUM_DIGITAL_CHANNELS ];

	int version;
	int four_step_mode;
	int turn_around_delay;
	int communication_timer;
} MX_PDI45;

#define MXI_PDI45_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "io_type", MXFT_HEX, NULL, 1, {MX_PDI45_NUM_DIGITAL_CHANNELS}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI45, io_type), \
	{sizeof(unsigned long)}, NULL, MXFF_READ_ONLY}

MX_API mx_status_type mxi_pdi45_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_pdi45_open( MX_RECORD *record );

MX_API mx_status_type mxi_pdi45_command( MX_PDI45 *pdi45, char *command,
				char *response, size_t response_length );

extern MX_RECORD_FUNCTION_LIST mxi_pdi45_record_function_list;

extern long mxi_pdi45_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_pdi45_rfield_def_ptr;

/**************************************************************************/

/* Error codes returned by the Model 45 (from page 15). */

#define MX_PDI45_ERR_POWER_UP_CLEAR_EXPECTED		0x00
#define MX_PDI45_ERR_UNDEFINED_COMMAND			0x01
#define MX_PDI45_ERR_CHECKSUM_ERROR			0x02
#define MX_PDI45_ERR_INPUT_BUFFER_OVERFLOW		0x03
#define MX_PDI45_ERR_NON_PRINTABLE_CHARACTER_RECEIVED	0x04
#define MX_PDI45_ERR_DATA_FIELD_ERROR			0x05
#define MX_PDI45_ERR_COMMUNICATION_TIME_OUT_ERROR	0x06
#define MX_PDI45_ERR_INVALID_SPECIFIED_VALUE		0x07
#define MX_PDI45_ERR_MODULE_INTERFACE_ERROR		0x10

/* Communication configuration bit patterns (from page 18). */

#define MX_PDI45_COM_7_DATA_BITS		0x1
#define MX_PDI45_COM_PARITY_ENABLED	0x2
#define MX_PDI45_COM_ODD_PARITY		0x4

#define MX_PDI45_COM_300_BAUD		0x0
#define MX_PDI45_COM_600_BAUD		0x1
#define MX_PDI45_COM_1200_BAUD		0x2
#define MX_PDI45_COM_2400_BAUD		0x3
#define MX_PDI45_COM_4800_BAUD		0x4
#define MX_PDI45_COM_9600_BAUD		0x5
#define MX_PDI45_COM_19200_BAUD		0x6
#define MX_PDI45_COM_38400_BAUD		0x7
#define MX_PDI45_COM_57600_BAUD		0x8
#define MX_PDI45_COM_115200_BAUD	0x9

/* Digital I/O type mask.  To see the details of the individual
 * types, see pages 21 and 22 of the manual.  The descriptions
 * of their distinctions are rather long to encapsulate in 
 * a #define statement.
 */

#define MX_PDI45_OUTPUT_TYPE_MASK	0x80

/* Type definitions for setting the digital I/O type. */

#define MX_PDI45_INPUT_TYPE	0x00
#define MX_PDI45_OUTPUT_TYPE	0x80

/* Miscellaneous definitions. */

#define MX_PDI45_COMMAND_LENGTH	50

/* PDI Model 45 interface functions. */

#endif /* __I_PDI45_H__ */

