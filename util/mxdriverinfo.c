/*
 * Name:    mxdriverinfo.c
 *
 * Purpose: MX utility for displaying information about available MX drivers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2003-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MX_DEFINE_DRIVER_LISTS

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_version.h"

#ifndef SUCCESS
#define SUCCESS     1
#define FAILURE     0
#endif

#define MXDI_SUPERCLASSES	1
#define MXDI_CLASSES		2
#define MXDI_TYPES		3
#define MXDI_FIELDS		4
#define MXDI_DRIVERS		5
#define MXDI_VERSION		6
#define MXDI_LATEX_FIELDS	7

static int find_driver( MX_DRIVER *driver_list,
			char *name,
			MX_DRIVER **driver_found );

static char * find_varargs_field_name( MX_DRIVER *driver,
					long varargs_value,
					int debug );

static int show_all_drivers( MX_DRIVER **list_of_types, int debug );

static int show_drivers( MX_DRIVER **list_of_types,
				int items_to_show,
				char *item_name,
				int debug );

static int show_field_list( MX_DRIVER **list_of_types,
				char *item_name,
				int show_all_fields,
				int show_handles,
				int debug );

static int show_field( MX_DRIVER *driver,
			MX_RECORD_FIELD_DEFAULTS *field_defaults,
			int field_number,
			int show_handles,
			int debug );

static int show_latex_field_table( MX_DRIVER **list_of_types,
					char *item_name,
					int show_all_fields,
					int debug );

static int show_latex_field( MX_DRIVER *driver,
				MX_RECORD_FIELD_DEFAULTS *field_defaults,
				int debug );


static char program_name[] = "mxdriverinfo";

int
main( int argc, char *argv[] ) {

	MX_DRIVER **list_of_types;
	int c, items_to_show, debug, show_all_fields, show_handles, status;
	char item_name[ MXU_DRIVER_NAME_LENGTH + 1 ];

	static char usage_format[] =
"Usage: %s [options]\n"
"\n"
"  where options are\n"
"    -f'driver_name'     Display record fields in record description\n"
"    -a'driver_name'     Display all record fields\n"
"\n"
"    -s                  Display available MX superclasses\n"
"    -c'superclass_name' Display available MX classes\n"
"    -t'class_name'      Display available MX types\n"
"\n"
"    -l                  Display all MX types\n"
"\n"
"    -v                  Display the version of MX in use\n"
"\n"
"    -h                  Display array handles\n"
"    -d                  Turn on debugging\n"
"\n";

	/* Process the command line arguments, if any. */

	items_to_show = 0;
	debug = FALSE;
	show_all_fields = FALSE;
	show_handles = FALSE;
	strcpy( item_name, "" );

	while ((c = getopt(argc, argv, "a:c:df:hlst:vA:F:")) != -1 ) {
		switch (c) {
		case 'a':
			items_to_show = MXDI_FIELDS;
			show_all_fields = TRUE;

			strlcpy( item_name, optarg, MXU_DRIVER_NAME_LENGTH );
			break;
		case 'c':
			items_to_show = MXDI_CLASSES;

			strlcpy( item_name, optarg, MXU_DRIVER_NAME_LENGTH );
			break;
		case 'd':
			debug = TRUE;
			break;
		case 'f':
			items_to_show = MXDI_FIELDS;
			show_all_fields = FALSE;

			strlcpy( item_name, optarg, MXU_DRIVER_NAME_LENGTH );
			break;
		case 'h':
			show_handles = TRUE;
			break;
		case 'l':
			items_to_show = MXDI_DRIVERS;
			break;
		case 's':
			items_to_show = MXDI_SUPERCLASSES;
			break;
		case 't':
			items_to_show = MXDI_TYPES;

			strlcpy( item_name, optarg, MXU_DRIVER_NAME_LENGTH );
			break;
		case 'v':
			items_to_show = MXDI_VERSION;
			break;
		case 'A':
			items_to_show = MXDI_LATEX_FIELDS;
			show_all_fields = TRUE;

			strlcpy( item_name, optarg, MXU_DRIVER_NAME_LENGTH );
			break;
		case 'F':
			items_to_show = MXDI_LATEX_FIELDS;
			show_all_fields = FALSE;

			strlcpy( item_name, optarg, MXU_DRIVER_NAME_LENGTH );
			break;
		case '?':
			fprintf(stderr, usage_format, program_name);
			exit(1);
		}
	}

	if ( debug ) {
		fprintf(stderr,"items_to_show = %d\n", items_to_show);
		fprintf(stderr,"item_name     = '%s'\n", item_name);
	}

	list_of_types = mx_get_driver_lists();

	switch (items_to_show) {
	case MXDI_DRIVERS:
		status = show_all_drivers( list_of_types, debug );
		break;
	case MXDI_SUPERCLASSES:
		status = show_drivers( list_of_types,
					items_to_show, NULL, debug );
		break;
	case MXDI_CLASSES:
	case MXDI_TYPES:
		status = show_drivers( list_of_types,
					items_to_show, item_name, debug );
		break;
	case MXDI_FIELDS:
		status = show_field_list( list_of_types, item_name,
					show_all_fields, show_handles, debug );
		break;
	case MXDI_VERSION:
		fprintf(stderr, "\nMX version %s\n\n", mx_get_version_string());

		status = SUCCESS;
		break;
	case MXDI_LATEX_FIELDS:
		status = show_latex_field_table( list_of_types, item_name,
						show_all_fields, debug );
		break;
	default:
		fprintf(stderr, usage_format, program_name);
		exit(1);
		break;
	}

	if ( status != SUCCESS )
		exit(1);

	exit(0);

