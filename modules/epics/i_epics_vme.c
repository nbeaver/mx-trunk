/*
 * Name:    i_epics_vme.c
 *
 * Purpose: MX driver for VME access via the EPICS VME record written by
 *          Mark Rivers of the University of Chicago.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2008-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_EPICS_VME_DEBUG	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include <stdlib.h>
#include <limits.h>

#include "mx_constants.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_epics.h"
#include "mx_vme.h"
#include "i_epics_vme.h"

MX_RECORD_FUNCTION_LIST mxi_epics_vme_record_function_list = {
	NULL,
	mxi_epics_vme_create_record_structures,
	mxi_epics_vme_finish_record_initialization,
	mxi_epics_vme_delete_record,
	NULL,
	mxi_epics_vme_open,
	NULL,
	NULL,
	mxi_epics_vme_resynchronize
};

MX_VME_FUNCTION_LIST mxi_epics_vme_vme_function_list = {
	mxi_epics_vme_input,
	mxi_epics_vme_output,
	mxi_epics_vme_multi_input,
	mxi_epics_vme_multi_output,
	mxi_epics_vme_get_parameter,
	mxi_epics_vme_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxi_epics_vme_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VME_STANDARD_FIELDS,
	MXI_EPICS_VME_STANDARD_FIELDS
};

long mxi_epics_vme_num_record_fields
		= sizeof( mxi_epics_vme_record_field_defaults )
			/ sizeof( mxi_epics_vme_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxi_epics_vme_rfield_def_ptr
		= &mxi_epics_vme_record_field_defaults[0];

/*--*/

#if MX_EPICS_VME_DEBUG

/* MX_EPICS_VME_DEBUG_WARNING_SUPPRESS is used to suppress uninitialized
 * variable warnings from GCC.
 */

#if 0
#  define MX_EPICS_VME_DEBUG_WARNING_SUPPRESS \
	do { \
		uint8_ptr = NULL; uint16_ptr = NULL; uint32_ptr = NULL; \
	} while(0)
#endif

