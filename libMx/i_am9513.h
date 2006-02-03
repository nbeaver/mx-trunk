/*
 * Name:    i_am9513.h
 *
 * Purpose: Header for MX interface driver for Am9513 counter/timer chips.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_AM9513_H__
#define __I_AM9513_H__

#include "mx_generic.h"

/* Do we want delays to be inserted to slow down I/O port calls? */

#define MX_AM9513_INSERT_DELAYS		FALSE

#define MX_AM9513_DELAY_TIME	10	/* in milliseconds */

#if MX_AM9513_INSERT_DELAYS
#  define MX_AM9513_DELAY	mx_msleep( MX_AM9513_DELAY_TIME )
#else
#  define MX_AM9513_DELAY
#endif

/* Debugging macro */

#define MX_AM9513_DEBUG(a,b)	\
		MX_DEBUG( 3,("%s\n%s", fname, mxi_am9513_dump((a),(b)) ))

/*---------*/

#define MX_AM9513_NUM_COUNTERS	5

typedef struct {
	MX_RECORD *record;

	MX_RECORD *portio_record;
	unsigned long base_address;

	/* The master mode register is stored as an 'unsigned long' so that
	 * the MXFT_HEX field type may be used.  The actual data it contains
	 * is actually a 16 bit unsigned integer.
	 */

	unsigned long master_mode_register;

	uint16_t counter_mode_register[MX_AM9513_NUM_COUNTERS];
	uint16_t load_register[MX_AM9513_NUM_COUNTERS];
	uint16_t hold_register[MX_AM9513_NUM_COUNTERS];

	uint8_t status_register;

	MX_RECORD *counter_array[MX_AM9513_NUM_COUNTERS];
} MX_AM9513;

#define MXI_AM9513_STANDARD_FIELDS \
  {-1, -1, "portio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513, portio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "master_mode_register", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AM9513, master_mode_register), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MX_AM9513_DATA_REGISTER  0
#define MX_AM9513_CMD_REGISTER   1

MX_API uint8_t mxi_am9513_inp8( MX_AM9513 *am9513, int port_number );

MX_API uint16_t mxi_am9513_inp16( MX_AM9513 *am9513, int port_number );

MX_API void mxi_am9513_outp8( MX_AM9513 *am9513, int port_number,
						uint8_t byte_value );

MX_API void mxi_am9513_outp16( MX_AM9513 *am9513, int port_number,
						uint16_t word_value );

/*---*/

MX_API mx_status_type mxi_am9513_initialize_type( long type );
MX_API mx_status_type mxi_am9513_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_am9513_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_am9513_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_am9513_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_am9513_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_am9513_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_am9513_open( MX_RECORD *record );
MX_API mx_status_type mxi_am9513_close( MX_RECORD *record );
MX_API mx_status_type mxi_am9513_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_am9513_grab_counters( MX_RECORD *record,
					long num_counters,
					MX_INTERFACE *am9513_interface_array );

MX_API mx_status_type mxi_am9513_release_counters( MX_RECORD *record,
					long num_counters,
					MX_INTERFACE *am9513_interface_array );

MX_API char *mxi_am9513_dump( MX_AM9513 *am9513, int do_inquire );

MX_API mx_status_type mxi_am9513_get_counter_mode_register(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_set_counter_mode_register(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_get_load_register(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_set_load_register(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_load_counter_from_load_register(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_load_counter(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_get_hold_register(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_set_hold_register(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_save_counter_to_hold_register(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_read_counter(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_set_tc( MX_AM9513 *am9513, int counter );
MX_API mx_status_type mxi_am9513_clear_tc( MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_arm_counter( MX_AM9513 *am9513, int counter );
MX_API mx_status_type mxi_am9513_disarm_counter(
					MX_AM9513 *am9513, int counter );

MX_API mx_status_type mxi_am9513_get_status( MX_AM9513 *am9513 );

extern MX_RECORD_FUNCTION_LIST mxi_am9513_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_am9513_generic_function_list;

extern mx_length_type mxi_am9513_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_am9513_rfield_def_ptr;

/* The following structure is used for debugging only. */

extern MX_AM9513 mxi_am9513_debug_struct;

#endif /* __I_AM9513_H__ */