#if defined(OS_IRIX) || defined(OS_HPUX) \
		|| defined(OS_VXWORKS) || defined(__BORLANDC__)
	return 0;   /* These think a non-void main() should return a value. */
#endif
}

static int
find_driver( MX_DRIVER *driver_list, char *name, MX_DRIVER **driver_found )
{
	unsigned long i;

	*driver_found = NULL;

	for ( i = 0; ; i++ ) {
		if ( driver_list[i].mx_superclass == 0 ) {
			break;		/* End of the list. */
		}

		if ( strcmp( name, driver_list[i].name ) == 0 ) {
			*driver_found = &driver_list[i];

			return SUCCESS;
		}
	}
	return FAILURE;
}

static char *
find_varargs_field_name( MX_DRIVER *driver,
			long varargs_cookie,
			int debug )
{
	static char defaults_string[MXU_FIELD_NAME_LENGTH + 1];

	MX_RECORD_FIELD_DEFAULTS *field_defaults_array;
	MX_RECORD_FIELD_DEFAULTS *referenced_field_defaults;
	unsigned long num_record_fields;
	long field_index, array_in_field_index;

	varargs_cookie = -varargs_cookie;

	field_index = varargs_cookie / MXU_VARARGS_COOKIE_MULTIPLIER;

	array_in_field_index = varargs_cookie % MXU_VARARGS_COOKIE_MULTIPLIER;

	num_record_fields = *(driver->num_record_fields);

	field_defaults_array = *(driver->record_field_defaults_ptr);

	if ( ( field_index < 0 ) || ( field_index >= num_record_fields ) ) {
		fprintf( stderr, "ERROR: Illegal referenced field index %ld\n",
					field_index );
		exit(1);
	}

	referenced_field_defaults = &field_defaults_array[ field_index ];

	sprintf( defaults_string, "%s,%ld",
				referenced_field_defaults->name,
				array_in_field_index );

	return &defaults_string[0];
}