#  define MX_EPICS_VME_DEBUG_PRINT( label ) \
	do { \
		unsigned long value = 0; \
                                     \
		switch( vme->data_size ) { \
		case MXF_VME_D8:  value = (unsigned long) *uint8_ptr;  break; \
		case MXF_VME_D16: value = (unsigned long) *uint16_ptr; break; \
		case MXF_VME_D32: value = (unsigned long) \
			*(uint32_t *)(vme->data_pointer); break; \
		} \
		MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx", \
			label, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value )); \
	} while(0)

#  define MX_EPICS_VME_MULTI_DEBUG_PRINT( label ) \
	do { \
		unsigned long value = 0; \
                                     \
		switch( vme->data_size ) { \
		case MXF_VME_D8:  value = (unsigned long) *uint8_ptr;  break; \
		case MXF_VME_D16: value = (unsigned long) *uint16_ptr; break; \
		case MXF_VME_D32: value = (unsigned long) \
			*(uint32_t *)(vme->data_pointer); break; \
		} \
	MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx (%lu values)", \
			label, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value, \
			vme->num_values )); \
	} while(0)
#else

#  define MX_EPICS_VME_DEBUG_WARNING_SUPPRESS

#  define MX_EPICS_VME_DEBUG_PRINT

#  define MX_EPICS_VME_MULTI_DEBUG_PRINT

#endif

/*==========================*/

static mx_status_type
mxi_epics_vme_transfer_data( MX_VME *vme,
				MX_EPICS_VME *epics_vme,
				int direction )
{
	static const char fname[] = "mxi_epics_vme_transfer_data()";

	MX_EPICS_GROUP epics_group;
	int32_t epics_address_mode, epics_data_size, epics_num_values;
	int32_t address_increment, epics_direction, epics_address, proc_value;
	int32_t *int32_data_pointer;
	long i;
	uint8_t *uint8_ptr;
	uint16_t *uint16_ptr;
	uint32_t *uint32_ptr;
	mx_bool_type longs_are_32bits;
	mx_status_type mx_status;

	uint8_ptr = NULL;
	uint16_ptr = NULL;

	/* Object if we are asked to transfer more data than the EPICS
	 * side is prepared for.
	 */

	if ( vme->num_values > epics_vme->max_epics_values ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Attempt to transfer %lu data values via EPICS VME record '%s' failed."
	"  EPICS record '%s' only allows up to %ld values to be transferred.",
			vme->num_values, vme->record->name,
			epics_vme->epics_record_name,
			(long) epics_vme->max_epics_values );
	}

	/***************** Start the synchronous group. *****************/

	mx_epics_start_group( &epics_group );

	/* Set the VME addressing mode. */

	if ( vme->address_mode != epics_vme->old_address_mode ) {

		switch( vme->address_mode ) {
		case MXF_VME_A16:
			epics_address_mode = 0;
			break;
		case MXF_VME_A24:
			epics_address_mode = 1;
			break;
		case MXF_VME_A32:
			epics_address_mode = 2;
			break;
		default:
			mx_epics_delete_group( &epics_group );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal VME address mode A%lu for EPICS VME record '%s'.  "
		"The allowed values are A16, A24, and A32.",
				vme->address_mode, vme->record->name );
			break;
		}
					
		MX_DEBUG( 2,("%s: sending %ld to '%s'",
					fname, (long) epics_address_mode,
					epics_vme->amod_pv.pvname ));

		mx_status = mx_group_caput( &epics_group, &(epics_vme->amod_pv),
					MX_CA_LONG, 1, &epics_address_mode);

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		epics_vme->old_address_mode = vme->address_mode;
	}

	/* Set the VME data size. */

	if ( vme->data_size != epics_vme->old_data_size ) {

		switch( vme->data_size ) {
		case MXF_VME_D8:
			epics_data_size = 0;
			break;
		case MXF_VME_D16:
			epics_data_size = 1;
			break;
		case MXF_VME_D32:
			epics_data_size = 2;
			break;
		default:
			mx_epics_delete_group( &epics_group );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal VME data size D%lu for EPICS VME record '%s'.  "
		"The allowed values are D8, D16, and D32.",
				vme->data_size, vme->record->name );
			break;
		}
					
		MX_DEBUG( 2,("%s: sending %ld to '%s'",
					fname, (long) epics_data_size,
					epics_vme->dsiz_pv.pvname ));

		mx_status = mx_group_caput( &epics_group, &(epics_vme->dsiz_pv),
					MX_CA_LONG, 1, &epics_data_size);

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		epics_vme->old_data_size = vme->data_size;
	}

	/* Set the number of values to transfer. */

	if ( vme->num_values != epics_vme->old_num_values ) {

		epics_num_values = (long) vme->num_values;

		MX_DEBUG( 2,("%s: sending %ld to '%s'",
					fname, (long) epics_num_values,
					epics_vme->nuse_pv.pvname ));

		mx_status = mx_group_caput( &epics_group, &(epics_vme->nuse_pv),
					MX_CA_LONG, 1, &epics_num_values );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		epics_vme->old_num_values = vme->num_values;
	}

	/* Set the address increment. */

	if ( direction == MXF_EPICS_VME_OUTPUT ) {
		address_increment = (long) vme->write_address_increment;

	} else if ( direction == MXF_EPICS_VME_INPUT ) {
		address_increment = (long) vme->read_address_increment;
	} else {
		mx_epics_delete_group( &epics_group );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal direction flag %d for EPICS VME record '%s'.  "
		"The allowed values are %d and %d.",
			direction, vme->record->name,
			MXF_EPICS_VME_OUTPUT, MXF_EPICS_VME_INPUT );
	}

	if ( address_increment != epics_vme->old_address_increment ) {

		if ( ( address_increment < 0 ) || ( address_increment > 4 ) )
		{
			mx_epics_delete_group( &epics_group );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal address increment %ld for EPICS VME record '%s'.  "
		"The allowed values are in the range (0-4).",
			(long) address_increment, vme->record->name );
		}

		MX_DEBUG( 2,("%s: sending %ld to '%s'",
					fname, (long) address_increment,
					epics_vme->ainc_pv.pvname ));

		mx_status = mx_group_caput( &epics_group, &(epics_vme->ainc_pv),
					MX_CA_LONG, 1, &address_increment );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		epics_vme->old_address_increment = address_increment;
	}

	/* Set the data transfer direction. */

	if ( direction != epics_vme->old_direction ) {

		epics_direction = direction;

		MX_DEBUG( 2,("%s: sending %ld to '%s'",
					fname, (long) epics_direction,
					epics_vme->rdwt_pv.pvname ));

		mx_status = mx_group_caput( &epics_group, &(epics_vme->rdwt_pv),
					MX_CA_LONG, 1, &epics_direction );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		epics_vme->old_direction = direction;
	}

	/* Set the VME address. */

	if ( vme->address != epics_vme->old_address ) {

		epics_address = (long) vme->address;

		MX_DEBUG( 2,("%s: sending %#lx to '%s'",
					fname, (long) epics_address,
					epics_vme->addr_pv.pvname ));

		mx_status = mx_group_caput( &epics_group, &(epics_vme->addr_pv),
					MX_CA_LONG, 1, &epics_address );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		epics_vme->old_address = vme->address;
	}

	/* If the data are D32 values and longs are 32-bit integers, then
	 * we can transfer them directly via vme->data_pointer since the
	 * remote EPICS server is using 32 bit integers as well.  Otherwise,
	 * we must use the temporary transfer buffer.
	 */

	if ( sizeof(int32_t) == sizeof(long) ) {
		longs_are_32bits = TRUE;
	} else {
		longs_are_32bits = FALSE;
	}

	if ( (vme->data_size == MXF_VME_D32) && longs_are_32bits ) {
		int32_data_pointer = (int32_t *) vme->data_pointer;
	} else {
		int32_data_pointer = epics_vme->temp_buffer;
	}

	/* If we are sending data to the VME crate, copy any 8 or 16 bit
	 * data to the temporary transfer buffer.
	 */

	if ( direction == MXF_EPICS_VME_OUTPUT ) {
		switch ( vme->data_size ) {
		case MXF_VME_D8:
			uint8_ptr = (uint8_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				epics_vme->temp_buffer[i] = (int32_t) uint8_ptr[i];
			}
			break;
		case MXF_VME_D16:
			uint16_ptr = (uint16_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				epics_vme->temp_buffer[i] = (int32_t) uint16_ptr[i];
			}
			break;
		case MXF_VME_D32:
			if ( longs_are_32bits == FALSE ) {
				uint32_ptr = (uint32_t *) vme->data_pointer;

				for ( i = 0; i < vme->num_values; i++ ) {
					epics_vme->temp_buffer[i] =
						(int32_t) uint32_ptr[i];
				}
			}
			break;
		}
	}

	/* Now we are ready to transfer the data. */

	if ( direction == MXF_EPICS_VME_OUTPUT ) {

		/* OUTPUT: Send the values. */

		if ( vme->num_values == 1 ) {

			MX_DEBUG( 2,("%s: sending %lu to '%s'",
					fname, (unsigned long) *int32_data_pointer,
					epics_vme->val_pv.pvname ));
		} else {
			MX_DEBUG( 2,("%s: sending data to '%s'",
					fname, epics_vme->val_pv.pvname ));
		}

		mx_status = mx_group_caput( &epics_group, &(epics_vme->val_pv),
				MX_CA_LONG, vme->num_values, int32_data_pointer);

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		/* OUTPUT: Process the record. */

		proc_value = 1;

		MX_DEBUG( 2,("%s: sending %ld to '%s'",
					fname, (long) proc_value,
					epics_vme->proc_pv.pvname ));

		mx_status = mx_group_caput( &epics_group, &(epics_vme->proc_pv),
						MX_CA_LONG, 1, &proc_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}
	} else {
		/* INPUT: Process the record. */

		proc_value = 1;

		MX_DEBUG( 2,("%s: sending %ld to '%s'",
					fname, (long) proc_value,
					epics_vme->proc_pv.pvname ));

		mx_status = mx_group_caput( &epics_group, &(epics_vme->proc_pv),
						MX_CA_LONG, 1, &proc_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		/* INPUT: Read the values. */

		MX_DEBUG( 2,("%s: about to read from '%s'", fname,
						epics_vme->val_pv.pvname ));

		mx_status = mx_group_caget( &epics_group, &(epics_vme->val_pv),
				MX_CA_LONG, vme->num_values, int32_data_pointer);

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}

		if ( vme->num_values == 1 ) {

			MX_DEBUG( 2,("%s: read %ld from '%s'",
					fname, (long) *int32_data_pointer,
					epics_vme->val_pv.pvname ));
		} else {
			MX_DEBUG( 2,("%s: data received for '%s'.",
					fname, epics_vme->val_pv.pvname ));
		}
	}

	/* Check to see if there were any errors in the transfer. */

	MX_DEBUG( 2,("%s: about to read from '%s'",
			fname, epics_vme->sarr_pv.pvname ));

	mx_status = mx_group_caget( &epics_group, &(epics_vme->sarr_pv),
			MX_CA_CHAR, vme->num_values, epics_vme->status_array );

	/***************** End the synchronous group. *****************/

	mx_epics_end_group( &epics_group );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: data received for '%s'.",
			fname, epics_vme->sarr_pv.pvname ));

	for ( i = 0; i < vme->num_values; i++ ) {
		if ( epics_vme->status_array[i] != 0 ) {
			if ( direction == MXF_EPICS_VME_INPUT ) {
				mx_status = mx_error(
					MXE_INTERFACE_IO_ERROR, fname,
		"Error reading array element %ld (D%lu) from address A%#lx %lu "
		"for EPICS VME record '%s'.  vxMemProbe status = %#x",
					i, vme->data_size,
					vme->address_mode, vme->address,
					epics_vme->record->name,
					epics_vme->status_array[i] );
			} else {
				mx_status = mx_error(
					MXE_INTERFACE_IO_ERROR, fname,
		"Error writing array element %ld (D%lu) to address A%#lx %lu "
		"for EPICS VME record '%s'.  vmMemProbe status = %#x",
					i, vme->data_size,
					vme->address_mode, vme->address,
					epics_vme->record->name,
					epics_vme->status_array[i] );
			}
			if ((vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS) == 0)
				return mx_status;
		}
	}

	/* If we are receiving data from the VME crate, copy any 8 or 16 bit
	 * data from the temporary transfer buffer.
	 */

	if ( direction == MXF_EPICS_VME_INPUT ) {
		switch ( vme->data_size ) {
		case MXF_VME_D8:
			uint8_ptr = (uint8_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				uint8_ptr[i] = (uint8_t)
						epics_vme->temp_buffer[i];
			}
			break;
		case MXF_VME_D16:
			uint16_ptr = (uint16_t *) vme->data_pointer;

			for ( i = 0; i < vme->num_values; i++ ) {
				uint16_ptr[i] = (uint16_t)
						epics_vme->temp_buffer[i];

#if 0
				MX_DEBUG(-2,(
					"%s: epics_vme->temp_buffer[%d] = %ld",
					fname, i, epics_vme->temp_buffer[i]));

				MX_DEBUG(-2,("%s: uint16_ptr[%d] = %d",
					fname, i, (int) uint16_ptr[i]));
#endif
			}
			break;
		case MXF_VME_D32:
			if ( longs_are_32bits == FALSE ) {

				uint32_ptr = (uint32_t *) vme->data_pointer;

				for ( i = 0; i < vme->num_values; i++ ) {
					uint32_ptr[i] = (uint32_t)
							epics_vme->temp_buffer[i];
				}
			}
			break;
		}
	}

