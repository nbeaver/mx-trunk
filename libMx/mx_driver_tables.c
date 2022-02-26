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
 * Copyright 1999-2006, 2008-2010, 2012-2017, 2019, 2021-2022
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_DRIVER_TABLES_DEBUG	FALSE

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_stdint.h"

#include "mx_driver.h"

/* -- Define lists that relate types to classes and to function lists. -- */

  /****************** Record Superclasses ********************/

static MX_DRIVER mx_superclass_table[] = {
{"list_head_sclass", 0, 0, MXR_LIST_HEAD,     NULL, NULL, NULL, NULL, NULL},
{"interface",        0, 0, MXR_INTERFACE,     NULL, NULL, NULL, NULL, NULL},
{"device",           0, 0, MXR_DEVICE,        NULL, NULL, NULL, NULL, NULL},
{"scan",             0, 0, MXR_SCAN,          NULL, NULL, NULL, NULL, NULL},
{"variable",         0, 0, MXR_VARIABLE,      NULL, NULL, NULL, NULL, NULL},
{"server",           0, 0, MXR_SERVER,        NULL, NULL, NULL, NULL, NULL},
{"operation",        0, 0, MXR_OPERATION,     NULL, NULL, NULL, NULL, NULL},
{"special",          0, 0, MXR_SPECIAL,       NULL, NULL, NULL, NULL, NULL},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

  /********************* Record Classes **********************/

static MX_DRIVER mx_class_table[] = {

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
{"ethernet",       0, MXI_ETHERNET,       MXR_INTERFACE,
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
{"powerpmac",      0, MXV_POWERPMAC,      MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"field",          0, MXV_FIELD,          MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"special_variable", 0, MXV_SPECIAL,      MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"umx_variable",   0, MXV_UMX,            MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},
{"flowbus_variable", 0, MXV_FLOWBUS,      MXR_VARIABLE,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Server classes ================== */

{"network",        0, MXN_NETWORK_SERVER, MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},
{"spec",           0, MXN_SPEC,           MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},
{"bluice",         0, MXN_BLUICE,         MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},
{"umx",            0, MXN_UMX,            MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},
{"url",            0, MXN_URL,           MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Operation classes ================== */

{"operation_class", 0, MXO_OPERATION, MXR_OPERATION,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Special classes ================== */

{"program",        0, MXZ_PROGRAM, MXR_SPECIAL, NULL, NULL, NULL, NULL, NULL},
{"mod",            0, MXZ_MOD, MXR_SPECIAL, NULL, NULL, NULL, NULL, NULL},
{"dict",           0, MXZ_DICTIONARY, MXR_SPECIAL,
				NULL, NULL, NULL, NULL, NULL},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};


/* -- mx_type_table is now defined in another file. -- */

extern MX_DRIVER mx_type_table[];

/*-----*/

/* All of the loaded MX drivers are found via the static mx_driver_list
 * variable which is only visible to functions in this file.
 */

static MX_DRIVER *mx_driver_list = NULL;

/*-----*/

/* If, when loaded, an MX driver has an "mx_type" that is less than 0,
 * then this means that the driver writer has requested that MX allocate
 * an "mx_type" value dynamically.  mxp_next_dynamic_driver_type is used
 * to store the value of the next available "mx_type" number.
 *
 * By setting MX_DYNAMIC_DRIVER_BASE to 1,000,000,000 we make it possible
 * for there to be up to 1,147,483,647 dynamically allocated driver numbers
 * on a 32-bit computer.  On a 64-bit machine, the limit is, of course,
 * much larger.
 */

#define MX_DYNAMIC_DRIVER_BASE	1000000000L

static long mxp_next_dynamic_driver_type = MX_DYNAMIC_DRIVER_BASE;

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
mx_get_driver_object( MX_RECORD *record )
{
	static const char fname[] = "mx_get_driver_object()";

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
		"Record '%s' does not have a 'mx_type' field.", record->name );

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

	driver = mx_get_driver_object( record );

	if ( driver == (MX_DRIVER *) NULL ) 
		return NULL;

	return driver->name;
}

MX_EXPORT MX_DRIVER *
mx_get_driver_class_object( MX_RECORD *record )
{
	static const char fname[] = "mx_get_driver_class_objecgt()";

	MX_DRIVER *driver_class_object = NULL;
	long driver_class_number;
	size_t i, num_driver_classes;

	driver_class_number = record->mx_class;

	/* Walk through the class table looking for this driver. */

	num_driver_classes = sizeof(mx_class_table) / sizeof(mx_class_table[0]);

	for ( i = 0; i < num_driver_classes; i++ ) {
		driver_class_object = &(mx_class_table[i]);

		if ( driver_class_number == driver_class_object->mx_class ) {
			break;
		}
	}

	if ( i >= num_driver_classes ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
		"The MX driver class was not found for record '%s'.  "
		"This should never happen!", record->name );

		return NULL;
	}

	return driver_class_object;
}

MX_EXPORT const char *
mx_get_driver_class_name( MX_RECORD *record )
{
	MX_DRIVER *driver_class;

	driver_class = mx_get_driver_class_object( record );

	if ( driver_class == (MX_DRIVER *) NULL ) 
		return NULL;

	return driver_class->name;
}

MX_EXPORT MX_DRIVER *
mx_get_driver_superclass_object( MX_RECORD *record )
{
	static const char fname[] = "mx_get_driver_superclass_object()";

	MX_DRIVER *driver_superclass_object = NULL;
	long driver_superclass_number;
	size_t i, num_driver_superclasses;

	driver_superclass_number = record->mx_superclass;

	/* Walk through the superclass table looking for this driver. */

	num_driver_superclasses = sizeof(mx_superclass_table)
					/ sizeof(mx_superclass_table[0]);

	for ( i = 0; i < num_driver_superclasses; i++ ) {
		driver_superclass_object = &(mx_superclass_table[i]);

		if ( driver_superclass_number ==
			driver_superclass_object->mx_superclass )
		{
			break;
		}
	}

	if ( i >= num_driver_superclasses ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
		"The MX driver superclass was not found for record '%s'.  "
		"This should never happen!", record->name );

		return NULL;
	}

	return driver_superclass_object;
}

MX_EXPORT const char *
mx_get_driver_superclass_name( MX_RECORD *record )
{
	MX_DRIVER *driver_superclass;

	driver_superclass = mx_get_driver_superclass_object( record );

	if ( driver_superclass == (MX_DRIVER *) NULL ) 
		return NULL;

	return driver_superclass->name;
}

/*=====================================================================*/

MX_EXPORT MX_DRIVER *
mx_get_superclass_driver_by_name( char *name )
{
	MX_DRIVER *result;
	char *list_name;
	int i;

	if ( name == NULL ) {
		return mx_superclass_table;
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( mx_superclass_table[i].mx_superclass == 0 ) {
			return (MX_DRIVER *) NULL;
		}

		list_name = mx_superclass_table[i].name;

		if ( list_name == NULL ) {
			return (MX_DRIVER *) NULL;
		}

		if ( strcmp( name, list_name ) == 0 ){
			result = &( mx_superclass_table[i] );

			return result;
		}
	}
}

MX_EXPORT MX_DRIVER *
mx_get_class_driver_by_name( char *name )
{
	MX_DRIVER *result;
	char *list_name;
	int i;

	if ( name == NULL ) {
		return mx_class_table;
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( mx_class_table[i].mx_class == 0 ) {
			return (MX_DRIVER *) NULL;
		}

		list_name = mx_class_table[i].name;

		if ( list_name == NULL ) {
			return (MX_DRIVER *) NULL;
		}

		if ( strcmp( name, list_name ) == 0 ){
			result = &( mx_class_table[i] );

			return result;
		}
	}
}

/*=====================================================================*/

MX_EXPORT MX_DRIVER *
mx_get_superclass_driver_by_type( long requested_superclass_type )
{
#if 0
	static const char fname[] = "mx_get_superclass_driver_by_type()";
#endif

	MX_DRIVER *result;
	long list_superclass;
	int i;

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( mx_superclass_table[i].mx_superclass == 0 ) {
			return (MX_DRIVER *) NULL;
		}

		list_superclass = mx_superclass_table[i].mx_superclass;

		if ( list_superclass == requested_superclass_type ) {
			result = &( mx_superclass_table[i] );

#if 0
			MX_DEBUG(-8,
	( "%s: ptr = 0x%p, type = %ld", fname, result, result->mx_type));
#endif

			return result;
		}
	}
}

MX_EXPORT MX_DRIVER *
mx_get_class_driver_by_type( long requested_class_type )
{
#if 0
	static const char fname[] = "mx_get_class_driver_by_type()";
#endif

	MX_DRIVER *result;
	long list_class;
	int i;

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( mx_class_table[i].mx_class == 0 ) {
			return (MX_DRIVER *) NULL;
		}

		list_class = mx_class_table[i].mx_class;

		if ( list_class == requested_class_type ) {
			result = &( mx_class_table[i] );

#if 0
			MX_DEBUG(-8,
	( "%s: ptr = 0x%p, type = %ld", fname, result, result->mx_type));
#endif

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
	MX_RECORD_FIELD_DEFAULTS *superclass_field;
	MX_RECORD_FIELD_DEFAULTS *class_field;
	MX_RECORD_FIELD_DEFAULTS *type_field;
	long i;
	mx_status_type mx_status;

	if ( type_driver == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	/* Setup typeinfo for the superclass field. */

	for ( i = 0; ; i++ ) {
		if ( mx_superclass_table[i].mx_superclass
				== type_driver->mx_superclass )
		{
			superclass_driver = &mx_superclass_table[i];
			break;
		}
		if ( mx_superclass_table[i].mx_superclass == 0 ) {
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
		if ( mx_class_table[i].mx_class == type_driver->mx_class ) {
			class_driver = &mx_class_table[i];
			break;
		}
		if ( mx_class_table[i].mx_class == 0 ) {
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
	static const char fname[] = "mxp_initialize_driver_entry()";

	MX_RECORD_FIELD_DEFAULTS *defaults_array;
	MX_RECORD_FIELD_DEFAULTS *array_element;
	MX_RECORD_FUNCTION_LIST *flist;
	mx_status_type (*fptr) ( MX_DRIVER * );
	long *num_record_fields_ptr;
	long i, num_record_fields;
	mx_status_type mx_status;

#if MX_DRIVER_TABLES_DEBUG
	MX_DEBUG(-8,
	("superclass = %ld, class = %ld, type = %ld, name = '%s'",
		driver->mx_superclass, driver->mx_class,
		driver->mx_type, driver->name));
#endif

	/* Initialize the typeinfo fields if possible. */

	if ( (driver->num_record_fields != NULL)
	  && (driver->record_field_defaults_ptr != NULL) )
	{

#if MX_DRIVER_TABLES_DEBUG
		MX_DEBUG(-8,
		("num_record_fields = %ld, *record_field_defaults_ptr = %p",
		    *(driver->num_record_fields),
		    *(driver->record_field_defaults_ptr)));
#endif

		mx_status = mxp_setup_typeinfo_for_record_type_fields( driver );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Next, do type specific initialization. */

	flist = (MX_RECORD_FUNCTION_LIST *) driver->record_function_list;

#if MX_DRIVER_TABLES_DEBUG
	MX_DEBUG(-8,("  record_function_list pointer = %p",
		flist));
#endif

	if ( flist != NULL ) {
		fptr = flist->initialize_driver;

#if MX_DRIVER_TABLES_DEBUG
		MX_DEBUG(-8,("  initialize_driver pointer = %p",
			fptr));
#endif

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

	/* If "mx_type" is greater than MX_DYNAMIC_DRIVER_BASE, then
	 * the driver writer has picked a static driver type value from
	 * the dynamic range.  This is illegal, so we must generate
	 * an error for this.
	 */

	if ( driver->mx_type >= MX_DYNAMIC_DRIVER_BASE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Static MX driver number %ld for driver '%s' "
		"is in the dynamic driver number "
		"range of %ld and above.  Statically allocated MX driver "
		"numbers must be a unique value in the range from 0 to %ld.",
			driver->mx_type, driver->name,
			MX_DYNAMIC_DRIVER_BASE,
			MX_DYNAMIC_DRIVER_BASE - 1L );
	}

	/* If "mx_type" is less than zero, then we must dynamically
	 * allocate an "mx_type" for the driver.
	 */

	if ( driver->mx_type < 0 ) {
		if ( mxp_next_dynamic_driver_type >= LONG_MAX ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"We have run out of dynamically allocated "
			"MX driver type numbers!  The maximum possible "
			"number of dynamically loaded drivers is %ld, so "
			"this is unlikely to happen in normal operation.",
				LONG_MAX - MX_DYNAMIC_DRIVER_BASE - 1L );
		}

		driver->mx_type = mxp_next_dynamic_driver_type;

		mxp_next_dynamic_driver_type++;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_add_driver_table( MX_DRIVER *driver_table )
{
	static const char fname[] = "mx_add_driver_table()";

	MX_DRIVER *current_driver, *table_entry;
	unsigned long i, offset;
	mx_status_type mx_status;

	if ( driver_table == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The driver table pointer passed was NULL." );
	}

	if ( mx_driver_list != (MX_DRIVER *) NULL ) {
		offset = 0;
	} else {
		/* We are loading our first driver table,
		 * so initialize the driver list. */

		mx_status = mxp_initialize_driver_entry( &driver_table[0] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_driver_list = &driver_table[0];

		mx_driver_list->next_driver = NULL;

		offset = 1;
	}

	current_driver = mx_driver_list;

	/* Find the end of the existing list of loaded drivers. */

	while ( current_driver->next_driver != (MX_DRIVER *) NULL ) {
		current_driver = current_driver->next_driver;
	}
		
	/* Add in the rest of the driver table to the loaded driver list. */

	for ( i = offset; ; i++ ) {
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
	static const char fname[] = "mx_initialize_drivers()";

	MX_DRIVER *current_superclass_driver, *next_superclass_driver;
	MX_DRIVER *current_class_driver, *next_class_driver;
	mx_status_type mx_status;

	/* Note: Both mx_superclass_table and mx_class_table are arrays
	 * of MX_DRIVER structures.
	 */

	/*---------------------------------------------------------------*/

	current_superclass_driver = mx_superclass_table;

	if ( current_superclass_driver->mx_superclass == 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mx_superclass_table array contains no drivers." );
	}

	while ( 1 ) {
		mx_status = mxp_initialize_driver_entry(
					current_superclass_driver );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		next_superclass_driver = current_superclass_driver + 1;

		if ( next_superclass_driver->mx_superclass == 0 ) {
			current_superclass_driver->next_driver = NULL;

			break;	/* Exit the while() loop. */
		} else {
			current_superclass_driver->next_driver
				= next_superclass_driver;
		}

		current_superclass_driver = next_superclass_driver;
	}

	/*---------------------------------------------------------------*/

	current_class_driver = mx_class_table;

	while ( 1 ) {
		mx_status = mxp_initialize_driver_entry( current_class_driver );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		next_class_driver = current_class_driver + 1;

		if ( next_class_driver->mx_class == 0 ) {
			current_class_driver->next_driver = NULL;

			break;	/* Exit the while() loop. */
		} else {
			current_class_driver->next_driver
				= next_class_driver;
		}

		current_class_driver = next_class_driver;
	}

	/*---------------------------------------------------------------*/

	mx_status = mx_add_driver_table( mx_type_table );

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

/* These variables are used below by the test for verifying that the 
 * minimum number of record fields are present.
 */

MX_RECORD_FIELD_DEFAULTS mxp_field_test_table_xyzzy[] =
				{MX_RECORD_STANDARD_FIELDS};

static unsigned long mxp_minimum_number_of_fields = 
    ( sizeof(mxp_field_test_table_xyzzy) / sizeof(MX_RECORD_FIELD_DEFAULTS) );

/* mx_verify_driver() checks to see if there is anything wrong with
 * the specified driver.
 */

MX_EXPORT mx_status_type
mx_verify_driver( MX_DRIVER *driver )
{
	static const char fname[] = "mx_verify_driver()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS *their_record_field_defaults;
	long i, j, num_record_fields;
	short our_structure_id, their_structure_id;
	size_t our_structure_offset, their_structure_offset;
	mx_bool_type driver_failed;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	mx_info( "Verifying driver '%s'", driver->name );

	driver_failed = FALSE;

	/*----------------------------------------------------------------*/

	/* Every driver must have a create_record_structures method defined,
	 * except for the list head driver.
	 */

	if ( driver->mx_superclass != MXR_LIST_HEAD ) {

		if ( driver->record_function_list->create_record_structures
			== NULL )
		{
			driver_failed = TRUE;

			(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Driver '%s' does not have a create_record_structures method.",
				driver->name );
		}
	}

	/*----------------------------------------------------------------*/

	/* Every driver must have at least as many record fields as
	 * are defined in MX_RECORD_STANDARD_FIELDS.
	 */

	num_record_fields = *(driver->num_record_fields);

	if ( num_record_fields < mxp_minimum_number_of_fields ) {
		driver_failed = TRUE;

		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The number of record fields (%ld) defined by the "
		"driver '%s' is less than the minimum of %lu "
		"as defined by MX_RECORD_STANDARD_FIELDS.",
			num_record_fields,
			driver->name,
			mxp_minimum_number_of_fields );
	}

	/*----------------------------------------------------------------*/

	/* Now we need to loop through the entries in the array of
	 * MX_RECORD_FIELD_DEFAULTS structures.
	 */

	record_field_defaults_array = *(driver->record_field_defaults_ptr);

	if ( record_field_defaults_array == NULL ) {
	    driver_failed = TRUE;

	    (void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	  "The MX_RECORD_FIELD_DEFAULTS array pointer for driver '%s' is NULL.",
			driver->name );
	} else {
	    for ( i = 0; i < num_record_fields; i++ ) {

		record_field_defaults = &(record_field_defaults_array[i]);

		if ( record_field_defaults == NULL ) {
		    driver_failed = TRUE;

		    (void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD_FIELD_DEFAULTS pointer for field %ld "
			"of driver '%s' is NULL.", i, driver->name );
		} else {
#if 0
		    MX_DEBUG(-2,("%s: field '%s'",
			fname, record_field_defaults->name));
#endif
		    /* Verify that no other fields share this field's
		     * combination of structure_id and structure_offset.
		     * If one does, then the wrong data structure may
		     * be assigned to the wrong field.
		     */

		    /* FIXME: We need to add an MXFF_ALIAS field for fields
		     * that are intentional duplicates of each other, such
		     * as the motor 'stop' and 'soft_abort' fields.
		     * Otherwise, we will get lots of false positives.
		     */

		    our_structure_id = record_field_defaults->structure_id;
		    our_structure_offset =
				record_field_defaults->structure_offset;

		    for ( j = i+1; j < num_record_fields; j++ ) {
			their_record_field_defaults =
				&(record_field_defaults_array[j]);

			their_structure_id =
				their_record_field_defaults->structure_id;

			their_structure_offset =
				their_record_field_defaults->structure_offset;

			if ( (our_structure_id == their_structure_id)
			  && (our_structure_offset == their_structure_offset) )
			{
			    driver_failed = TRUE;

			    (void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			    "Driver '%s' has the same structure_id and "
			    "structure_offset for both fields '%s' and '%s'.",
				driver->name,
				record_field_defaults->name,
				their_record_field_defaults->name );
			}
		    }
		}
	    }
	}

	/*----------------------------------------------------------------*/

	if ( driver_failed == TRUE ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Driver '%s' has failed its verification tests.",
			driver->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

/*=====================================================================*/

MX_EXPORT mx_status_type
mx_verify_driver_tables( void )
{
	static const char fname[] = "mx_verify_driver_tables()";

	MX_DRIVER *current_driver;
	mx_bool_type at_least_one_driver_failed;
	mx_status_type mx_status;

	at_least_one_driver_failed = FALSE;

	current_driver = mx_driver_list;

#if 0
	/* The list head's driver comes first (and is special),
	 * so we skip over it.
	 */

	current_driver = current_driver->next_driver;
#endif

	/* Now walk through the drivers. */

	while ( current_driver != (MX_DRIVER *) NULL ) {

		mx_status =  mx_verify_driver( current_driver );

		if ( mx_status.code != MXE_SUCCESS ) {
			at_least_one_driver_failed = TRUE;
		}

		current_driver = current_driver->next_driver;
	}

	if ( at_least_one_driver_failed ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"At least one MX driver failed its verification checks." );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}


/*=====================================================================*/

