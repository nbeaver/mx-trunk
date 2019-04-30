/*
 * Name:    i_numato_gpio.h
 *
 * Purpose: Header file for Numato Lab GPIO devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NUMATO_GPIO_H__
#define __I_NUMATO_GPIO_H__

/* Values for the 'numato_gpio_flags' field. */

#define MXF_NUMATO_GPIO_DEBUG_RS232		0x1
#define MXF_NUMATO_GPIO_DEBUG_RS232_VERBOSE	0x2

/*---*/

#define MXU_NUMATO_GPIO_ID_LENGTH		20
#define MXU_NUMATO_GPIO_VERSION_LENGTH		40

/*---*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long numato_gpio_flags;

	char id[MXU_NUMATO_GPIO_ID_LENGTH+1];
	char version[MXU_NUMATO_GPIO_VERSION_LENGTH+1];
} MX_NUMATO_GPIO;

#define MXI_NUMATO_GPIO_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUMATO_GPIO, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "numato_gpio_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUMATO_GPIO, numato_gpio_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "id", MXFT_STRING, NULL, 1, {MXU_NUMATO_GPIO_ID_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUMATO_GPIO, id), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "version", MXFT_STRING, NULL, 1, {MXU_NUMATO_GPIO_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUMATO_GPIO, version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}

MX_API mx_status_type mxi_numato_gpio_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxi_numato_gpio_open( MX_RECORD *record );
MX_API mx_status_type mxi_numato_gpio_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxi_numato_gpio_command(
					MX_NUMATO_GPIO *numato_gpio,
					char *command,
					char *response,
					unsigned long max_response_length );

extern MX_RECORD_FUNCTION_LIST mxi_numato_gpio_record_function_list;
extern MX_RECORD_FUNCTION_LIST mxi_numato_gpio_record_function_list;

extern long mxi_numato_gpio_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_numato_gpio_rfield_def_ptr;

#endif /* __I_NUMATO_GPIO_H__ */