#if MX_EPICS_VME_DEBUG	
	if ( vme->num_values == 1 ) {
		if ( direction == MXF_EPICS_VME_OUTPUT ) {
			MX_EPICS_VME_DEBUG_PRINT("Output");
		} else {
			MX_EPICS_VME_DEBUG_PRINT("Input");
		}
	} else {
		if ( direction == MXF_EPICS_VME_OUTPUT ) {
			MX_EPICS_VME_MULTI_DEBUG_PRINT("Output");
		} else {
			MX_EPICS_VME_MULTI_DEBUG_PRINT("Input");
		}
	}
#endif
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_epics_vme_get_pointers( MX_VME *vme,
				MX_EPICS_VME **epics_vme,
				const char calling_fname[] )
{
	static const char fname[] = "mxi_epics_vme_get_pointers()";

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}
	if ( epics_vme == (MX_EPICS_VME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*epics_vme = (MX_EPICS_VME *) vme->record->record_type_struct;

	if ( *epics_vme == (MX_EPICS_VME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_VME pointer for record '%s' is NULL.",
				vme->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_epics_vme_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_epics_vme_create_record_structures()";

	MX_VME *vme;
	MX_EPICS_VME *epics_vme;

	/* Allocate memory for the necessary structures. */

	vme = (MX_VME *) malloc( sizeof(MX_VME) );

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VME structure." );
	}

	epics_vme = (MX_EPICS_VME *) malloc( sizeof(MX_EPICS_VME) );

	if ( epics_vme == (MX_EPICS_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_VME structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = vme;
	record->record_type_struct = epics_vme;
	record->class_specific_function_list = &mxi_epics_vme_vme_function_list;

	vme->record = record;
	epics_vme->record = record;

	epics_vme->temp_buffer = NULL;
	epics_vme->status_array = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_vme_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epics_vme_finish_record_initialization()";

	MX_VME *vme;
	MX_EPICS_VME *epics_vme = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = mx_vme_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vme = (MX_VME *) record->record_class_struct;

	mx_status = mxi_epics_vme_get_pointers( vme, &epics_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(epics_vme->addr_pv),
				"%s.ADDR", epics_vme->epics_record_name );

	mx_epics_pvname_init( &(epics_vme->ainc_pv),
				"%s.AINC", epics_vme->epics_record_name );

	mx_epics_pvname_init( &(epics_vme->amod_pv),
				"%s.AMOD", epics_vme->epics_record_name );

	mx_epics_pvname_init( &(epics_vme->dsiz_pv),
				"%s.DSIZ", epics_vme->epics_record_name );

	mx_epics_pvname_init( &(epics_vme->nuse_pv),
				"%s.NUSE", epics_vme->epics_record_name );

	mx_epics_pvname_init( &(epics_vme->proc_pv),
				"%s.PROC", epics_vme->epics_record_name );

	mx_epics_pvname_init( &(epics_vme->rdwt_pv),
				"%s.RDWT", epics_vme->epics_record_name );

	mx_epics_pvname_init( &(epics_vme->sarr_pv),
				"%s.SARR", epics_vme->epics_record_name );

	mx_epics_pvname_init( &(epics_vme->val_pv),
				"%s.VAL", epics_vme->epics_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_vme_delete_record( MX_RECORD *record )
{
	MX_EPICS_VME *epics_vme;

        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }

	epics_vme = (MX_EPICS_VME *) record->record_type_struct;

	if ( epics_vme != NULL ) {
		if ( epics_vme->status_array != NULL ) {
			free( epics_vme->status_array );

			epics_vme->status_array = NULL;
		}
		free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_vme_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_epics_vme_open()";

	MX_VME *vme;
	MX_EPICS_VME *epics_vme = NULL;
	char pvname[MXU_EPICS_PVNAME_LENGTH+1];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vme = (MX_VME *) record->record_class_struct;

	mx_status = mxi_epics_vme_get_pointers( vme, &epics_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the stored old values to values that are probably illegal. */

	epics_vme->old_address = MX_ULONG_MAX;
	epics_vme->old_address_mode = MX_ULONG_MAX;
	epics_vme->old_data_size = MX_ULONG_MAX;
	epics_vme->old_num_values = MX_ULONG_MAX;
	epics_vme->old_address_increment = MX_ULONG_MAX;

	/* Find out the maximum number of values that can be transferred
	 * in one request.
	 */

	sprintf( pvname, "%s.NMAX", epics_vme->epics_record_name );

	MX_DEBUG( 2,("%s: about to read from '%s'", fname, pvname ));

	mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1,
					&(epics_vme->max_epics_values) );

	MX_DEBUG( 2,("%s: read %ld from '%s'", fname,
				(long) epics_vme->max_epics_values, pvname));

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate memory for the EPICS temporary VME data transfer buffer. */

	if ( epics_vme->temp_buffer != NULL ) {
		free( epics_vme->temp_buffer );
	}

	epics_vme->temp_buffer = (int32_t *)
		malloc(epics_vme->max_epics_values * sizeof( int32_t ));

	if ( epics_vme->temp_buffer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory allocating a %ld element "
			"data transfer buffer for EPICS VME record '%s'.",
				(long) epics_vme->max_epics_values, record->name );
	}

	/* Allocate memory for the EPICS VME status array. */

	if ( epics_vme->status_array != NULL ) {
		free( epics_vme->status_array );
	}

	epics_vme->status_array = (unsigned char *)
		malloc(epics_vme->max_epics_values * sizeof( unsigned char ));

	if ( epics_vme->status_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory allocating a %ld element "
			"status array for EPICS VME record '%s'.",
				(long) epics_vme->max_epics_values, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_vme_resynchronize( MX_RECORD *record )
{
	return mxi_epics_vme_open( record );
}

/*===============*/

MX_EXPORT mx_status_type
mxi_epics_vme_input( MX_VME *vme )
{
	static const char fname[] = "mxi_epics_vme_input()";

	MX_EPICS_VME *epics_vme = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_vme_get_pointers( vme, &epics_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_vme_transfer_data( vme,
					epics_vme, MXF_EPICS_VME_INPUT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_vme_output( MX_VME *vme )
{
	static const char fname[] = "mxi_epics_vme_output()";

	MX_EPICS_VME *epics_vme = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_vme_get_pointers( vme, &epics_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_vme_transfer_data( vme,
					epics_vme, MXF_EPICS_VME_OUTPUT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_vme_multi_input( MX_VME *vme )
{
	static const char fname[] = "mxi_epics_vme_input()";

	MX_EPICS_VME *epics_vme = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_vme_get_pointers( vme, &epics_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_vme_transfer_data( vme,
					epics_vme, MXF_EPICS_VME_INPUT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_vme_multi_output( MX_VME *vme )
{
	static const char fname[] = "mxi_epics_vme_output()";

	MX_EPICS_VME *epics_vme = NULL;
	mx_status_type mx_status;

	mx_status = mxi_epics_vme_get_pointers( vme, &epics_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_epics_vme_transfer_data( vme,
					epics_vme, MXF_EPICS_VME_OUTPUT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epics_vme_get_parameter( MX_VME *vme )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epics_vme_set_parameter( MX_VME *vme )
{
	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EPICS */
