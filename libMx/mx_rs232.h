/*
 * Name:    mx_rs232.h
 *
 * Purpose: Header file for RS-232, TCP sockets and other similar interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2008, 2010-2012, 2014, 2016-2017, 2019-2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_RS232_H__
#define __MX_RS232_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/*** Supported parity types ***/

#define MXF_232_NO_PARITY	'N'
#define MXF_232_EVEN_PARITY	'E'
#define MXF_232_ODD_PARITY	'O'
#define MXF_232_MARK_PARITY	'M'
#define MXF_232_SPACE_PARITY	'S'

/*** Supported flow control types ***/

#define MXF_232_NO_FLOW_CONTROL		'N'
#define MXF_232_HARDWARE_FLOW_CONTROL	'H'	/* RTS-CTS _and_ CD */
#define MXF_232_SOFTWARE_FLOW_CONTROL	'S'	/* XON-XOFF */
#define MXF_232_BOTH_FLOW_CONTROL	'B'

/* MXF_232_RTS_CTS_FLOW_CONTROL is similar to hardware flow control, except
 * that the status of the Carrier Detect line is ignored.
 */

#define MXF_232_RTS_CTS_FLOW_CONTROL	'R'

/* MXF_232_DTR_DSR_FLOW_CONTROL is similar to RTS-CTS flow control, except
 * for the fact that the DSR and DTR lines are used instead.  At the moment,
 * this is only supported under Win32.
 */

#define MXF_232_DTR_DSR_FLOW_CONTROL	'D'

/******/

/* Transfer flags used by mx_rs232_getchar(), mx_rs232_putchar(),
 * mx_rs232_read(), mx_rs232_write(), mx_rs232_getline(), and
 * mx_rs232_putline().
 *
 * MXF_232_WAIT, MXF_232_NOWAIT, and MXF_232_DEBUG are defined
 * the way they are for backward compatibility with old code.
 * In particular, MXF_232_DEBUG must be 0x1 to be compatible with
 * drivers that pass their internal debugging flags to RS232
 * functions.
 *
 * MXF_232_HIDE_FROM_DEBUG is used to prevent a character from 
 * appearing in debug output twice.  For example, suppose that
 * a particular RS232 driver implements getline() by making calls
 * to getchar().  If implemented in a naive way, serial debugging
 * might result in a character being displayed once by getchar()
 * and a second time by getline().
 *
 * If MXF_232_DEBUG and MXF_232_HIDE_FROM_DEBUG are both specified,
 * then MXF_232_DEBUG wins.
 *
 * The function mx_rs232_show_debugging() can be used to determine
 * whether serial debugging output should be displayed.
 */

#define MXF_232_WAIT			0x0

#define MXF_232_DEBUG			0x1
#define MXF_232_NOWAIT			0x2
#define MXF_232_IGNORE_NULLS		0x4

#define MXF_232_AUTO_TIMEOUT_RECOVERY	0x1000

#define MXF_232_HIDE_FROM_DEBUG		0x10000000

/* 'rs232_flags' field bit definitions. */

#define MXF_232_UNBUFFERED_IO			0x1
#define MXF_232_DO_NOT_CHANGE_PORT_SETTINGS	0x2
#define MXF_232_ALWAYS_IGNORE_NULLS		0x4

#define MXF_232_SHARED				0x10

#define MXF_232_FLUSH_AFTER_PUTCHAR		0x100
#define MXF_232_FLUSH_AFTER_WRITE		0x200
#define MXF_232_FLUSH_AFTER_PUTLINE		0x400

#define MXF_232_SUPPRESS_TIMEOUT_ERROR_MESSAGES	0x1000
#define MXF_232_POSIX_VMIN_FIX			0x2000

#define MXF_232_DEBUG_SERIAL			0x1000000
#define MXF_232_DEBUG_SERIAL_HEX		0x2000000

#define MXF_232_NO_REMOTE_ACCESS		0x80000000

/* 'signal_state' field bit definitions. */

#define MXF_232_REQUEST_TO_SEND		0x01
#define MXF_232_CLEAR_TO_SEND		0x02
#define MXF_232_DATA_TERMINAL_READY	0x04
#define MXF_232_DATA_SET_READY		0x08
#define MXF_232_DATA_CARRIER_DETECT	0x10
#define MXF_232_RING_INDICATOR		0x20