static int
show_all_drivers( MX_DRIVER **list_of_types, int debug )
{
	static const char fname[] = "show_all_drivers()";

	MX_DRIVER *superclass_list, *class_list, *type_list;
	MX_DRIVER *driver;
	unsigned long i, j;
	unsigned long old_superclass, old_class;
	char *superclass_name, *class_name;

	superclass_list = list_of_types[0];
	class_list      = list_of_types[1];
	type_list       = list_of_types[2];

	old_superclass = 0;
	old_class      = 0;

	superclass_name = NULL;
	class_name      = NULL;

	for ( i = 0; ; i++ ) {

		driver = &type_list[i];

		/* Is this the end of the list? */

		if ( driver->mx_superclass == 0 ) {
			break;			/* Exit the for() loop. */
		}

		/* Find the superclass name if necessary. */

		if ( old_superclass != driver->mx_superclass ) {

			for ( j = 0; ; j++ ) {

				if ( superclass_list[j].mx_superclass == 0 ) {
					fprintf( stderr,
			"%s: Internal error, driver '%s' specified a "
			"superclass %ld that does not exist!\n",
						fname, driver->name,
						driver->mx_superclass );

					return FAILURE;
				}

				if ( driver->mx_superclass
					== superclass_list[j].mx_superclass )
				{
					old_superclass = driver->mx_superclass;

					superclass_name
						= superclass_list[j].name;

					/* Force the class name to be
					 * looked up again.
					 */

					old_class = 0;

					break;	/* Exit the for() loop. */
				}
			}
		}

		/* Find the class name if necessary. */

		if ( old_class != driver->mx_class ) {

			for ( j = 0; ; j++ ) {

				if ( class_list[j].mx_class == 0 ) {
					fprintf( stderr,
			"%s: Internal error, driver '%s' specified a "
			"class %ld that does not exist!\n",
						fname, driver->name,
						driver->mx_class );

					return FAILURE;
				}

				if ( driver->mx_class
					== class_list[j].mx_class )
				{
					old_class = driver->mx_class;

					class_name = class_list[j].name;

					break;	/* Exit the for() loop. */
				}
			}
		}

		printf( "%s %s %s\n",
			superclass_name, class_name, driver->name );
	}

	return SUCCESS;
}

static int
show_drivers( MX_DRIVER **list_of_types,
		int items_to_show,
		char *item_name,
		int debug )
{
	static const char fname[] = "show_drivers()";

	MX_DRIVER *item_list;
	MX_DRIVER *driver;
	unsigned long i, mx_superclass, mx_class, mx_type;
	int status;

	mx_superclass = mx_class = mx_type = 0;

	switch (items_to_show) {
	case MXDI_SUPERCLASSES:
		item_list = list_of_types[0];

		if ( debug ) {
			fprintf(stderr, "%s invoked for all superclasses\n",
					fname);
		}
		break;
	case MXDI_CLASSES:
		item_list = list_of_types[1];

		if ( debug ) {
			fprintf(stderr, "%s invoked for superclass '%s'\n",
					fname, item_name);
		}
		status = find_driver( list_of_types[0], item_name, &driver );

		if ( status != SUCCESS )
			return status;

		mx_superclass = driver->mx_superclass;
		break;
	case MXDI_TYPES:
		item_list = list_of_types[2];

		if ( debug ) {
			fprintf(stderr, "%s invoked for class '%s'\n",
					fname, item_name);
		}
		status = find_driver( list_of_types[1], item_name, &driver );

		if ( status != SUCCESS )
			return status;

		mx_superclass = driver->mx_superclass;
		mx_class = driver->mx_class;
		break;
	default:
		fprintf(stderr,"Error: unexpected items_to_show = %d\n",
			items_to_show);
		exit(1);
		break;
	}

	if ( debug ) {
		fprintf(stderr, "mx_superclass = %lu\n", mx_superclass);
		fprintf(stderr, "mx_class      = %lu\n", mx_class);
		fprintf(stderr, "mx_type       = %lu\n", mx_type);
	}

	for ( i = 0; ; i++ ) {
		if ( item_list[i].mx_superclass == 0 ) {
			break;		/* End of the list. */
		}

		if ( debug ) {
			fprintf(stderr,"item_list[%lu] = '%s'\n",
				i, item_list[i].name);
		}

		if ( ( mx_superclass == 0 )
		  || ( mx_superclass == item_list[i].mx_superclass ) )
		{
			if ( ( mx_class == 0 )
			  || ( mx_class == item_list[i].mx_class ) )
			{
				if ( ( mx_type == 0 )
				  || ( mx_type == item_list[i].mx_type ) )
				{
					printf("%s\n", item_list[i].name);
				}
			}
		}
	}

	return SUCCESS;
}

