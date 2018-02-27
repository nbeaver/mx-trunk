/*
 * Name:    i_labjack_ux.h
 *
 * Purpose: Header for the LabJack U3, U6, and UE9 devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LABJACK_UX_H__
#define __I_LABJACK_UX_H__

/*----*/

#include "labjackusb.h"		/* Vendor include file */

/*----*/

#if defined(OS_WIN32)
#  define MX_LABJACK_UX_HANDLE			LJ_HANDLE
#else
#  define MX_LABJACK_UX_HANDLE			HANDLE
#endif

#define MXU_LABJACK_UX_PRODUCT_NAME_LENGTH	20
/*----*/

/* Values for the 'labjack_flags' field. */

#define MXF_LABJACK_UX_SHOW_INFO		0x1

/*----*/

typedef struct {
	uint8_t eio_analog;
	uint8_t fio_analog;
	mx_bool_type uart_enable;
	mx_bool_type dac1_enable;
	uint8_t timer_counter_enable;
	int8_t timer_counter_pin_offset;
} MX_LABJACK_U3_CONFIG;

typedef struct {
	int dummy;
} MX_LABJACK_U6_CONFIG;

typedef struct {
	int dummy;
} MX_LABJACK_UE9_CONFIG;

/*----*/

typedef struct {
	MX_RECORD *record;

	char product_name[ MXU_LABJACK_UX_PRODUCT_NAME_LENGTH+1 ];
	unsigned long device_number;
	unsigned long labjack_flags;

	MX_LABJACK_UX_HANDLE handle;
	unsigned long product_id;

	union {
		MX_LABJACK_U3_CONFIG u3_config;
		MX_LABJACK_U6_CONFIG u6_config;
		MX_LABJACK_UE9_CONFIG ue9_confit;
	} u;

	long timeout_msec;	/* milliseconds */
} MX_LABJACK_UX;

#define MXI_LABJACK_UX_STANDARD_FIELDS \
  {-1, -1, "product_name", MXFT_STRING, NULL, \
			1, {MXU_LABJACK_UX_PRODUCT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABJACK_UX, product_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "device_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABJACK_UX, device_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "labjack_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABJACK_UX, labjack_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

/*-----*/

MX_API mx_status_type mxi_labjack_ux_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_labjack_ux_finish_record_initialization(
						MX_RECORD *record );

MX_API mx_status_type mxi_labjack_ux_open( MX_RECORD *record );

MX_API mx_status_type mxi_labjack_ux_close( MX_RECORD *record );

MX_API mx_status_type mxi_labjack_ux_finish_delayed_initialization(
						MX_RECORD *record );

/*-----*/

MX_API_PRIVATE mx_status_type mxi_labjack_ux_read(
					MX_LABJACK_UX *labjack_ux,
					uint8_t *buffer,
					unsigned long num_bytes_to_read,
					long timeout_msec );

MX_API_PRIVATE mx_status_type mxi_labjack_ux_write(
					MX_LABJACK_UX *labjack_ux,
					uint8_t *buffer,
					unsigned long num_bytes_to_write,
					long timeout_msec );

MX_API_PRIVATE mx_status_type mxi_labjack_ux_command(
					MX_LABJACK_UX *labjack_ux,
					uint8_t *command,
					unsigned long command_length,
					uint8_t *response,
					unsigned long max_response_length,
					long timeout_msec );

/*-----*/

extern MX_RECORD_FUNCTION_LIST mxi_labjack_ux_record_function_list;

extern long mxi_labjack_ux_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_labjack_ux_rfield_def_ptr;

#endif /* __I_LABJACK_UX_H__ */