/* Used by mx_rs232_verify_configuration(). */

#define MXF_232_DONT_CARE		(0xffffffff)

/* ======== Generic RS-232 interface data structure ======= */

#define MX_RS232_MAX_TERMINATORS	4

#define MXU_RS232_BUFFER_LENGTH		250

typedef struct {
	MX_RECORD *record;

	long speed;
	long word_size;
	char parity;
	long stop_bits;
	char flow_control;
	unsigned long read_terminators;
	unsigned long write_terminators;
	double timeout;
	unsigned long rs232_flags;

	unsigned long transfer_flags;

	/* We should not use the unadorned names getchar, putchar, read,
	 * write, getline, and putline below since these are the names of
	 * functions in the ANSI and POSIX libraries.  Some compilers may
	 * get unduly upset if you try to use the names of run-time library
	 * functions here.
	 */

	char getchar_value;
	char putchar_value;

	unsigned long bytes_read;
	unsigned long bytes_written;

	char getline_buffer[MXU_RS232_BUFFER_LENGTH+1];
	char putline_buffer[MXU_RS232_BUFFER_LENGTH+1];

	unsigned long num_input_bytes_available;

	mx_bool_type discard_unread_input;
	mx_bool_type discard_unwritten_output;

	long num_read_terminator_chars;
	long num_write_terminator_chars;
	char read_terminator_array[MX_RS232_MAX_TERMINATORS+1];
	char write_terminator_array[MX_RS232_MAX_TERMINATORS+1];

	unsigned long signal_state;
	unsigned long old_signal_state;

	mx_bool_type get_configuration;
	mx_bool_type set_configuration;
	mx_bool_type send_break;
	mx_bool_type echo;

	unsigned long serial_debug;
} MX_RS232;

#define MXLV_232_GETCHAR			101
#define MXLV_232_PUTCHAR			102
#define MXLV_232_READ				103
#define MXLV_232_WRITE				104
#define MXLV_232_GETLINE			105
#define MXLV_232_PUTLINE			106
#define MXLV_232_NUM_INPUT_BYTES_AVAILABLE	107
#define MXLV_232_DISCARD_UNREAD_INPUT		108
#define MXLV_232_DISCARD_UNWRITTEN_OUTPUT	109
#define MXLV_232_SIGNAL_STATE			110
#define MXLV_232_GET_CONFIGURATION		111
#define MXLV_232_SET_CONFIGURATION		112
#define MXLV_232_SEND_BREAK			113
#define MXLV_232_ECHO				114
#define MXLV_232_SERIAL_DEBUG			115

#define MX_RS232_STANDARD_FIELDS \
  {-1, -1, "speed", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, speed), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "word_size", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, word_size), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "parity", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, parity), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "stop_bits", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, stop_bits), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "flow_control", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, flow_control), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "read_terminators", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, read_terminators), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "write_terminators", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, write_terminators), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, timeout), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "rs232_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, rs232_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {MXLV_232_GETCHAR, -1, "getchar", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, getchar_value), \
	{0}, NULL, 0}, \
  \
  {MXLV_232_PUTCHAR, -1, "putchar", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, putchar_value), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "bytes_read", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, bytes_read), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "bytes_written", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, bytes_written), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_232_GETLINE, -1, "getline", \
			MXFT_STRING, NULL, 1, {MXU_RS232_BUFFER_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, getline_buffer), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_232_PUTLINE, -1, "putline", \
			MXFT_STRING, NULL, 1, {MXU_RS232_BUFFER_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, putline_buffer), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_232_NUM_INPUT_BYTES_AVAILABLE, -1, "num_input_bytes_available", \
						MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, num_input_bytes_available), \
	{0}, NULL, MXFF_POLL}, \
  \
  {MXLV_232_DISCARD_UNREAD_INPUT, -1, "discard_unread_input", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, discard_unread_input), \
	{0}, NULL, 0}, \
  \
  {MXLV_232_DISCARD_UNWRITTEN_OUTPUT, -1, "discard_unwritten_output", \
						MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, discard_unwritten_output), \
	{0}, NULL, 0}, \
  \
  {MXLV_232_SIGNAL_STATE, -1, "signal_state", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, signal_state), \
	{0}, NULL, 0}, \
  \
  {MXLV_232_GET_CONFIGURATION, -1, "get_configuration", \
				MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, get_configuration), \
	{0}, NULL, 0}, \
  \
  {MXLV_232_SET_CONFIGURATION, -1, "set_configuration", \
				MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, set_configuration), \
	{0}, NULL, 0}, \
  \
  {MXLV_232_SEND_BREAK, -1, "send_break", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, send_break), \
	{0}, NULL, 0}, \
  \
  {MXLV_232_ECHO, -1, "echo", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, echo), \
	{0}, NULL, 0}, \
  \
  {MXLV_232_SERIAL_DEBUG, -1, "serial_debug", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_RS232, serial_debug), \
	{0}, NULL, 0}