static int
show_field_list( MX_DRIVER **list_of_types,
		char *driver_name,
		int show_all_fields,
		int show_handles,
		int debug )
{
	static const char fname[] = "show_field_list()";

	MX_DRIVER *driver_list;
	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *field_defaults_array, *field_defaults;
	unsigned long i;
	int status;

	if ( debug ) {
		fprintf(stderr, "%s invoked for driver '%s'\n",
			fname, driver_name);
	}

	/* Initialize the MX device drivers. */

	mx_initialize_drivers();

	/* Walk through the list of drivers looking for the requested driver. */

	driver_list = list_of_types[2];

	for ( i = 0; ; i++ ) {
		if ( driver_list[i].mx_superclass == 0 ) {
			/* End of the list. */

			fprintf(stderr, "The MX driver '%s' was not found.\n",
					driver_name );
			exit(1);
		}

		if ( strcmp( driver_name, driver_list[i].name ) == 0 ) {
			driver = &driver_list[i];

			break;	/* Exit the for() loop. */
		}
	}

	/* The pointers to the number of record fields and the record
	 * field defaults array must both be non-NULL for us to proceed.
	 */

	if ( ( driver->num_record_fields == NULL ) 
	  || ( driver->record_field_defaults_ptr == NULL ) )
	{
		/* This driver has no fields available and is not currently
		 * useable, so we do not try to do anything further with it.
		 */

		return SUCCESS;
	}

	/* Show the field list. */

	field_defaults_array = *(driver->record_field_defaults_ptr);

	for ( i = 0; i < *(driver->num_record_fields); i++ ) {

		field_defaults = &field_defaults_array[i];

		if ( ( show_all_fields == TRUE )
		  || ( field_defaults->flags & MXFF_IN_DESCRIPTION ) )
		{
			status = show_field( driver, field_defaults,
						(int) i, show_handles, debug );

		}
	}

	return SUCCESS;
}

static int
show_field( MX_DRIVER *driver,
		MX_RECORD_FIELD_DEFAULTS *field_defaults,
		int field_number,
		int show_handles,
		int debug )
{
	unsigned long i;
	long dimension, num_dimensions;
	long field_is_varargs;

	field_is_varargs = ( field_defaults->flags & MXFF_VARARGS );

	if ( show_handles ) {
		printf( "(%d) ", field_number );
	}

	printf( "%s ", field_defaults->name );

	printf( "%s ", mx_get_field_type_string( field_defaults->datatype ) );

	/* Display the number of dimensions. */

	if ( field_defaults->num_dimensions < 0 ) {
		num_dimensions = MXU_FIELD_MAX_DIMENSIONS;

		printf( "V:%s ", find_varargs_field_name( driver,
					field_defaults->num_dimensions,
					debug ) );
	} else {
		num_dimensions = field_defaults->num_dimensions;

		printf( "F:%ld ", num_dimensions );
	}

	/* Display each of the dimensions in turn. */

	for ( i = 0; i < num_dimensions; i++ ) {

		dimension = field_defaults->dimension[i];

		if ( dimension < 0 ) {
			printf( "V:%s ", find_varargs_field_name( driver,
						dimension,
						debug ) );
		} else {
			printf( "F:%ld ", dimension );
		}
	}


	printf( "\n" );

	return SUCCESS;
}

/*--------------------------------------------------------------------------*/

/* The following functions are intended to help in constructing field tables
 * for the MX Driver Reference Manual.  They are probably not useful for
 * other purposes.
 */

