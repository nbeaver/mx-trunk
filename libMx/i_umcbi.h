/*
 * Name:   i_umcbi.h
 *
 * Purpose: Header for MX driver for generic interface to the
 *          EG&G Ortec Unified MCB Interface for 32 bits (A11-B32).
 *
 *          So far this driver has only been tested in conjunction
 *          with the Ortec Trump multichannel buffer card for PCs
 *          under Windows 95.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_UMCBI_H__
#define __I_UMCBI_H__

#include "mx_record.h"
#include "mx_generic.h"

/* The following #define is made so that we don't have to include mcbcio32.h
 * here.  The value should be the same as MIODETNAMEMAX.
 */

#define MX_UMCBI_DETECTOR_NAME_LENGTH	128

/* Define the data structures used by the Ortec UMCBI driver. */
typedef struct {
	int detector_number;
	void *detector_handle;		/* Same as HDET. */
	unsigned long detector_id;
	MX_RECORD *record;	/* Pointer to MX record for this detector. */
	char name[MX_UMCBI_DETECTOR_NAME_LENGTH];
} MX_UMCBI_DETECTOR;

/* In the MX_UMCBI typedef below, 'detector_array' is an array of pointers to
 * all the individual Ortec multichannel buffers (MCBs) structures available.
 * 'num_detectors' contains the number of detectors in the list.
 */

typedef struct {
	MX_RECORD *record;

	int num_detectors;
	MX_UMCBI_DETECTOR *detector_array;
} MX_UMCBI;

/* There are no new fields, so MXI_UMCBI_STANDARD_FIELDS would be
 * of zero length.
 */

MX_API mx_status_type mxi_umcbi_initialize_type( long type );
MX_API mx_status_type mxi_umcbi_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_umcbi_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_umcbi_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_umcbi_read_parms_from_hardware(MX_RECORD *record);
MX_API mx_status_type mxi_umcbi_write_parms_to_hardware(MX_RECORD *record);
MX_API mx_status_type mxi_umcbi_open( MX_RECORD *record );
MX_API mx_status_type mxi_umcbi_close( MX_RECORD *record );

MX_API mx_status_type mxi_umcbi_getchar( MX_GENERIC *generic,
					char *c, int flags );
MX_API mx_status_type mxi_umcbi_putchar( MX_GENERIC *generic,
					char c, int flags );
MX_API mx_status_type mxi_umcbi_read( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_umcbi_write( MX_GENERIC *generic,
					void *buffer, size_t count );
MX_API mx_status_type mxi_umcbi_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available );
MX_API mx_status_type mxi_umcbi_discard_unread_input(
					MX_GENERIC *generic, int debug_flag);
MX_API mx_status_type mxi_umcbi_discard_unwritten_output(
					MX_GENERIC *generic, int debug_flag);

extern MX_RECORD_FUNCTION_LIST mxi_umcbi_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_umcbi_generic_function_list;

extern long mxi_umcbi_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_umcbi_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_umcbi_get_detector_struct( MX_RECORD *umcbi_record,
			int detector_number, MX_UMCBI_DETECTOR **detector );

MX_API mx_status_type mxi_umcbi_command( MX_UMCBI_DETECTOR *detector,
	char *command, char *response, int response_buffer_length,
	int debug_flag );

#if 0
MX_API mx_status_type mxi_umcbi_getline( MX_UMCBI_DETECTOR *detector,
			char *buffer, long buffer_size, int debug_flag );
#endif

MX_API mx_status_type mxi_umcbi_putline( MX_UMCBI_DETECTOR *detector,
			char *buffer, int debug_flag );

MX_API char *mxi_umcbi_strerror( void );

#endif /* __I_UMCBI_H__ */
