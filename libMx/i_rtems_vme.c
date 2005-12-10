/*
 * Name:    i_rtems_vme.c
 *
 * Purpose: MX driver for VME bus access under RTEMS.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_RTEMS_VME_DEBUG	FALSE

#include <stdio.h>

#if defined(OS_RTEMS) && defined(HAVE_RTEMS_VME)

#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_vme.h"
#include "i_rtems_vme.h"

#define TWO_TO_THE_16TH_POWER	65536UL
#define TWO_TO_THE_24TH_POWER	16777216UL
#define TWO_TO_THE_32ND_POWER	4294967296UL

MX_RECORD_FUNCTION_LIST mxi_rtems_vme_record_function_list = {
	NULL,
	mxi_rtems_vme_create_record_structures,
	mxi_rtems_vme_finish_record_initialization
};

MX_VME_FUNCTION_LIST mxi_rtems_vme_vme_function_list = {
	mxi_rtems_vme_input,
	mxi_rtems_vme_output,
	mxi_rtems_vme_multi_input,
	mxi_rtems_vme_multi_output,
	mxi_rtems_vme_get_parameter,
	mxi_rtems_vme_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxi_rtems_vme_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VME_STANDARD_FIELDS,
	MXI_RTEMS_VME_STANDARD_FIELDS
};

long mxi_rtems_vme_num_record_fields
		= sizeof( mxi_rtems_vme_record_field_defaults )
			/ sizeof( mxi_rtems_vme_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxi_rtems_vme_rfield_def_ptr
		= &mxi_rtems_vme_record_field_defaults[0];

/*--*/

#if MX_RTEMS_VME_DEBUG

#  define MX_RTEMS_VME_DEBUG_PRINT \
	do { \
	    unsigned long value = 0; \
                                     \
	    switch( vme->data_size ) { \
	    case MXF_VME_D8:  \
		value = (unsigned long) *(mx_uint8_type *) vme->data_pointer; \
		break; \
	    case MXF_VME_D16: \
		value = (unsigned long) *(mx_uint16_type *) vme->data_pointer; \
		break; \
	    case MXF_VME_D32: \
		value = (unsigned long) *(mx_uint32_type *) vme->data_pointer; \
		break; \
	    } \
	    MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value )); \
	} while(0)