static int
show_latex_field_table( MX_DRIVER **list_of_types,
		char *driver_name,
		int show_all_fields,
		int debug )
{
	static const char fname[] = "show_latex_field_table()";

	MX_DRIVER *driver_list;
	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *field_defaults_array, *field_defaults;
	unsigned long i;
	int status;

	if ( debug ) {
		fprintf(stderr, "%s invoked for driver '%s'\n",
			fname, driver_name);
	}

	/* Initialize the MX device drivers. */

	mx_initialize_drivers();

	/* Walk through the list of drivers looking for the requested driver. */

	driver_list = list_of_types[2];

	for ( i = 0; ; i++ ) {
		if ( driver_list[i].mx_superclass == 0 ) {
			/* End of the list. */

			fprintf(stderr, "The MX driver '%s' was not found.\n",
					driver_name );
			exit(1);
		}

		if ( strcmp( driver_name, driver_list[i].name ) == 0 ) {
			driver = &driver_list[i];

			break;	/* Exit the for() loop. */
		}
	}

	/* The pointers to the number of record fields and the record
	 * field defaults array must both be non-NULL for us to proceed.
	 */

	if ( ( driver->num_record_fields == NULL ) 
	  || ( driver->record_field_defaults_ptr == NULL ) )
	{
		/* This driver has no fields available and is not currently
		 * useable, so we do not try to do anything further with it.
		 */

		return SUCCESS;
	}

	/* Print out a header for the LaTeX table. */

	printf( "\\begin{tabular}{|c|c|c|c|p{72mm}|}\n" );
	printf( "  \\hline\n" );
	printf(
	"  \\MxTextFieldName & \\MxTextFieldType & \\MxTextNumDimensions\n" );
	printf(
	"          & \\MxTextSizes & \\MxTextDescription \\\\\n" );
	printf( "  \\hline\n" );
	printf( "  %%\\multicolumn{5}{|c|}{\\linkmotorfields} \\\\\n" );
	printf( "  %%\\hline\n" );

	/* Show the field list. */

	field_defaults_array = *(driver->record_field_defaults_ptr);

	for ( i = 0; i < *(driver->num_record_fields); i++ ) {

		field_defaults = &field_defaults_array[i];

		if ( ( show_all_fields == TRUE )
		  || ( field_defaults->flags & MXFF_IN_DESCRIPTION ) )
		{
			status = show_latex_field( driver,
					field_defaults, debug );

		}
	}

	printf( "\\end{tabular}\n" );

	return SUCCESS;
}

static int
show_latex_field( MX_DRIVER *driver,
		MX_RECORD_FIELD_DEFAULTS *field_defaults,
		int debug )
{
	char buffer[500];
	char *name_ptr;
	int c;
	const char *type_ptr;
	unsigned long i, j, length;
	long dimension, num_dimensions;
	long field_is_varargs;

	field_is_varargs = ( field_defaults->flags & MXFF_VARARGS );

	/* Print out the field name with LaTeX escapes for underscores. */

	name_ptr = field_defaults->name;

	length = strlen(name_ptr);

	for ( i = 0, j = 0; i < length; i++, j++ ) {
		if ( name_ptr[i] == '_' ) {
			buffer[j] = '\\';
			j++;
		}
		buffer[j] = name_ptr[i];
	}
	buffer[j] = '\0';

	printf( "  \\textit{%s} & ", buffer );

	/* Print out the field datatype in lower case with the MXFT_ prefix
	 * removed and with LaTeX escapes for underscores.
	 */

	type_ptr = mx_get_field_type_string( field_defaults->datatype );
	type_ptr += 5;

	length = strlen(type_ptr);

	for ( i = 0, j = 0; i < length; i++, j++ ) {
		if ( type_ptr[i] == '_' ) {
			buffer[j] = '\\';
			j++;
		}
		c = type_ptr[i];

		buffer[j] = tolower(c);
	}
	buffer[j] = '\0';

	printf( "  \\textit{%s} & ", buffer );

	/* Display the number of dimensions. */

	if ( field_defaults->num_dimensions < 0 ) {
		num_dimensions = MXU_FIELD_MAX_DIMENSIONS;

		printf( "  \\textit{%s} & ", find_varargs_field_name( driver,
					field_defaults->num_dimensions,
					debug ) );
	} else {
		num_dimensions = field_defaults->num_dimensions;

		printf( "%ld & ", num_dimensions );
	}

	/* Display each of the dimensions in turn. */

	if ( num_dimensions == 0 ) {
		printf( "0" );
	} else {
		for ( i = 0; i < num_dimensions; i++ ) {

			if ( i > 0 ) {
				printf( ", " );
			}

			dimension = field_defaults->dimension[i];

			if ( dimension < 0 ) {
				printf( "\textit{%s}",
					find_varargs_field_name( driver,
					dimension, debug ) );
			} else {
				printf( "%ld", dimension );
			}
		}
	}

	printf( " & xxx \\\\\n" );
	printf( "  \\hline\n" );

	return SUCCESS;
}

/*--------------------------------------------------------------------------*/

