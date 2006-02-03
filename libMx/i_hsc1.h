/*
 * Name:   i_hsc1.h
 *
 * Purpose: Header for MX driver for the X-ray Instrumentation Associates
 *          HSC-1 Huber slit controller.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_HSC1_H__
#define __I_HSC1_H__

/* Flags to use with mxi_hsc1_command() */

#define MXF_HSC1_IGNORE_BUSY_STATUS	0x1000
#define MXF_HSC1_IGNORE_RESPONSE	0x2000

/* Define the data structures used by an HSC-1 interface. */

#define MX_MAX_HSC1_AXES		4

#define MX_MAX_HSC1_MODULE_ID_LENGTH	13

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	long num_modules;

	char **module_id;
	int *module_is_busy;

} MX_HSC1_INTERFACE;

#define MXI_HSC1_INTERFACE_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HSC1_INTERFACE, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_modules", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HSC1_INTERFACE, num_modules), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_id", MXFT_STRING, NULL, \
		2, {MXU_VARARGS_LENGTH, MX_MAX_HSC1_MODULE_ID_LENGTH+1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HSC1_INTERFACE, module_id), \
	{sizeof(char), sizeof(char *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}

#ifndef MX_CR
#define MX_CR	'\015'
#define MX_LF	'\012'
#endif

MX_API mx_status_type mxi_hsc1_initialize_type( long type );
MX_API mx_status_type mxi_hsc1_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_hsc1_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_hsc1_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_hsc1_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_hsc1_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_hsc1_open( MX_RECORD *record );
MX_API mx_status_type mxi_hsc1_close( MX_RECORD *record );
MX_API mx_status_type mxi_hsc1_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_hsc1_getchar( MX_GENERIC *generic,
					char *c, int flags );
MX_API mx_status_type mxi_hsc1_putchar( MX_GENERIC *generic,
					char c, int flags );
MX_API mx_status_type mxi_hsc1_read( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_hsc1_write( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_hsc1_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available );
MX_API mx_status_type mxi_hsc1_discard_unread_input(
				MX_GENERIC *generic, int debug_flag );
MX_API mx_status_type mxi_hsc1_discard_unwritten_output(
				MX_GENERIC *generic, int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_hsc1_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_hsc1_generic_function_list;

extern mx_length_type mxi_hsc1_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_hsc1_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_hsc1_command(
	MX_HSC1_INTERFACE *hsc1_interface, unsigned long module_number,
	char *command, char *response, int response_buffer_length,
	int debug_flag );

#endif /* __I_HSC1_H__ */
