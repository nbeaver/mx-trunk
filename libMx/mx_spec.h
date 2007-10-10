/*
 * Name:    mx_spec.h
 *
 * Purpose: Header file for communication with SPEC by MX.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2004, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SPEC_H__
#define __MX_SPEC_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define SV_PORT_FIRST	6510
#define SV_PORT_LAST	6530

#define SV_NAME_LEN	80

struct svr_head {
	int magic;	/* SV_SPEC_MAGIC */
	int vers;	/* Protocol version number */
	unsigned size;	/* Size of this structure */
	unsigned sn;	/* Serial number (client's choice) */
	unsigned sec;	/* Time when sent (seconds) */
	unsigned usec;	/* Time when sent (microseconds) */
	int cmd;	/* Command code */
	int type;	/* Type of data */
	unsigned rows;	/* Number of rows if array data */
	unsigned cols;	/* Number of cols if array data */
	unsigned len;	/* Bytes of data that follow */
	char name[SV_NAME_LEN];		/* Name of property */
};

#define SV_SPEC_MAGIC	4277009102U

#define SV_VERSION	2

#define SV_CLOSE		1	/* From Client */
#define SV_ABORT		2	/* From Client */
#define SV_CMD			3	/* From Client */
#define SV_CMD_WITH_RETURN	4	/* From Client */
#define SV_RETURN		5	/* Not yet used */
#define SV_REGISTER		6	/* From Client */
#define SV_UNREGISTER		7	/* From Client */
#define SV_EVENT		8	/* From Server */
#define SV_HDW			9	/* Not yet used */
#define SV_ACK			10	/* Not yet used */
#define SV_CHAN_READ		11	/* From Client */
#define SV_CHAN_SEND		12	/* From Client */
#define SV_REPLY		13	/* From Server */
#define SV_HELLO		14	/* From Client */
#define SV_HELLO_REPLY		15	/* From Server */

#define SV_DOUBLE		1
#define SV_STRING		2
#define SV_ERROR		3
#define SV_ASSOC		4
#define SV_ARR_DOUBLE		5
#define SV_ARR_FLOAT		6
#define SV_ARR_LONG		7
#define SV_ARR_ULONG		8
#define SV_ARR_SHORT		9
#define SV_ARR_USHORT		10
#define SV_ARR_CHAR		11
#define SV_ARR_UCHAR		12
#define SV_ARR_STRING		13

/* ----- */

MX_API mx_status_type
mx_spec_send_message( MX_RECORD *spec_server_record,
			long spec_command_code,
			long spec_datatype,
			long spec_array_data_rows,
			long spec_array_data_cols,
			long spec_data_length,
			char *spec_property_name,
			void *data_pointer );

MX_API mx_status_type
mx_spec_receive_message( MX_RECORD *spec_server_record,
			long *spec_command_code,
			long *spec_datatype,
			long *spec_array_data_rows,
			long *spec_array_data_cols,
			long maximum_data_length,
			long *spec_data_length,
			char *spec_property_name,
			void *data_pointer );

/* ----- */

MX_API mx_status_type
mx_spec_send_command( MX_RECORD *spec_server_record,
			long spec_command_code,
			char *command );

MX_API mx_status_type
mx_spec_receive_response_line( MX_RECORD *spec_server_record,
			char *response,
			long response_buffer_length );

MX_API mx_status_type
mx_spec_num_response_bytes_available( MX_RECORD *spec_server_record,
					long *num_bytes_available );

MX_API mx_status_type
mx_spec_discard_unread_responses( MX_RECORD *spec_server_record );

/* ----- */

MX_API mx_status_type
mx_spec_get_array( MX_RECORD *spec_server_record,
			char *property_name,
			long spec_datatype,
			long num_dimensions,
			long *dimension_array,
			void *value );

MX_API mx_status_type
mx_spec_put_array( MX_RECORD *spec_server_record,
			char *property_name,
			long spec_datatype,
			long num_dimensions,
			long *dimension_array,
			void *value );

MX_API mx_status_type
mx_spec_get_string( MX_RECORD *spec_server_record,
			char *property_name,
			char *string_value,
			long max_string_length );

MX_API mx_status_type
mx_spec_put_string( MX_RECORD *spec_server_record,
			char *property_name,
			char *string_value );

/*
 * WARNING: The following functions use MX datatypes rather than Spec datatypes.
 */

MX_API mx_status_type
mx_spec_get_number( MX_RECORD *spec_server_record,
			char *property_name,
			long mx_datatype,
			void *value );

MX_API mx_status_type
mx_spec_put_number( MX_RECORD *spec_server_record,
			char *property_name,
			long mx_datatype,
			void *value );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SPEC_H__ */