#  define MX_RTEMS_VME_MULTI_DEBUG_PRINT \
	do { \
	    unsigned long value = 0; \
                                     \
	    switch( vme->data_size ) { \
	    case MXF_VME_D8:  \
		value = (unsigned long) *(mx_uint8_type *) vme->data_pointer; \
		break; \
	    case MXF_VME_D16: \
		value = (unsigned long) *(mx_uint16_type *) vme->data_pointer; \
		break; \
	    case MXF_VME_D32: \
		value = (unsigned long) *(mx_uint32_type *) vme->data_pointer; \
		break; \
	    } \
	MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx (%lu values)", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value, \
			vme->num_values )); \
	} while(0)
#else

#  define MX_RTEMS_VME_DEBUG_PRINT

#  define MX_RTEMS_VME_MULTI_DEBUG_PRINT

#endif

/*==========================*/

static mx_status_type
mxi_rtems_vme_get_pointers( MX_VME *vme,
			MX_RTEMS_VME **rtems_vme,
			const char calling_fname[] )
{
	static const char fname[] = "mxi_rtems_vme_get_pointers()";

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( rtems_vme == (MX_RTEMS_VME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RTEMS_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( vme->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*rtems_vme = (MX_RTEMS_VME *) vme->record->record_type_struct;

	if ( *rtems_vme == (MX_RTEMS_VME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RTEMS_VME pointer for record '%s' is NULL.",
				vme->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

#if defined(MX_RTEMS_BSP_MVME2307)
#  include <bsp/VME.h>
#endif

static mx_status_type
mxi_rtems_vme_bus_to_local_address( MX_RTEMS_VME *rtems_vme,
					int address_mode,
					unsigned long bus_address,
					void **local_address )
{
	static const char fname[] = "mxi_rtems_vme_bus_to_local_address()";

#if defined(MX_RTEMS_BSP_MVME2307)

	/* PowerPC PCI to VME Universe chip. */

	switch( address_mode ) {
	case MXF_VME_A16:
		*local_address = (void *) BSP_vme2local_A16_fast( bus_address );
		break;
	case MXF_VME_A24:
		*local_address = (void *) BSP_vme2local_A24_fast( bus_address );
		break;
	case MXF_VME_A32:
		*local_address = (void *) BSP_vme2local_A32_fast( bus_address );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address mode A%d requested for record '%s'.",
			address_mode, rtems_vme->record->name );
		break;
	}

#elif defined(MX_RTEMS_BSP_MVME162)

	switch( address_mode ) {
	case MXF_VME_A16:
		*local_address = 
			(void *) (0xFFFF0000 + ( bus_address & 0xFFFF ));
		break;
	case MXF_VME_A24:
		*local_address =
			(void *) (0xFF000000 + ( bus_address & 0xFFFFFF ));
		break;
	case MXF_VME_A32:
		*local_address = (void *) bus_address;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address mode A%d requested for record '%s'.",
			address_mode, rtems_vme->record->name );
		break;
	}

#else

#error No mechanism has been provided for translating from VME bus to local addresses.

#endif

/********************************************************************
 * FIXME: The following is present only as a reminder that I should *
 * figure out someday why it does not work.  (WML)                  *
 ********************************************************************/

#if 0   /* Meant for MX_RTEMS_BSP_MVME2307, but it doesn't work. */

	/* PowerPC PCI to VME Universe chip. */

	unsigned long ulong_local_address;
	unsigned long bsp_address_mode;
	int vme_status;

	switch( address_mode ) {
	case MXF_VME_A16:
		bsp_address_mode = VME_AM_SUP_SHORT_IO;
		break;
	case MXF_VME_A24:
		bsp_address_mode = VME_AM_STD_SUP_DATA;
		break;
	case MXF_VME_A32:
		bsp_address_mode = VME_AM_EXT_SUP_DATA;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address mode A%d requested for record '%s'.",
			address_mode, rtems_vme->record->name );
		break;
	}

#  if 1
	vme_status = BSP_vme2local_adrs( bsp_address_mode,
					bus_address, &ulong_local_address );
#  else
	vme_status = vmeUniverseXlateAddr(1, 0, bsp_address_mode,
				bus_address, &ulong_local_address );

	ulong_local_address += PCI_MEM_BASE;
#  endif

	if ( vme_status != 0 ) {
		switch( vme_status ) {
		case -1:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"A%d address %#lx not found in any PCI-to-VME bridge "
			"port for VME record '%s'.", address_mode,
				bus_address, rtems_vme->record->name );
			break;
		case -2:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Invalid VME bus address modifier %#lx specified "
			"for record '%s'.",
				bsp_address_mode, rtems_vme->record->name );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Error converting VME A%d bus address %#lx to "
			"local address for record '%s'.  VME status = %d.",
	    			address_mode, bus_address, 
				rtems_vme->record->name, vme_status );
			break;
		}
	}

	*local_address = (void *) ulong_local_address;

#endif /* End of 'Meant for MX_RTEMS_BSP_MVME2307'. */

/********************************************************************/

#if 1
	MX_DEBUG(-2,("%s: A%d bus address = %#lx, local address = %p",
		fname, address_mode, bus_address, *local_address ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_rtems_vme_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_rtems_vme_create_record_structures()";

	MX_VME *vme;
	MX_RTEMS_VME *rtems_vme;

	/* Allocate memory for the necessary structures. */

	vme = (MX_VME *) malloc( sizeof(MX_VME) );

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VME structure." );
	}

	rtems_vme = (MX_RTEMS_VME *) malloc( sizeof(MX_RTEMS_VME) );

	if ( rtems_vme == (MX_RTEMS_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RTEMS_VME structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = vme;
	record->record_type_struct = rtems_vme;
	record->class_specific_function_list
			= &mxi_rtems_vme_vme_function_list;

	vme->record = record;
	rtems_vme->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_rtems_vme_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_vme_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_rtems_vme_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxi_rtems_vme_print_structure()";

	MX_VME *vme;
	MX_RTEMS_VME *rtems_vme;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_rtems_vme_get_pointers( vme, &rtems_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf( file, "VME parameters for interface '%s'\n\n", record->name );

	fprintf( file, "  rtems_vme_flags = '%#lx'\n\n",
				rtems_vme->rtems_vme_flags );

	return MX_SUCCESSFUL_RESULT;
}

/*===============*/

MX_EXPORT mx_status_type
mxi_rtems_vme_input( MX_VME *vme )
{
	static const char fname[] = "mxi_rtems_vme_input()";

	MX_RTEMS_VME *rtems_vme;
	void *vme_local_address;
	mx_uint8_type *uint8_input_ptr;
	mx_uint16_type *uint16_input_ptr;
	mx_uint32_type *uint32_input_ptr;
	mx_status_type mx_status;
	unsigned long *ulong_ptr;

	mx_status = mxi_rtems_vme_get_pointers( vme, &rtems_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_rtems_vme_bus_to_local_address( rtems_vme,
						vme->address_mode,
						vme->address,
						&vme_local_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ulong_ptr = (unsigned long *) vme_local_address;

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_input_ptr = (mx_uint8_type *) vme->data_pointer;

		*uint8_input_ptr = *( (mx_uint8_type *) vme_local_address );
		break;
	case MXF_VME_D16:
		uint16_input_ptr = (mx_uint16_type *) vme->data_pointer;

		*uint16_input_ptr = *( (mx_uint16_type *) vme_local_address );
		break;
	case MXF_VME_D32:
		uint32_input_ptr = (mx_uint32_type *) vme->data_pointer;

		*uint32_input_ptr = *( (mx_uint32_type *) vme_local_address );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_RTEMS_VME_DEBUG_PRINT;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_rtems_vme_output( MX_VME *vme )
{
	static const char fname[] = "mxi_rtems_vme_output()";

	MX_RTEMS_VME *rtems_vme;
	void *vme_local_address;
	mx_uint8_type *uint8_output_ptr;
	mx_uint16_type *uint16_output_ptr;
	mx_uint32_type *uint32_output_ptr;
	mx_status_type mx_status;

	mx_status = mxi_rtems_vme_get_pointers( vme, &rtems_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_rtems_vme_bus_to_local_address( rtems_vme,
						vme->address_mode,
						vme->address,
						&vme_local_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_output_ptr = (mx_uint8_type *) vme_local_address;

		*uint8_output_ptr = *( (mx_uint8_type *) vme->data_pointer );
		break;
	case MXF_VME_D16:
		uint16_output_ptr = (mx_uint16_type *) vme_local_address;

		*uint16_output_ptr = *( (mx_uint16_type *) vme->data_pointer );
		break;
	case MXF_VME_D32:
		uint32_output_ptr = (mx_uint32_type *) vme_local_address;

		*uint32_output_ptr = *( (mx_uint32_type *) vme->data_pointer );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_RTEMS_VME_DEBUG_PRINT;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_rtems_vme_multi_input( MX_VME *vme )
{
	static const char fname[] = "mxi_rtems_vme_multi_input()";

	MX_RTEMS_VME *rtems_vme;
	unsigned long i;
	void *vme_local_address;
	mx_uint8_type *uint8_ptr;
	mx_uint16_type *uint16_ptr;
	mx_uint32_type *uint32_ptr;
	mx_uint8_type *uint8_local_address;
	mx_uint16_type *uint16_local_address;
	mx_uint32_type *uint32_local_address;
	unsigned long address_increment, data_size_in_bytes;
	mx_status_type mx_status;

	mx_status = mxi_rtems_vme_get_pointers( vme, &rtems_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_rtems_vme_bus_to_local_address( rtems_vme,
						vme->address_mode,
						vme->address,
						&vme_local_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->data_size ) {
	case MXF_VME_D8:
		data_size_in_bytes = sizeof(mx_uint8_type) / sizeof(char);
		break;
	case MXF_VME_D16:
		data_size_in_bytes = sizeof(mx_uint16_type) / sizeof(char);
		break;
	case MXF_VME_D32:
		data_size_in_bytes = sizeof(mx_uint32_type) / sizeof(char);
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	if ( ( vme->read_address_increment % data_size_in_bytes ) != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Requested VME read address increment %lu for record "
			"'%s' is not a multiple of the VME data size in "
			"bytes %lu for this transfer.",
				vme->read_address_increment,
				vme->record->name,
				data_size_in_bytes );
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = (mx_uint8_type *) vme->data_pointer;

		uint8_local_address = (mx_uint8_type *) vme_local_address;

		if ( vme->read_address_increment == sizeof(mx_uint8_type) ) {

			memcpy( uint8_local_address, uint8_ptr,
				vme->num_values * data_size_in_bytes );

		} else {
			address_increment = vme->read_address_increment /
						sizeof(mx_uint8_type);

			for ( i = 0; i < vme->num_values; i++ ) {
				*uint8_ptr = *uint8_local_address;

				uint8_ptr++;

				uint8_local_address += address_increment;
			}
		}
		break;
	case MXF_VME_D16:
		uint16_ptr = (mx_uint16_type *) vme->data_pointer;

		uint16_local_address = (mx_uint16_type *) vme_local_address;

		if ( vme->read_address_increment == sizeof(mx_uint16_type) ) {

			memcpy( uint16_local_address, uint16_ptr,
				vme->num_values * data_size_in_bytes );

		} else {
			address_increment = vme->read_address_increment /
						sizeof(mx_uint16_type);

			for ( i = 0; i < vme->num_values; i++ ) {
				*uint16_ptr = *uint16_local_address;

				uint16_ptr++;

				uint16_local_address += address_increment;
			}
		}
		break;
	case MXF_VME_D32:
		uint32_ptr = (mx_uint32_type *) vme->data_pointer;

		uint32_local_address = (mx_uint32_type *) vme_local_address;

		if ( vme->read_address_increment == sizeof(mx_uint32_type) ) {

			memcpy( uint32_local_address, uint32_ptr,
				vme->num_values * data_size_in_bytes );

		} else {
			address_increment = vme->read_address_increment /
						sizeof(mx_uint32_type);

			for ( i = 0; i < vme->num_values; i++ ) {
				*uint32_ptr = *uint32_local_address;

				uint32_ptr++;

				uint32_local_address += address_increment;
			}
		}
		break;
	}

	MX_RTEMS_VME_MULTI_DEBUG_PRINT;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_rtems_vme_multi_output( MX_VME *vme )
{
	static const char fname[] = "mxi_rtems_vme_multi_output()";

	MX_RTEMS_VME *rtems_vme;
	unsigned long i;
	void *vme_local_address;
	mx_uint8_type *uint8_ptr;
	mx_uint16_type *uint16_ptr;
	mx_uint32_type *uint32_ptr;
	mx_uint8_type *uint8_local_address;
	mx_uint16_type *uint16_local_address;
	mx_uint32_type *uint32_local_address;
	unsigned long address_increment, data_size_in_bytes;
	mx_status_type mx_status;

	mx_status = mxi_rtems_vme_get_pointers( vme, &rtems_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_rtems_vme_bus_to_local_address( rtems_vme,
						vme->address_mode,
						vme->address,
						&vme_local_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->data_size ) {
	case MXF_VME_D8:
		data_size_in_bytes = sizeof(mx_uint8_type) / sizeof(char);
		break;
	case MXF_VME_D16:
		data_size_in_bytes = sizeof(mx_uint16_type) / sizeof(char);
		break;
	case MXF_VME_D32:
		data_size_in_bytes = sizeof(mx_uint32_type) / sizeof(char);
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	if ( ( vme->read_address_increment % data_size_in_bytes ) != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Requested VME read address increment %lu for record "
			"'%s' is not a multiple of the VME data size in "
			"bytes %lu for this transfer.",
				vme->read_address_increment,
				vme->record->name,
				data_size_in_bytes );
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = (mx_uint8_type *) vme->data_pointer;

		uint8_local_address = (mx_uint8_type *) vme_local_address;

		if ( vme->read_address_increment == sizeof(mx_uint8_type) ) {

			memcpy( uint8_ptr, uint8_local_address,
				vme->num_values * data_size_in_bytes );

		} else {
			address_increment = vme->read_address_increment /
						sizeof(mx_uint8_type);

			for ( i = 0; i < vme->num_values; i++ ) {
				*uint8_local_address = *uint8_ptr;

				uint8_ptr++;

				uint8_local_address += address_increment;
			}
		}
		break;
	case MXF_VME_D16:
		uint16_ptr = (mx_uint16_type *) vme->data_pointer;

		uint16_local_address = (mx_uint16_type *) vme_local_address;

		if ( vme->read_address_increment == sizeof(mx_uint16_type) ) {

			memcpy( uint16_ptr, uint16_local_address,
				vme->num_values * data_size_in_bytes );

		} else {
			address_increment = vme->read_address_increment /
						sizeof(mx_uint16_type);

			for ( i = 0; i < vme->num_values; i++ ) {
				*uint16_local_address = *uint16_ptr;

				uint16_ptr++;

				uint16_local_address += address_increment;
			}
		}
		break;
	case MXF_VME_D32:
		uint32_ptr = (mx_uint32_type *) vme->data_pointer;

		uint32_local_address = (mx_uint32_type *) vme_local_address;

		if ( vme->read_address_increment == sizeof(mx_uint32_type) ) {

			memcpy( uint32_ptr, uint32_local_address,
				vme->num_values * data_size_in_bytes );

		} else {
			address_increment = vme->read_address_increment /
						sizeof(mx_uint32_type);

			for ( i = 0; i < vme->num_values; i++ ) {
				*uint32_local_address = *uint32_ptr;

				uint32_ptr++;

				uint32_local_address += address_increment;
			}
		}
		break;
	}

	MX_RTEMS_VME_MULTI_DEBUG_PRINT;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_rtems_vme_get_parameter( MX_VME *vme )
{
	static const char fname[] = "mxi_rtems_vme_get_parameter()";

	MX_RTEMS_VME *rtems_vme;
	mx_status_type mx_status;

	mx_status = mxi_rtems_vme_get_pointers( vme, &rtems_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for VME interface '%s' for parameter type '%s' (%d).",
		fname, vme->record->name,
		mx_get_field_label_string( vme->record, vme->parameter_type ),
		vme->parameter_type));

	switch( vme->parameter_type ) {
	case MXLV_VME_READ_ADDRESS_INCREMENT:
	case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		/* The controller itself does not have a place to store
		 * this information, so we just return the value we have
		 * stored locally.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the MX driver "
		"for VME interface '%s'.",
			mx_get_field_label_string(
				vme->record, vme->parameter_type ),
			vme->parameter_type, vme->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_rtems_vme_set_parameter( MX_VME *vme )
{
	static const char fname[] = "mxi_rtems_vme_set_parameter()";

	MX_RTEMS_VME *rtems_vme;
	mx_status_type mx_status;

	mx_status = mxi_rtems_vme_get_pointers( vme, &rtems_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for VME interface '%s' for parameter type '%s' (%d).",
		fname, vme->record->name,
		mx_get_field_label_string( vme->record, vme->parameter_type ),
		vme->parameter_type));

	switch( vme->parameter_type ) {
	case MXLV_VME_READ_ADDRESS_INCREMENT:
	case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		/* The controller itself does not have a place to store
		 * this information, so we just store the information
		 * locally.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the MX driver "
		"for VME interface '%s'.",
			mx_get_field_label_string(
				vme->record, vme->parameter_type ),
			vme->parameter_type, vme->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_RTEMS */
