/*
 * Name:    mx_driver_tables.c
 *
 * Purpose: Describes the data structures used to maintain the lists
 *          of superclass, class, and type drivers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2008-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_osdef.h"
#include "mx_stdint.h"

#include "mx_driver.h"

/* -- Define lists that relate types to classes and to function lists. -- */

  /****************** Record Superclasses ********************/

static MX_DRIVER mx_superclass_list[] = {
{"list_head_sclass", 0, 0, MXR_LIST_HEAD,     NULL, NULL, NULL, NULL, NULL},
{"interface",        0, 0, MXR_INTERFACE,     NULL, NULL, NULL, NULL, NULL},
{"device",           0, 0, MXR_DEVICE,        NULL, NULL, NULL, NULL, NULL},
{"scan",             0, 0, MXR_SCAN,          NULL, NULL, NULL, NULL, NULL},
{"variable",         0, 0, MXR_VARIABLE,      NULL, NULL, NULL, NULL, NULL},
{"server",           0, 0, MXR_SERVER,        NULL, NULL, NULL, NULL, NULL},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

  /********************* Record Classes **********************/

static MX_DRIVER mx_class_list[] = {

{"list_head_class", 0, MXL_LIST_HEAD,      MXR_LIST_HEAD,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Interface classes ================== */

{"rs232",          0, MXI_RS232,          MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"gpib",           0, MXI_GPIB,           MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"camac",          0, MXI_CAMAC,          MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"controller",     0, MXI_CONTROLLER,     MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"generic",        0, MXI_CONTROLLER,     MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"portio",         0, MXI_PORTIO,         MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"vme",            0, MXI_VME,            MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"mmio",           0, MXI_MMIO,           MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"modbus",         0, MXI_MODBUS,         MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"usb",            0, MXI_USB,            MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"camera_link",    0, MXI_CAMERA_LINK,    MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},

  /* =================== Device classes =================== */

{"analog_input",   0, MXC_ANALOG_INPUT,   MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"analog_output",  0, MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"digital_input",  0, MXC_DIGITAL_INPUT,  MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"digital_output", 0, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"motor",          0, MXC_MOTOR,          MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"encoder",        0, MXC_ENCODER,        MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"scaler",         0, MXC_SCALER,         MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"timer",          0, MXC_TIMER,          MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"amplifier",      0, MXC_AMPLIFIER,      MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"relay",          0, MXC_RELAY,          MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mca",            0, MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mce",            0, MXC_MULTICHANNEL_ENCODER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcs",            0, MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"table",          0, MXC_TABLE,          MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"autoscale",      0, MXC_AUTOSCALE,      MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"pulse_generator",0, MXC_PULSE_GENERATOR, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"sca",            0, MXC_SINGLE_CHANNEL_ANALYZER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"sample_changer", 0, MXC_SAMPLE_CHANGER, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcai",           0, MXC_MULTICHANNEL_ANALOG_INPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcao",           0, MXC_MULTICHANNEL_ANALOG_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcdi",           0, MXC_MULTICHANNEL_DIGITAL_INPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"mcdo",           0, MXC_MULTICHANNEL_DIGITAL_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"ptz",            0, MXC_PAN_TILT_ZOOM,  MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"area_detector",  0, MXC_AREA_DETECTOR, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"video_input",    0, MXC_VIDEO_INPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"video_output",   0, MXC_VIDEO_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"waveform_input", 0, MXC_WAVEFORM_INPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
{"waveform_output",0, MXC_WAVEFORM_OUTPUT, MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},

  /* ==================== Scan classes ==================== */

{"linear_scan",    0, MXS_LINEAR_SCAN,    MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},
{"list_scan",      0, MXS_LIST_SCAN,      MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},
{"xafs_scan_class",0, MXS_XAFS_SCAN,      MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},
{"quick_scan",     0, MXS_QUICK_SCAN,     MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},
{"ad_scan_class",  0, MXS_AREA_DETECTOR_SCAN, MXR_SCAN,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Variable classes ================== */

{"inline",         0, MXV_INLINE,         MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"net_variable",   0, MXV_NETWORK,        MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"epics_variable", 0, MXV_EPICS,          MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"calc",           0, MXV_CALC,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"pmac",           0, MXV_PMAC,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"spec_property",  0, MXV_SPEC,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"bluice_variable",0, MXV_BLUICE,         MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"file",           0, MXV_FILE,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"u500",           0, MXV_U500,           MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Server classes ================== */

{"network",        0, MXN_NETWORK_SERVER, MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},
{"spec",           0, MXN_SPEC,           MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},
{"bluice",         0, MXN_BLUICE,         MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};


/* -- mx_type_list is now defined in another file. -- */

extern MX_DRIVER mx_type_list[];

#if 0

/* -- Define the list of record types. -- */

MX_DRIVER *mx_list_of_types[] = {
	mx_superclass_list,
	mx_class_list,
	mx_type_list,
	NULL
};

MX_EXPORT MX_DRIVER **
mx_get_driver_lists( void )
{
	MX_DRIVER **ptr;

	ptr = mx_list_of_types;

	return ptr;
}

#endif

/*-----*/

static MX_DRIVER *mx_driver_list = NULL;

/*-----*/

MX_EXPORT MX_DRIVER *
mx_get_driver_by_name( char *driver_name )
{
	MX_DRIVER *current_driver;

	if ( driver_name == NULL ) {
		return mx_driver_list;
	}

	current_driver = mx_driver_list;

	while ( current_driver != (MX_DRIVER *) NULL ) {

		if ( strcmp( current_driver->name, driver_name ) == 0 ){
			break;
		}

		current_driver = current_driver->next_driver;
	}

	return current_driver;
}

MX_EXPORT MX_DRIVER *
mx_get_driver_by_type( long mx_type )
{
	MX_DRIVER *current_driver;

	current_driver = mx_driver_list;

	while ( current_driver != (MX_DRIVER *) NULL ) {

		if ( current_driver->mx_type == mx_type ) {
			break;
		}

		current_driver = current_driver->next_driver;
	}

	return current_driver;
}

MX_EXPORT MX_DRIVER *
mx_get_driver_for_record( MX_RECORD *record )
{
	static const char fname[] = "mx_get_driver_for_record()";

	MX_RECORD_FIELD *field;
	MX_DRIVER *driver;

	if ( record == (MX_RECORD *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );

		return NULL;
	}

	field = mx_get_record_field( record, "mx_type" );

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Record '%s' does not have a 'mx_type' field.  "
		"This should be impossible!", record->name );

		return NULL;
	}

	driver = (MX_DRIVER *) field->typeinfo;

	if ( driver == (MX_DRIVER *) NULL ) {
		mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Record '%s' does not appear to have a device driver.  "
		"This should be impossible for a running database!", 
			record->name );

		return NULL;
	}

	return driver;
}

MX_EXPORT const char *
mx_get_driver_name( MX_RECORD *record )
{
	MX_DRIVER *driver;

	driver = mx_get_driver_for_record( record );

	if ( driver == (MX_DRIVER *) NULL ) 
		return NULL;

	return driver->name;
}

/*=====================================================================*/

MX_EXPORT MX_DRIVER *
mx_get_superclass_driver_by_name( char *name )
{
	static const char fname[] = "mx_get_superclass_driver_by_name()";

	MX_DRIVER *result;
	char *list_name;
	int i;

	if ( name == NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"Record type name passed was NULL." );

		return NULL;
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( mx_superclass_list[i].mx_superclass == 0 ) {
			return (MX_DRIVER *) NULL;
		}

		list_name = mx_superclass_list[i].name;

		if ( list_name == NULL ) {
			return (MX_DRIVER *) NULL;
		}

		if ( strcmp( name, list_name ) == 0 ){
			result = &( mx_superclass_list[i] );

			MX_DEBUG( 8,
	( "%s: ptr = 0x%p, type = %ld", fname, result, result->mx_type));

			return result;
		}
	}
}

MX_EXPORT MX_DRIVER *
mx_get_class_driver_by_name( char *name )
{
	static const char fname[] = "mx_get_class_driver_by_name()";

	MX_DRIVER *result;
	char *list_name;
	int i;

	if ( name == NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"Record type name passed was NULL." );

		return NULL;
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( mx_class_list[i].mx_class == 0 ) {
			return (MX_DRIVER *) NULL;
		}

		list_name = mx_class_list[i].name;

		if ( list_name == NULL ) {
			return (MX_DRIVER *) NULL;
		}

		if ( strcmp( name, list_name ) == 0 ){
			result = &( mx_class_list[i] );

			MX_DEBUG( 8,
	( "%s: ptr = 0x%p, type = %ld", fname, result, result->mx_type));

			return result;
		}
	}
}

/*=====================================================================*/

static mx_status_type
mxp_setup_typeinfo_for_record_type_fields( MX_DRIVER *type_driver )
{
	static const char fname[] =
		"mxp_setup_typeinfo_for_record_type_fields()";

	MX_DRIVER *superclass_driver, *class_driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array;
	long num_record_fields;
	MX_RECORD_FIELD_DEFAULTS *superclass_field;
	MX_RECORD_FIELD_DEFAULTS *class_field;
	MX_RECORD_FIELD_DEFAULTS *type_field;
	long i;
	mx_status_type mx_status;

	if ( type_driver == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	record_field_defaults_array = *(type_driver->record_field_defaults_ptr);

	num_record_fields = *(type_driver->num_record_fields);

	/* Setup typeinfo for the superclass field. */

	for ( i = 0; ; i++ ) {
		if ( mx_superclass_list[i].mx_superclass
				== type_driver->mx_superclass )
		{
			superclass_driver = &mx_superclass_list[i];
			break;
		}
		if ( mx_superclass_list[i].mx_superclass == 0 ) {
			superclass_driver = NULL;
			break;
		}
	}

	if ( superclass_driver == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Superclass %ld is not a legal superclass.",
			type_driver->mx_superclass );
	}

	mx_status = mx_find_record_field_defaults( type_driver,
						"mx_superclass",
						&superclass_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	superclass_field->typeinfo = superclass_driver;

	/* Setup typeinfo for the class field. */

	for ( i = 0; ; i++ ) {
		if ( mx_class_list[i].mx_class == type_driver->mx_class ) {
			class_driver = &mx_class_list[i];
			break;
		}
		if ( mx_class_list[i].mx_class == 0 ) {
			class_driver = NULL;
			break;
		}
	}

	if ( class_driver == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Class %ld is not a legal class.",
			type_driver->mx_class );
	}

	mx_status = mx_find_record_field_defaults( type_driver,
						"mx_class", &class_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	class_field->typeinfo = class_driver;

	/* Setup typeinfo for the type field. */

	mx_status = mx_find_record_field_defaults( type_driver,
						"mx_type", &type_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_field->typeinfo = type_driver;

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

static mx_status_type
mxp_initialize_driver_entry( MX_DRIVER *driver )
{
	MX_RECORD_FIELD_DEFAULTS *defaults_array;
	MX_RECORD_FIELD_DEFAULTS *array_element;
	MX_RECORD_FUNCTION_LIST *flist;
	mx_status_type (*fptr) ( MX_DRIVER * );
	long *num_record_fields_ptr;
	long i, num_record_fields;
	mx_status_type mx_status;

	MX_DEBUG(-8,
	("superclass = %ld, class = %ld, type = %ld, name = '%s'",
		driver->mx_superclass, driver->mx_class,
		driver->mx_type, driver->name));

	/* Initialize the typeinfo fields if possible. */

	if ( (driver->num_record_fields != NULL)
	  && (driver->record_field_defaults_ptr != NULL) )
	{
		MX_DEBUG(-8,
		("num_record_fields = %ld, *record_field_defaults_ptr = %p",
		    *(driver->num_record_fields),
		    *(driver->record_field_defaults_ptr)));

		mx_status = mxp_setup_typeinfo_for_record_type_fields( driver );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Next, do type specific initialization. */

	flist = (MX_RECORD_FUNCTION_LIST *) driver->record_function_list;

	MX_DEBUG(-8,("  record_function_list pointer = %p",
		flist));

	if ( flist != NULL ) {
		fptr = flist->initialize_driver;

		MX_DEBUG(-8,("  initialize_driver pointer = %p",
			fptr));

		/* If all is well, invoke the function. */

		if ( fptr != NULL ) {
			mx_status = (*fptr)( driver );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Assign values to the field numbers. */

	num_record_fields_ptr = driver->num_record_fields;

	if ( num_record_fields_ptr != NULL ) {
		defaults_array
		    = *(driver->record_field_defaults_ptr);

		num_record_fields = *num_record_fields_ptr;

		for ( i = 0; i < num_record_fields; i++ ) {
			array_element = &defaults_array[i];

			array_element->field_number = i;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_add_driver_table( MX_DRIVER *driver_table )
{
	static const char fname[] = "mx_add_driver_table()";

	MX_DRIVER *current_driver, *table_entry;
	unsigned long i;
	mx_status_type mx_status;

	if ( driver_table == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The driver table pointer passed was NULL." );
	}

	/* Initialize the first driver in the table. */

	mx_status = mxp_initialize_driver_entry( &driver_table[0] );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_driver_list == (MX_DRIVER *) NULL ) {
		/* We are loading our first driver table,
		 * so initialize the driver list. */

		mx_driver_list = &driver_table[0];

		mx_driver_list->next_driver = NULL;
	}

	current_driver = mx_driver_list;

	/* Find the end of the existing list of loaded drivers. */

	while ( current_driver->next_driver != (MX_DRIVER *) NULL ) {
		current_driver = current_driver->next_driver;
	}
		
	/* Add in the rest of the driver table to the loaded driver list. */

	for ( i = 1; ; i++ ) {
		table_entry = &driver_table[i];

		table_entry->next_driver = NULL;

		if ( table_entry->mx_superclass == 0 ) {
			/* We have reached the end of the driver table. */

			break;
		}

		mx_status = mxp_initialize_driver_entry( table_entry );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		current_driver->next_driver = table_entry;

		current_driver = current_driver->next_driver;
	}

	/* We are done. */

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_initialize_drivers( void )
{
	MX_DRIVER *list_ptr;
	mx_status_type mx_status;

	list_ptr = mx_superclass_list;

	while ( list_ptr->mx_superclass != 0 ) {
		mx_status = mxp_initialize_driver_entry( list_ptr );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		list_ptr++;
	}

	list_ptr = mx_class_list;

	while ( list_ptr->mx_class != 0 ) {
		mx_status = mxp_initialize_driver_entry( list_ptr );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		list_ptr++;
	}

	mx_status = mx_add_driver_table( mx_type_list );

	return mx_status;
}

/*=====================================================================*/

MX_EXPORT mx_bool_type
mx_verify_driver_type( MX_RECORD *record, long mx_superclass,
				long mx_class, long mx_type )
{
	static const char fname[] = "mx_verify_driver_type()";

	int superclass_matches, class_matches, type_matches;
	int record_matches;

	if ( record == (MX_RECORD *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );

		return FALSE;
	}

	superclass_matches = FALSE;
	class_matches = FALSE;
	type_matches = FALSE;

	if ( ( mx_superclass == MXR_ANY )
	  || ( mx_superclass == record->mx_superclass ) )
	{
		superclass_matches = TRUE;
	}

	if ( ( mx_class == MXC_ANY )
	  || ( mx_class == record->mx_class ) )
	{
		class_matches = TRUE;
	}

	if ( ( mx_type == MXT_ANY )
	  || ( mx_type == record->mx_type ) )
	{
		type_matches = TRUE;
	}

	if ( superclass_matches && class_matches && type_matches ) {
		record_matches = TRUE;
	} else {
		record_matches = FALSE;
	}

#if 0
	MX_DEBUG(-2,("%s: '%s', (%ld, %ld, %ld) (%ld, %ld, %ld) (%d, %d, %d)",
		fname, record->name,
		mx_superclass, mx_class, mx_type,
		record->mx_superclass, record->mx_class, record->mx_type,
		superclass_matches, class_matches, type_matches));
#endif

	return record_matches;
}

/*=====================================================================*/

