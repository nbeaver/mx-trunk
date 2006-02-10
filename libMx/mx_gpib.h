/*
 * Name:    mx_gpib.h
 *
 * Purpose: General header file for GPIB interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_GPIB_H__
#define __MX_GPIB_H__

#include "mx_record.h"

#define MX_NUM_GPIB_ADDRESSES		31

#define MX_GPIB_DEFAULT_BUFFER_LENGTH	1000

/* 'gpib_flags' field bit definitions. */

#define MXF_GPIB_NO_REMOTE_ACCESS	0x80000000

/* ======== GPIB interface flag values ======== */

#define MXF_GPIB_DEBUG		0x1
#define MXF_GPIB_NOWAIT		0x2

/* ======== Generic GPIB interface data structure ======= */

typedef struct {
	MX_RECORD *record;

	double default_io_timeout;			/* in seconds */
	int32_t default_eoi_mode;
	mx_hex_type default_read_terminator;
	mx_hex_type default_write_terminator;
	mx_hex_type gpib_flags;

	int32_t open_semaphore[MX_NUM_GPIB_ADDRESSES];
	int32_t secondary_address[MX_NUM_GPIB_ADDRESSES];

	double io_timeout[MX_NUM_GPIB_ADDRESSES];	/* in seconds */
	int32_t eoi_mode[MX_NUM_GPIB_ADDRESSES];
	mx_hex_type read_terminator[MX_NUM_GPIB_ADDRESSES];
	mx_hex_type write_terminator[MX_NUM_GPIB_ADDRESSES];

	int32_t address;

	char *read_buffer;
	char *write_buffer;
	mx_length_type read_buffer_length;
	mx_length_type write_buffer_length;
	int32_t bytes_to_read;
	int32_t bytes_to_write;
	int32_t bytes_read;
	int32_t bytes_written;

	mx_bool_type ascii_read;
	mx_bool_type ascii_write;

	int32_t open_device;
	int32_t close_device;
	mx_bool_type interface_clear;
	mx_bool_type device_clear;
	int32_t selective_device_clear;
	mx_bool_type local_lockout;
	int32_t remote_enable;
	int32_t go_to_local;
	int32_t trigger;
	double wait_for_service_request;
	uint8_t serial_poll;
	mx_bool_type serial_poll_disable;
} MX_GPIB;

#define MXLV_GPIB_OPEN_DEVICE			101
#define MXLV_GPIB_CLOSE_DEVICE			102
#define MXLV_GPIB_READ				103
#define MXLV_GPIB_WRITE				104
#define MXLV_GPIB_GETLINE			105
#define MXLV_GPIB_PUTLINE			106
#define MXLV_GPIB_INTERFACE_CLEAR		107
#define MXLV_GPIB_DEVICE_CLEAR			108
#define MXLV_GPIB_SELECTIVE_DEVICE_CLEAR	109
#define MXLV_GPIB_LOCAL_LOCKOUT			110
#define MXLV_GPIB_REMOTE_ENABLE			111
#define MXLV_GPIB_GO_TO_LOCAL			112
#define MXLV_GPIB_TRIGGER			113
#define MXLV_GPIB_WAIT_FOR_SERVICE_REQUEST	114
#define MXLV_GPIB_SERIAL_POLL			115
#define MXLV_GPIB_SERIAL_POLL_DISABLE		116

#define MX_GPIB_STANDARD_FIELDS \
  {-1, -1, "default_io_timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, default_io_timeout), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_eoi_mode", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, default_eoi_mode), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_read_terminator", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, default_read_terminator), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_write_terminator", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, default_write_terminator), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "gpib_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, gpib_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "open_semaphore", MXFT_INT32, NULL, 1, {MX_NUM_GPIB_ADDRESSES}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, open_semaphore), \
	{sizeof(int)}, NULL, 0}, \
  \
  {-1, -1, "secondary_address", MXFT_INT32, NULL, 1, {MX_NUM_GPIB_ADDRESSES}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, secondary_address), \
	{sizeof(int)}, NULL, 0}, \
  \
  {-1, -1, "io_timeout", MXFT_DOUBLE, NULL, 1, {MX_NUM_GPIB_ADDRESSES}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, io_timeout), \
	{sizeof(int)}, NULL, 0}, \
  \
  {-1, -1, "eoi_mode", MXFT_INT32, NULL, 1, {MX_NUM_GPIB_ADDRESSES}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, eoi_mode), \
	{sizeof(int)}, NULL, 0}, \
  \
  {-1, -1, "read_terminator", MXFT_HEX, NULL, 1, {MX_NUM_GPIB_ADDRESSES}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, read_terminator), \
	{sizeof(unsigned long)}, NULL, 0}, \
  \
  {-1, -1, "write_terminator", MXFT_HEX, NULL, 1, {MX_NUM_GPIB_ADDRESSES}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, write_terminator), \
	{sizeof(unsigned long)}, NULL, 0}, \
  \
  {-1, -1, "address", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, address), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "read_buffer_length", MXFT_LENGTH, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, read_buffer_length), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "write_buffer_length", MXFT_LENGTH, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, write_buffer_length), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "bytes_to_read", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, bytes_to_read), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "bytes_to_write", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, bytes_to_write), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "bytes_read", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, bytes_read), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "bytes_written", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, bytes_written), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_GPIB_OPEN_DEVICE, -1, "open_device", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, open_device), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_CLOSE_DEVICE, -1, "close_device", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, close_device), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_READ, -1, "read", MXFT_STRING, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, read_buffer), \
	{sizeof(char)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_GPIB_WRITE, -1, "write", MXFT_STRING, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, write_buffer), \
	{sizeof(char)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_GPIB_GETLINE, -1, "getline", MXFT_STRING, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, read_buffer), \
	{sizeof(char)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_GPIB_PUTLINE, -1, "putline", MXFT_STRING, NULL, 1, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, write_buffer), \
	{sizeof(char)}, NULL, MXFF_VARARGS}, \
  \
  {MXLV_GPIB_INTERFACE_CLEAR, -1, "interface_clear", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, interface_clear), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_DEVICE_CLEAR, -1, "device_clear", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, device_clear), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_SELECTIVE_DEVICE_CLEAR, -1, "selective_device_clear", \
	  					MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, selective_device_clear), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_LOCAL_LOCKOUT, -1, "local_lockout", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, local_lockout), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_REMOTE_ENABLE, -1, "remote_enable", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, remote_enable), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_GO_TO_LOCAL, -1, "go_to_local", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, go_to_local), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_TRIGGER, -1, "trigger", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, trigger), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_WAIT_FOR_SERVICE_REQUEST, -1, "wait_for_service_request", \
	  					MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, wait_for_service_request), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_SERIAL_POLL, -1, "serial_poll", MXFT_UINT8, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, serial_poll), \
	{0}, NULL, 0}, \
  \
  {MXLV_GPIB_SERIAL_POLL_DISABLE, -1, "serial_poll_disable", \
	  					MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_GPIB, serial_poll_disable), \
	{0}, NULL, 0}

