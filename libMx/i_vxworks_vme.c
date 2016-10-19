/*
 * Name:    i_vxworks_vme.c
 *
 * Purpose: MX driver for VME bus access for a VxWorks controlled VME crate.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003, 2006, 2010, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VXWORKS_VME_DEBUG	FALSE

#include <stdio.h>

#if defined(OS_VXWORKS)

#include <stdlib.h>
#include <vme.h>
#include <sysLib.h>
#include <vxLib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_vme.h"
#include "i_vxworks_vme.h"

MX_RECORD_FUNCTION_LIST mxi_vxworks_vme_record_function_list = {
	NULL,
	mxi_vxworks_vme_create_record_structures,
	mxi_vxworks_vme_finish_record_initialization,
	NULL,
	mxi_vxworks_vme_print_structure,
	mxi_vxworks_vme_open,
	mxi_vxworks_vme_close,
	NULL,
	mxi_vxworks_vme_resynchronize
};

MX_VME_FUNCTION_LIST mxi_vxworks_vme_vme_function_list = {
	mxi_vxworks_vme_input,
	mxi_vxworks_vme_output,
	mxi_vxworks_vme_multi_input,
	mxi_vxworks_vme_multi_output,
	mxi_vxworks_vme_get_parameter,
	mxi_vxworks_vme_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxi_vxworks_vme_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VME_STANDARD_FIELDS,
	MXI_VXWORKS_VME_STANDARD_FIELDS
};

long mxi_vxworks_vme_num_record_fields
		= sizeof( mxi_vxworks_vme_record_field_defaults )
			/ sizeof( mxi_vxworks_vme_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxi_vxworks_vme_rfield_def_ptr
		= &mxi_vxworks_vme_record_field_defaults[0];

/*--*/

#if MX_VXWORKS_VME_DEBUG

/* MX_VXWORKS_VME_DEBUG_WARNING_SUPPRESS is used to suppress uninitialized
 * variable warnings from GCC.
 */

#  define MX_VXWORKS_VME_DEBUG_PRINT \
	do { \
	    unsigned long value = 0; \
                                     \
	    switch( vme->data_size ) { \
	    case MXF_VME_D8:  \
		value = (unsigned long) *(uint8_t *) vme->data_pointer; \
		break; \
	    case MXF_VME_D16: \
		value = (unsigned long) *(uint16_t *) vme->data_pointer; \
		break; \
	    case MXF_VME_D32: \
		value = (unsigned long) *(uint32_t *) vme->data_pointer; \
		break; \
	    } \
	    MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value )); \
	} while(0)