/*
 * The structure type MX_RS232_FUNCTION_LIST contains a list of
 * pointers to functions that vary from interface type to
 * interface type.
 */

typedef struct {
	mx_status_type ( *getchar ) ( MX_RS232 *rs232, char *c );
	mx_status_type ( *putchar ) ( MX_RS232 *rs232, char c );
	mx_status_type ( *read ) ( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
	mx_status_type ( *write ) ( MX_RS232 *rs232,
					char *buffer,
					size_t bytes_to_write,
					size_t *bytes_written );
	mx_status_type ( *getline ) ( MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );
	mx_status_type ( *putline ) ( MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written );
	mx_status_type ( *num_input_bytes_available ) ( MX_RS232 *rs232 );
	mx_status_type ( *discard_unread_input ) ( MX_RS232 *rs232 );
	mx_status_type ( *discard_unwritten_output ) ( MX_RS232 *rs232 );
	mx_status_type ( *get_signal_state ) ( MX_RS232 *rs232 );
	mx_status_type ( *set_signal_state ) ( MX_RS232 *rs232 );
	mx_status_type ( *get_configuration ) ( MX_RS232 *rs232 );
	mx_status_type ( *set_configuration ) ( MX_RS232 *rs232 );
	mx_status_type ( *send_break ) ( MX_RS232 *rs232 );
	mx_status_type ( *wait_for_input_available ) ( MX_RS232 *rs232,
					double wait_timeout_in_seconds );
	mx_status_type ( *get_echo ) ( MX_RS232 *rs232 );
	mx_status_type ( *set_echo ) ( MX_RS232 *rs232 );
	mx_status_type ( *flush ) ( MX_RS232 *rs232 );
} MX_RS232_FUNCTION_LIST;

/* ============== Internal driver function prototypes. ============== */

MX_API_PRIVATE mx_status_type mx_rs232_unbuffered_getline(
					MX_RS232 *rs232,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read,
					unsigned long local_transfer_flags,
					double timeout_in_seconds );

MX_API_PRIVATE mx_status_type mx_rs232_unbuffered_putline(
					MX_RS232 *rs232,
					char *buffer,
					size_t *bytes_written,
					unsigned long local_transfer_flags );

/* ============== Interface function prototypes. ============== */

MX_API mx_bool_type mx_rs232_show_debugging( MX_RS232 *rs232,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_get_serial_debug( MX_RECORD *rs232_record,
						unsigned long *debug_flags );

MX_API mx_status_type mx_rs232_set_serial_debug( MX_RECORD *rs232_record,
						unsigned long debug_flags );

MX_API mx_status_type mx_rs232_check_port_parameters( MX_RECORD *rs232_record );

MX_API mx_status_type mx_rs232_convert_terminator_characters(
						MX_RECORD *rs232_record);

MX_API char *mx_rs232_find_terminators( char *buffer, size_t buffer_length,
					unsigned long num_terminators,
					char *terminator_array );

MX_API mx_status_type mx_rs232_get_pointers( MX_RECORD *rs232_record,
					MX_RS232 **rs232,
					MX_RS232_FUNCTION_LIST **fptr,
					const char *calling_fname );

MX_API mx_status_type mx_rs232_getchar( MX_RECORD *rs232_record,
					char *c,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_putchar( MX_RECORD *rs232_record,
					char c,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_getchar_with_timeout( MX_RECORD *rs232_record,
					char *c,
					unsigned long transfer_flags,
					double timeout_in_seconds );

MX_API mx_status_type mx_rs232_read( MX_RECORD *rs232_record,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_write( MX_RECORD *rs232_record,
					char *buffer,
					size_t bytes_to_write,
					size_t *bytes_written,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_read_with_timeout( MX_RECORD *rs232_record,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read,
					unsigned long transfer_flags,
					double timeout_in_seconds );

MX_API mx_status_type mx_rs232_getline( MX_RECORD *rs232_record,
					char *buffer, 
					size_t max_bytes_to_read,
					size_t *bytes_read,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_putline( MX_RECORD *rs232_record,
					char *buffer,
					size_t *bytes_written,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_getline_with_timeout( MX_RECORD *rs232_record,
					char *buffer, 
					size_t max_bytes_to_read,
					size_t *bytes_read,
					unsigned long transfer_flags,
					double timeout_in_seconds );

MX_API mx_status_type mx_rs232_num_input_bytes_available(
					MX_RECORD *rs232_record,
				    unsigned long *num_input_bytes_available );

MX_API mx_status_type mx_rs232_wait_for_input_available(
					MX_RECORD *rs232_record,
				    unsigned long *num_input_bytes_available,
					double wait_timeout_in_seconds );

MX_API mx_status_type mx_rs232_discard_unread_input(
					MX_RECORD *rs232_record,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_discard_unwritten_output(
					MX_RECORD *rs232_record,
					unsigned long transfer_flags );

MX_API mx_status_type mx_rs232_discard_until_string( MX_RECORD *rs232_record,
					char *string_to_look_for,
					mx_bool_type discard_read_terminators,
					unsigned long transfer_flags,
					double timeout_in_seconds );

MX_API mx_status_type mx_rs232_get_signal_state( MX_RECORD *rs232_record,
					unsigned long *signal_state );

MX_API mx_status_type mx_rs232_set_signal_state( MX_RECORD *rs232_record,
					unsigned long signal_state );

MX_API mx_status_type mx_rs232_print_signal_state( MX_RECORD *rs232_record );

MX_API mx_status_type mx_rs232_get_signal_bit( MX_RECORD *rs232_record,
					long bit_type, long *bit_value );

MX_API mx_status_type mx_rs232_set_signal_bit( MX_RECORD *rs232_record,
					long bit_type, long bit_value );

MX_API mx_status_type mx_rs232_get_configuration( MX_RECORD *rs232_record,
					long *speed,
					long *word_size,
					char *parity,
					long *stop_bits,
					char *flow_control,
					unsigned long *read_terminators,
					unsigned long *write_terminators,
					double *timeout_in_seconds );

MX_API mx_status_type mx_rs232_set_configuration( MX_RECORD *rs232_record,
					long speed,
					long word_size,
					char parity,
					long stop_bits,
					char flow_control,
					unsigned long read_terminators,
					unsigned long write_terminators,
					double timeout_in_seconds );

MX_API mx_status_type mx_rs232_verify_configuration( MX_RECORD *rs232_record,
					long speed,
					long word_size,
					char parity,
					long stop_bits,
					char flow_control,
					unsigned long read_terminators,
					unsigned long write_terminators,
					double timeout );

MX_API mx_status_type mx_rs232_send_break( MX_RECORD *rs232_record );

MX_API mx_status_type mx_rs232_get_echo( MX_RECORD *rs232_record,
					mx_bool_type *echo_state );

MX_API mx_status_type mx_rs232_set_echo( MX_RECORD *rs232_record,
					mx_bool_type echo_state );

MX_API mx_status_type mx_rs232_get_timeout( MX_RECORD *rs232_record,
					double *timeout_in_seconds );

MX_API mx_status_type mx_rs232_set_timeout( MX_RECORD *rs232_record,
					double timeout_in_seconds );

MX_API mx_status_type mx_rs232_flush( MX_RECORD *rs232_record );

MX_API mx_status_type mx_rs232_send_file( MX_RECORD *rs232_record,
					char *filename,
					mx_bool_type use_raw_io );

#ifdef __cplusplus
}
#endif

#endif /* __MX_RS232_H__ */