/*
 * The structure type MX_GPIB_FUNCTION_LIST contains a list of
 * pointers to functions that vary from interface type to
 * interface type.
 */

typedef struct {
	mx_status_type ( *open_device ) ( MX_GPIB *gpib,
					int32_t address );

	mx_status_type ( *close_device ) ( MX_GPIB *gpib,
					int32_t address );

	mx_status_type ( *read ) ( MX_GPIB *gpib,
					int32_t address,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read,
					mx_hex_type flags);

	mx_status_type ( *write ) ( MX_GPIB *gpib,
					int32_t address,
					char *buffer,
					size_t bytes_to_write,
					size_t *bytes_written,
					mx_hex_type flags);

	mx_status_type ( *interface_clear ) ( MX_GPIB *gpib );

	mx_status_type ( *device_clear ) ( MX_GPIB *gpib );

	mx_status_type ( *selective_device_clear ) ( MX_GPIB *gpib,
					int32_t address );

	mx_status_type ( *local_lockout ) ( MX_GPIB *gpib );

	mx_status_type ( *remote_enable ) ( MX_GPIB *gpib, int32_t address );

	mx_status_type ( *go_to_local ) ( MX_GPIB *gpib, int32_t address );

	mx_status_type ( *trigger ) ( MX_GPIB *gpib,
					int32_t address );

	mx_status_type ( *wait_for_service_request ) ( MX_GPIB *gpib,
					double timeout );

	mx_status_type ( *serial_poll ) ( MX_GPIB *gpib,
					int32_t address,
					uint8_t *serial_poll_byte );

	mx_status_type ( *serial_poll_disable ) ( MX_GPIB *gpib );
} MX_GPIB_FUNCTION_LIST;

/* ============== Initialization function. ============== */

MX_API_PRIVATE mx_status_type mx_gpib_finish_record_initialization(
				MX_RECORD *gpib_record );

MX_API_PRIVATE mx_status_type mx_gpib_setup_communication_buffers(
				MX_RECORD *gpib_record );

MX_API_PRIVATE mx_status_type mx_gpib_get_pointers( MX_RECORD *gpib_record,
				MX_GPIB **gpib,
				MX_GPIB_FUNCTION_LIST **function_list_ptr,
				const char *calling_fname );

/* ============== Interface function prototypes. ============== */

MX_API mx_status_type mx_gpib_open_device( MX_RECORD *gpib_record,
					int32_t address );

MX_API mx_status_type mx_gpib_close_device( MX_RECORD *gpib_record,
					int32_t address );

MX_API mx_status_type mx_gpib_read( MX_RECORD *gpib_record,
					int32_t address,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read,
					mx_hex_type flags );

MX_API mx_status_type mx_gpib_write( MX_RECORD *gpib_record,
					int32_t address,
					char *buffer,
					size_t bytes_to_write,
					size_t *bytes_written,
					mx_hex_type flags );

MX_API mx_status_type mx_gpib_getline( MX_RECORD *gpib_record,
					int32_t address,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read,
					mx_hex_type flags );

MX_API mx_status_type mx_gpib_putline( MX_RECORD *gpib_record,
					int32_t address,
					char *buffer,
					size_t *bytes_written,
					mx_hex_type flags );

MX_API mx_status_type mx_gpib_interface_clear( MX_RECORD *gpib_record );

MX_API mx_status_type mx_gpib_device_clear( MX_RECORD *gpib_record );

MX_API mx_status_type mx_gpib_selective_device_clear( MX_RECORD *gpib_record,
					int32_t address );

MX_API mx_status_type mx_gpib_local_lockout( MX_RECORD *gpib_record );

MX_API mx_status_type mx_gpib_remote_enable( MX_RECORD *gpib_record,
					int32_t address );

MX_API mx_status_type mx_gpib_go_to_local( MX_RECORD *gpib_record,
					int32_t address );

MX_API mx_status_type mx_gpib_trigger( MX_RECORD *gpib_record,
					int32_t address );

MX_API mx_status_type mx_gpib_wait_for_service_request( MX_RECORD *gpib_record,
					double timeout );

MX_API mx_status_type mx_gpib_serial_poll( MX_RECORD *gpib_record,
					int32_t address,
					uint8_t *serial_poll_byte );

MX_API mx_status_type mx_gpib_serial_poll_disable( MX_RECORD *gpib_record );

#endif /* __MX_GPIB_H__ */