#  define MX_VXWORKS_VME_MULTI_DEBUG_PRINT \
	do { \
	    unsigned long value = 0; \
                                     \
	    switch( vme->data_size ) { \
	    case MXF_VME_D8:  \
		value = (unsigned long) *(uint8_t *) vme->data_pointer; \
		break; \
	    case MXF_VME_D16: \
		value = (unsigned long) *(uint16_t *) vme->data_pointer; \
		break; \
	    case MXF_VME_D32: \
		value = (unsigned long) *(uint32_t *) vme->data_pointer; \
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

#  define MX_VXWORKS_VME_DEBUG_PRINT

#  define MX_VXWORKS_VME_MULTI_DEBUG_PRINT

#endif

/*==========================*/

static mx_status_type
mxi_vxworks_vme_get_pointers( MX_VME *vme,
			MX_VXWORKS_VME **vxworks_vme,
			const char calling_fname[] )
{
	static const char fname[] = "mxi_vxworks_vme_get_pointers()";

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( vxworks_vme == (MX_VXWORKS_VME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VXWORKS_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( vme->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*vxworks_vme = (MX_VXWORKS_VME *) vme->record->record_type_struct;

	if ( *vxworks_vme == (MX_VXWORKS_VME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VXWORKS_VME pointer for record '%s' is NULL.",
				vme->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_vxworks_vme_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_vxworks_vme_create_record_structures()";

	MX_VME *vme;
	MX_VXWORKS_VME *vxworks_vme;

	/* Allocate memory for the necessary structures. */

	vme = (MX_VME *) malloc( sizeof(MX_VME) );

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VME structure." );
	}

	vxworks_vme = (MX_VXWORKS_VME *) malloc( sizeof(MX_VXWORKS_VME) );

	if ( vxworks_vme == (MX_VXWORKS_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VXWORKS_VME structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = vme;
	record->record_type_struct = vxworks_vme;
	record->class_specific_function_list
			= &mxi_vxworks_vme_vme_function_list;

	vme->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_vme_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxi_vxworks_vme_print_structure()";

	MX_VME *vme;
	MX_VXWORKS_VME *vxworks_vme;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf( file, "VME parameters for interface '%s'\n\n", record->name );

	fprintf( file, "  vxworks_vme_flags = '%#lx'\n\n",
				vxworks_vme->vxworks_vme_flags );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_vxworks_vme_open()";

	MX_VME *vme;
	MX_VXWORKS_VME *vxworks_vme;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_vxworks_vme_close()";

	MX_VME *vme;
	MX_VXWORKS_VME *vxworks_vme;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_vxworks_vme_resynchronize()";

	MX_VME *vme;
	MX_VXWORKS_VME *vxworks_vme;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*===============*/

MX_EXPORT mx_status_type
mxi_vxworks_vme_input( MX_VME *vme )
{
	static const char fname[] = "mxi_vxworks_vme_input()";

	MX_VXWORKS_VME *vxworks_vme;
	int address_space;
	char *vme_local_address;
	STATUS vme_status;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		address_space = VME_AM_SUP_SHORT_IO;
		break;
	case MXF_VME_A24:
		address_space = VME_AM_STD_SUP_DATA;
		break;
	case MXF_VME_A32:
		address_space = VME_AM_EXT_SUP_DATA;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

	vme_status = sysBusToLocalAdrs( address_space,
					(char *) vme->address,
					&vme_local_address );

	if ( vme_status != OK ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "VME address space is unknown or the mapping is not possible." );
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		vme_status = vxMemProbe( vme_local_address, VX_READ, 1,
						(char *) vme->data_pointer );
		break;
	case MXF_VME_D16:
		vme_status = vxMemProbe( vme_local_address, VX_READ, 2,
						(char *) vme->data_pointer );
		break;
	case MXF_VME_D32:
		vme_status = vxMemProbe( vme_local_address, VX_READ, 4,
						(char *) vme->data_pointer );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_VXWORKS_VME_DEBUG_PRINT;

	if ( vme_status == OK ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		if ( vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS ) {
			return MX_SUCCESSFUL_RESULT;
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Bus error or misaligned data when "
			"reading from VME interface '%s', crate %lu, "
			"A%lu, D%lu, address %#lx, VME status = %#x",
				vme->record->name, vme->crate,
				vme->address_mode, vme->data_size,
				vme->address, vme_status );
	}
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_output( MX_VME *vme )
{
	static const char fname[] = "mxi_vxworks_vme_output()";

	MX_VXWORKS_VME *vxworks_vme;
	int address_space;
	char *vme_local_address;
	STATUS vme_status;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		address_space = VME_AM_SUP_SHORT_IO;
		break;
	case MXF_VME_A24:
		address_space = VME_AM_STD_SUP_DATA;
		break;
	case MXF_VME_A32:
		address_space = VME_AM_EXT_SUP_DATA;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

	vme_status = sysBusToLocalAdrs( address_space,
					(char *) vme->address,
					&vme_local_address );

	if ( vme_status != OK ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "VME address space is unknown or the mapping is not possible." );
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		vme_status = vxMemProbe( vme_local_address, VX_WRITE, 1,
						(char *) vme->data_pointer );
		break;
	case MXF_VME_D16:
		vme_status = vxMemProbe( vme_local_address, VX_WRITE, 2,
						(char *) vme->data_pointer );
		break;
	case MXF_VME_D32:
		vme_status = vxMemProbe( vme_local_address, VX_WRITE, 4,
						(char *) vme->data_pointer );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_VXWORKS_VME_DEBUG_PRINT;

	if ( vme_status == OK ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		if ( vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS ) {
			return MX_SUCCESSFUL_RESULT;
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Bus error or misaligned data when "
			"writing to VME interface '%s', crate %lu, "
			"A%lu, D%lu, address %#lx, VME status = %#x",
				vme->record->name, vme->crate,
				vme->address_mode, vme->data_size,
				vme->address, vme_status );
	}
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_multi_input( MX_VME *vme )
{
	static const char fname[] = "mxi_vxworks_vme_multi_input()";

	MX_VXWORKS_VME *vxworks_vme;
	STATUS vme_status;
	unsigned long i;
	int address_space;
	char *vme_local_address;
	uint8_t *uint8_ptr;
	uint16_t *uint16_ptr;
	uint32_t *uint32_ptr;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		address_space = VME_AM_SUP_SHORT_IO;
		break;
	case MXF_VME_A24:
		address_space = VME_AM_STD_SUP_DATA;
		break;
	case MXF_VME_A32:
		address_space = VME_AM_EXT_SUP_DATA;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

	vme_status = sysBusToLocalAdrs( address_space,
					(char *) vme->address,
					&vme_local_address );

	if ( vme_status != OK ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "VME address space is unknown or the mapping is not possible." );
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			vme_status = vxMemProbe( vme_local_address,
					VX_READ, 1, (char *) uint8_ptr );

			if ( vme_status != OK )
				break;

			uint8_ptr++;

			vme_local_address += vme->read_address_increment;
		}
		break;
	case MXF_VME_D16:
		uint16_ptr = vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			vme_status = vxMemProbe( vme_local_address,
					VX_READ, 2, (char *) uint16_ptr );

			if ( vme_status != OK )
				break;

			uint16_ptr++;

			vme_local_address += vme->read_address_increment;
		}
		break;
	case MXF_VME_D32:
		uint32_ptr = vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			vme_status = vxMemProbe( vme_local_address,
					VX_READ, 4, (char *) uint32_ptr );

			if ( vme_status != OK )
				break;

			uint32_ptr++;

			vme_local_address += vme->read_address_increment;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_VXWORKS_VME_MULTI_DEBUG_PRINT;

	if ( vme_status == OK ) {
		if ( i != vme->num_values ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Only %lu values were transferred out of %lu values requested "
		"for VME interface '%s'.",
				i, vme->num_values, vme->record->name );
		} else {
			return MX_SUCCESSFUL_RESULT;
		}
	} else {
		if ( vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS ) {
			return MX_SUCCESSFUL_RESULT;
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error reading from VME interface '%s', crate %lu, "
			"A%lu, D%lu, address %#lx, VME status = %#x",
				vme->record->name, vme->crate,
				vme->address_mode, vme->data_size,
				vme->address, vme_status );
	}
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_multi_output( MX_VME *vme )
{
	static const char fname[] = "mxi_vxworks_vme_multi_output()";

	MX_VXWORKS_VME *vxworks_vme;
	STATUS vme_status;
	unsigned long i;
	int address_space;
	char *vme_local_address;
	uint8_t *uint8_ptr;
	uint16_t *uint16_ptr;
	uint32_t *uint32_ptr;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->address_mode ) {
	case MXF_VME_A16:
		address_space = VME_AM_SUP_SHORT_IO;
		break;
	case MXF_VME_A24:
		address_space = VME_AM_STD_SUP_DATA;
		break;
	case MXF_VME_A32:
		address_space = VME_AM_EXT_SUP_DATA;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address space %lu.", vme->address_mode );
		break;
	}

	vme_status = sysBusToLocalAdrs( address_space,
					(char *) vme->address,
					&vme_local_address );

	if ( vme_status != OK ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "VME address space is unknown or the mapping is not possible." );
	}

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			vme_status = vxMemProbe( vme_local_address,
					VX_WRITE, 1, (char *) uint8_ptr );

			if ( vme_status != OK )
				break;

			uint8_ptr++;

			vme_local_address += vme->write_address_increment;
		}
		break;
	case MXF_VME_D16:
		uint16_ptr = vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			vme_status = vxMemProbe( vme_local_address,
					VX_WRITE, 2, (char *) uint16_ptr );

			if ( vme_status != OK )
				break;

			uint16_ptr++;

			vme_local_address += vme->write_address_increment;
		}
		break;
	case MXF_VME_D32:
		uint32_ptr = vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			vme_status = vxMemProbe( vme_local_address,
					VX_WRITE, 4, (char *) uint32_ptr );

			if ( vme_status != OK )
				break;

			uint32_ptr++;

			vme_local_address += vme->write_address_increment;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.", vme->data_size );
		break;
	}

	MX_VXWORKS_VME_MULTI_DEBUG_PRINT;

	if ( vme_status == OK ) {
		if ( i != vme->num_values ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Only %lu values were transferred out of %lu values requested "
		"for VME interface '%s'.",
				i, vme->num_values, vme->record->name );
		} else {
			return MX_SUCCESSFUL_RESULT;
		}
	} else {
		if ( vme->vme_flags & MXF_VME_IGNORE_BUS_ERRORS ) {
			return MX_SUCCESSFUL_RESULT;
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error writing to VME interface '%s', crate %lu, "
			"A%lu, D%lu, address %#lx, VME status = %#x",
				vme->record->name, vme->crate,
				vme->address_mode, vme->data_size,
				vme->address, vme_status );
	}
}

MX_EXPORT mx_status_type
mxi_vxworks_vme_get_parameter( MX_VME *vme )
{
	static const char fname[] = "mxi_vxworks_vme_get_parameter()";

	MX_VXWORKS_VME *vxworks_vme;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

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
mxi_vxworks_vme_set_parameter( MX_VME *vme )
{
	static const char fname[] = "mxi_vxworks_vme_set_parameter()";

	MX_VXWORKS_VME *vxworks_vme;
	mx_status_type mx_status;

	mx_status = mxi_vxworks_vme_get_pointers( vme, &vxworks_vme, fname );

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

#endif /* OS_VXWORKS */
