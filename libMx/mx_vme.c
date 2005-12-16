/*
 * Name:    mx_vme.c
 *
 * Purpose: VME interface access functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_vme.h"

#define DISPLAY_SINGLE_STEP_PROMPT \
	do { \
		if ( vme->vme_flags & MXF_VME_SINGLE_STEP ) { \
			mx_info_dialog( "Type a key to continue...", \
				"Click OK to continue...", "OK" ); \
		} \
	} while(0)

static mx_status_type
mx_vme_get_pointers( MX_RECORD *record,
			MX_VME **vme,
			MX_VME_FUNCTION_LIST **function_list,
			const char *calling_fname )
{
	const char fname[] = "mx_vme_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vme != (MX_VME **) NULL ) {

		*vme = (MX_VME *) record->record_class_struct;

		if ( *vme == (MX_VME *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VME pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}
	if ( function_list != (MX_VME_FUNCTION_LIST **) NULL ) {

		*function_list = (MX_VME_FUNCTION_LIST *)
					record->class_specific_function_list;

		if ( *function_list == (MX_VME_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_VME_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_vme_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mx_vme_finish_record_initialization()";

	MX_VME *vme;
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vme->crate = 0;
	vme->address = 0;
	vme->address_mode = MXF_VME_A16;
	vme->data_size = MXF_VME_D16;
	vme->num_values = 1;
	vme->read_address_increment = 1;
	vme->write_address_increment = 1;
	vme->data_pointer = NULL;
	vme->parameter_type = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_vme_in8( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address,
		mx_uint8_type *value )
{
	const char fname[] = "mx_vme_in8()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->input;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"8-bit input from record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->data_size = MXF_VME_D8;
	vme->num_values = 1;
	vme->data_pointer = value;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_in16( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address,
		mx_uint16_type *value )
{
	const char fname[] = "mx_vme_in16()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->input;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"16-bit input from record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->data_size = MXF_VME_D16;
	vme->num_values = 1;
	vme->data_pointer = value;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_in32( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address,
		mx_uint32_type *value )
{
	const char fname[] = "mx_vme_in32()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->input;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"32-bit input from record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->data_size = MXF_VME_D32;
	vme->num_values = 1;
	vme->data_pointer = value;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_out8( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address,
		mx_uint8_type value )
{
	const char fname[] = "mx_vme_out8()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_uint8_type local_value;
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->output;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"8-bit output for record '%s' is not supported.",
			record->name );
	}

	local_value = value;

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->data_size = MXF_VME_D8;
	vme->num_values = 1;
	vme->data_pointer = &local_value;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_out16( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address,
		mx_uint16_type value )
{
	const char fname[] = "mx_vme_out16()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_uint16_type local_value;
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->output;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"16-bit output for record '%s' is not supported.",
			record->name );
	}

	local_value = value;

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->data_size = MXF_VME_D16;
	vme->num_values = 1;
	vme->data_pointer = &local_value;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_out32( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address,
		mx_uint32_type value )
{
	const char fname[] = "mx_vme_out32()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_uint32_type local_value;
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->output;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"32-bit output for record '%s' is not supported.",
			record->name );
	}

	local_value = value;

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->data_size = MXF_VME_D32;
	vme->num_values = 1;
	vme->data_pointer = &local_value;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_multi_in8( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address_increment,
		unsigned long address,
		unsigned long num_values,
		mx_uint8_type *value_array )
{
	const char fname[] = "mx_vme_multi_in8()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->multi_input;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"8-bit multi-word input from record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->read_address_increment = address_increment;
	vme->data_size = MXF_VME_D8;
	vme->num_values = num_values;
	vme->data_pointer = value_array;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_multi_in16( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address_increment,
		unsigned long address,
		unsigned long num_values,
		mx_uint16_type *value_array )
{
	const char fname[] = "mx_vme_multi_in16()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->multi_input;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"16-bit multi-word input from record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->read_address_increment = address_increment;
	vme->data_size = MXF_VME_D16;
	vme->num_values = num_values;
	vme->data_pointer = value_array;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_multi_in32( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address_increment,
		unsigned long address,
		unsigned long num_values,
		mx_uint32_type *value_array )
{
	const char fname[] = "mx_vme_multi_in32()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->multi_input;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"32-bit multi-word input from record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->read_address_increment = address_increment;
	vme->data_size = MXF_VME_D32;
	vme->num_values = num_values;
	vme->data_pointer = value_array;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_multi_out8( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address_increment,
		unsigned long address,
		unsigned long num_values,
		mx_uint8_type *value_array )
{
	const char fname[] = "mx_vme_multi_out8()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->multi_output;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"8-bit multi-word output for record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->write_address_increment = address_increment;
	vme->data_size = MXF_VME_D8;
	vme->num_values = num_values;
	vme->data_pointer = value_array;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_multi_out16( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address_increment,
		unsigned long address,
		unsigned long num_values,
		mx_uint16_type *value_array )
{
	const char fname[] = "mx_vme_multi_out16()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->multi_output;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"16-bit multi-word output for record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->write_address_increment = address_increment;
	vme->data_size = MXF_VME_D16;
	vme->num_values = num_values;
	vme->data_pointer = value_array;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_multi_out32( MX_RECORD *record,
		unsigned long crate,
		unsigned long address_mode,
		unsigned long address_increment,
		unsigned long address,
		unsigned long num_values,
		mx_uint32_type *value_array )
{
	const char fname[] = "mx_vme_multi_out32()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->multi_output;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"32-bit multi-word output for record '%s' is not supported.",
			record->name );
	}

	vme->crate = crate;
	vme->address = address;
	vme->address_mode = address_mode;
	vme->write_address_increment = address_increment;
	vme->data_size = MXF_VME_D32;
	vme->num_values = num_values;
	vme->data_pointer = value_array;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_get_read_address_increment( MX_RECORD *record,
				unsigned long crate,
				unsigned long *address_increment )
{
	const char fname[] = "mx_vme_get_read_address_increment()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"The get_parameter function is not supported for record '%s'.",
			record->name );
	}

	vme->parameter_type = MXLV_VME_READ_ADDRESS_INCREMENT;

	mx_status = ( *fptr ) ( vme );

	if ( address_increment != NULL ) {
		*address_increment = vme->read_address_increment;
	}

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_set_read_address_increment( MX_RECORD *record,
				unsigned long crate,
				unsigned long address_increment )
{
	const char fname[] = "mx_vme_set_read_address_increment()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"The set_parameter function is not supported for record '%s'.",
			record->name );
	}

	vme->parameter_type = MXLV_VME_READ_ADDRESS_INCREMENT;

	vme->read_address_increment = address_increment;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_get_write_address_increment( MX_RECORD *record,
				unsigned long crate,
				unsigned long *address_increment )
{
	const char fname[] = "mx_vme_get_write_address_increment()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"The get_parameter function is not supported for record '%s'.",
			record->name );
	}

	vme->parameter_type = MXLV_VME_WRITE_ADDRESS_INCREMENT;

	mx_status = ( *fptr ) ( vme );

	if ( address_increment != NULL ) {
		*address_increment = vme->write_address_increment;
	}

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_set_write_address_increment( MX_RECORD *record,
				unsigned long crate,
				unsigned long address_increment )
{
	const char fname[] = "mx_vme_set_write_address_increment()";

	MX_VME *vme;
	MX_VME_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_VME * );
	mx_status_type mx_status;

	mx_status = mx_vme_get_pointers( record, &vme, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"The set_parameter function is not supported for record '%s'.",
			record->name );
	}

	vme->parameter_type = MXLV_VME_WRITE_ADDRESS_INCREMENT;

	vme->write_address_increment = address_increment;

	mx_status = ( *fptr ) ( vme );

	DISPLAY_SINGLE_STEP_PROMPT;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_vme_parse_address_mode( const char *mode_name, unsigned long *address_mode )
{
	const char fname[] = "mx_vme_parse_address_mode()";

	char address_mode_name[8];
	int c, i, length;

	if ( mode_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The mode_name pointer passed was NULL." );
	}
	if ( address_mode == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The address_mode pointer passed was NULL." );
	}

	/* Copy the mode name and force it to upper case. */

	strlcpy( address_mode_name, mode_name, sizeof(address_mode_name) );

	length = strlen( address_mode_name );

	for ( i = 0; i < length; i++ ) {
		c = address_mode_name[i];

		if ( islower(c) ) {
			address_mode_name[i] = toupper(c);
		}
	}

	/* Figure out which address mode this is. */

	if ( strcmp( address_mode_name, "A16" ) == 0 ) {
		*address_mode = MXF_VME_A16;
	} else if ( strcmp( address_mode_name, "A24" ) == 0 ) {
		*address_mode = MXF_VME_A24;
	} else if ( strcmp( address_mode_name, "A32" ) == 0 ) {
		*address_mode = MXF_VME_A32;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The VME address mode name '%s' is illegal.  "
		"The only supported modes are A16, A24, and A32.",
			mode_name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_vme_parse_data_size( const char *size_name, unsigned long *data_size )
{
	const char fname[] = "mx_vme_parse_data_size()";

	char data_size_name[8];
	int c, i, length;

	if ( size_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The size_name pointer passed was NULL." );
	}
	if ( data_size == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_size pointer passed was NULL." );
	}

	/* Copy the size name and force it to upper case. */

	strlcpy( data_size_name, size_name, sizeof(data_size_name) );

	length = strlen( data_size_name );

	for ( i = 0; i < length; i++ ) {
		c = data_size_name[i];

		if ( islower(c) ) {
			data_size_name[i] = toupper(c);
		}
	}

	/* Figure out which address mode this is. */

	if ( strcmp( data_size_name, "D8" ) == 0 ) {
		*data_size = MXF_VME_D8;
	} else if ( strcmp( data_size_name, "D16" ) == 0 ) {
		*data_size = MXF_VME_D16;
	} else if ( strcmp( data_size_name, "D32" ) == 0 ) {
		*data_size = MXF_VME_D32;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The VME address mode name '%s' is illegal.  "
		"The only supported modes are D8, D16, and D32.",
			size_name );
	}

	return MX_SUCCESSFUL_RESULT;
}

