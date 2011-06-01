/*
 * Name:    mxdriverinfo.c
 *
 * Purpose: MX utility for displaying information about available MX drivers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2003-2010 Illinois Institute of Technology
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

static char * find_varargs_field_name( MX_DRIVER *driver,
					long varargs_value,
					mx_bool_type debug );

static int show_all_drivers( mx_bool_type debug );

static int show_drivers( int items_to_show,
			char *item_name,
			mx_bool_type debug );

static int show_field_list( char *item_name,
			mx_bool_type show_all_fields,
			mx_bool_type show_handles,
			mx_bool_type debug );

static int show_field( MX_DRIVER *driver,
			MX_RECORD_FIELD_DEFAULTS *field_defaults,
			int field_number,
			mx_bool_type show_handles,
			mx_bool_type debug );

static int show_latex_field_table( char *item_name,
				unsigned long structures_to_show,
				mx_bool_type show_all_fields,
				mx_bool_type show_link,
				mx_bool_type debug );

static int show_latex_field( MX_DRIVER *driver,
				MX_RECORD_FIELD_DEFAULTS *field_defaults,
				mx_bool_type debug );

static int create_latex_command( char *buffer,
				size_t max_buffer_length,
				char *format,
				... );

static char *capitalize_string( char *original_string );

static char program_name[] = "mxdriverinfo";

int
main( int argc, char *argv[] ) {

	int c, items_to_show, debug, status;
	char item_name[ MXU_DRIVER_NAME_LENGTH + 1 ];
	mx_bool_type show_all_fields, show_handles, show_link, start_debugger;
	unsigned long i, length, structures_to_show;

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
"    -d                  Turn on debugging output\n"
"\n"
"    -D                  Start debugger\n"
"\n"
"  LaTeX documentation generation options:\n"
"\n"
"    -F'driver_name'     Display record fields in description for LaTeX\n"
"    -A'driver_name'     Display all record fields for LaTeX\n"
"\n"
"    -L                  Show the field links for LaTeX\n"
"\n"
"    -S[r][s][c][t]      Specify structures to show in LaTeX\n"
"                          r = record structures\n"
"                          s = superclass structures\n"
"                          c = class structures\n"
"                          t = type structures\n"
"\n";

	/* Process the command line arguments, if any. */

	items_to_show = 0;
	debug = FALSE;
	structures_to_show = 0xffffffff;
	show_all_fields = FALSE;
	show_handles = FALSE;
	show_link = FALSE;
	start_debugger = FALSE;
	strcpy( item_name, "" );

	while ((c = getopt(argc, argv, "a:c:dDf:hlst:vA:F:LS:")) != -1 ) {
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
		case 'D':
			start_debugger = TRUE;
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
		case 'L':
			show_link = TRUE;
			break;
		case 'S':
			length = strlen(optarg);
			structures_to_show = 0;

			for ( i = 0; i < length; i++ ) {
			    c = optarg[i];

			    switch(c) {
			    case 'r':
				structures_to_show |= MXF_REC_RECORD_STRUCT;
				break;
			    case 's':
				structures_to_show |= MXF_REC_SUPERCLASS_STRUCT;
				break;
			    case 'c':
				structures_to_show |= MXF_REC_CLASS_STRUCT;
				break;
			    case 't':
				structures_to_show |= MXF_REC_TYPE_STRUCT;
				break;
			    }
			}
			break;
		case '?':
			fprintf(stderr, usage_format, program_name);
			exit(1);
		}
	}

	if ( start_debugger ) {
		mx_start_debugger(NULL);
	}

	if ( debug ) {
		fprintf(stderr,"items_to_show = %d\n", items_to_show);
		fprintf(stderr,"item_name     = '%s'\n", item_name);
	}

	/* Initialize the MX device drivers. */

	mx_initialize_drivers();

	/* Select the function to run. */

	switch (items_to_show) {
	case MXDI_DRIVERS:
		status = show_all_drivers( debug );
		break;
	case MXDI_SUPERCLASSES:
		status = show_drivers( items_to_show, NULL, debug );
		break;
	case MXDI_CLASSES:
	case MXDI_TYPES:
		status = show_drivers( items_to_show, item_name, debug );
		break;
	case MXDI_FIELDS:
		status = show_field_list( item_name,
					show_all_fields, show_handles, debug );
		break;
	case MXDI_VERSION:
		fprintf(stderr, "\nMX version %s\n\n", mx_get_version_string());

		status = SUCCESS;
		break;
	case MXDI_LATEX_FIELDS:
		status = show_latex_field_table( item_name,
						structures_to_show,
						show_all_fields,
						show_link,
						debug );
		break;
	default:
		fprintf(stderr, usage_format, program_name);
		exit(1);
		break;
	}

	if ( status != SUCCESS )
		exit(1);

	return 0;
}

static char *
find_varargs_field_name( MX_DRIVER *driver,
			long varargs_cookie,
			mx_bool_type debug )
{
	static char defaults_string[MXU_FIELD_NAME_LENGTH + 20];
	char num_string[20];
	long i, j;
	char c;

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

	memset( defaults_string, 0, sizeof(defaults_string) );

	for ( i = 0, j = 0; i < sizeof(defaults_string) - 1; i++, j++ ) {
		c = referenced_field_defaults->name[i];

		if ( c == '\0' ) {
			defaults_string[j] = '\0';
			break;
		} else
		if ( c == '_' ) {
			defaults_string[j] = '\\';
			j++;
			defaults_string[j] = '_';
		} else {
			if ( isupper( (int) c) ) {
				defaults_string[j] = tolower( (int) c);
			} else {
				defaults_string[j] = c;
			}
		}
	}

	if ( array_in_field_index > 0 ) {
		snprintf( num_string, sizeof(num_string),
		":%ld", array_in_field_index );

		strlcat( defaults_string, num_string, sizeof(defaults_string) );
	}

	return &defaults_string[0];
}

static int
show_all_drivers( mx_bool_type debug )
{
	static const char fname[] = "show_all_drivers()";

	MX_DRIVER *superclass_driver, *class_driver;
	MX_DRIVER *current_driver;
	unsigned long old_superclass, old_class;
	char *superclass_name, *class_name;

	old_superclass = 0;
	old_class      = 0;

	superclass_name = NULL;
	class_name      = NULL;

	current_driver = mx_get_driver_by_name( NULL );

	while ( current_driver != (MX_DRIVER *) NULL ) {

		/* Find the superclass name if necessary. */

		if ( old_superclass != current_driver->mx_superclass ) {

			superclass_driver = mx_get_superclass_driver_by_type(
						current_driver->mx_superclass );

			if ( superclass_driver == (MX_DRIVER *) NULL ) {

				fprintf( stderr,
				"%s: Internal error, driver '%s' specified a "
				"superclass %ld that does not exist!\n",
						fname, current_driver->name,
						current_driver->mx_superclass );

				return FAILURE;
			}

			old_superclass = current_driver->mx_superclass;

			superclass_name = superclass_driver->name;

			/* Force the class name to be looked up again. */

			old_class = 0;
		}

		/* Find the class name if necessary. */

		if ( old_class != current_driver->mx_class ) {

			class_driver = mx_get_class_driver_by_type(
						current_driver->mx_class );

			if ( class_driver == (MX_DRIVER *) NULL ) {
				fprintf( stderr,
				"%s: Internal error, driver '%s' specified a "
				"class %ld that does not exist!\n",
						fname, current_driver->name,
						current_driver->mx_class );

				return FAILURE;
			}

			old_class = current_driver->mx_class;

			class_name = class_driver->name;
		}

		printf( "%s %s %s\n",
			superclass_name, class_name, current_driver->name );

		current_driver = current_driver->next_driver;
	}

	return SUCCESS;
}

static int
show_drivers( int items_to_show,
		char *item_name,
		mx_bool_type debug )
{
	static const char fname[] = "show_drivers()";

	MX_DRIVER *item_list;
	MX_DRIVER *driver, *current_driver, *next_driver;
	unsigned long mx_superclass, mx_class, mx_type;

	driver = NULL;
	mx_superclass = mx_class = mx_type = 0;

	switch (items_to_show) {
	case MXDI_SUPERCLASSES:
		item_list = mx_get_superclass_driver_by_name( NULL );

		if ( debug ) {
			fprintf(stderr, "%s invoked for all superclasses\n",
					fname);
		}
		break;
	case MXDI_CLASSES:
		item_list = mx_get_class_driver_by_name( NULL );

		if ( debug ) {
			fprintf(stderr, "%s invoked for superclass '%s'\n",
					fname, item_name);
		}

		driver = mx_get_superclass_driver_by_name( item_name );

		if ( driver == NULL )
			return FAILURE;

		mx_superclass = driver->mx_superclass;
		break;
	case MXDI_TYPES:
		item_list = mx_get_driver_by_name( NULL );

		if ( debug ) {
			fprintf(stderr, "%s invoked for class '%s'\n",
					fname, item_name);
		}

		driver = mx_get_class_driver_by_name( item_name );

		if ( driver == NULL )
			return FAILURE;

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

	current_driver = item_list;

	while ( 1 ) {

		if ( debug ) {
			fprintf( stderr,"current_driver = '%s'\n",
					current_driver->name );
		}

		if ( ( mx_superclass == 0 )
		  || ( mx_superclass == current_driver->mx_superclass ) )
		{
			if ( ( mx_class == 0 )
			  || ( mx_class == current_driver->mx_class ) )
			{
				if ( ( mx_type == 0 )
				  || ( mx_type == current_driver->mx_type ) )
				{
					printf("%s\n", current_driver->name);
				}
			}
		}

		next_driver = current_driver->next_driver;

		if ( next_driver == NULL ) {
			break;		/* Exit the while() loop. */
		}

		current_driver = next_driver;
	}

	return SUCCESS;
}

static int
show_field_list( char *driver_name,
		mx_bool_type show_all_fields,
		mx_bool_type show_handles,
		mx_bool_type debug )
{
	static const char fname[] = "show_field_list()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *field_defaults_array, *field_defaults;
	unsigned long i;
	int status;

	if ( debug ) {
		fprintf(stderr, "%s invoked for driver '%s'\n",
			fname, driver_name);
	}

	/* Get the requested driver. */

	driver = mx_get_driver_by_name( driver_name );

	if ( driver == (MX_DRIVER *) NULL ) {
		fprintf( stderr, "The MX driver '%s' was not found.\n",
			driver_name );

		exit(1);
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
		mx_bool_type show_handles,
		mx_bool_type debug )
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
show_latex_field_table( char *driver_name,
		unsigned long structures_to_show,
		mx_bool_type show_all_fields,
		mx_bool_type show_link,
		mx_bool_type debug )
{
	static const char fname[] = "show_latex_field_table()";

	MX_DRIVER *driver;
	MX_DRIVER *superclass_driver, *class_driver;
	MX_RECORD_FIELD_DEFAULTS *field_defaults_array, *field_defaults;
	char link_name[MXU_DRIVER_NAME_LENGTH+20];
	char macro_name[MXU_DRIVER_NAME_LENGTH+20];
	unsigned long i;
	int status;

	if ( debug ) {
		fprintf(stderr, "%s invoked for driver '%s'\n",
			fname, driver_name);
	}

	/* Get the requested driver. */

	driver = mx_get_driver_by_name( driver_name );

	if ( driver == (MX_DRIVER *) NULL ) {
		fprintf( stderr, "The MX driver '%s' was not found.\n",
			driver_name );

		exit(1);
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

	if ( show_link ) {
		/* Construct the field link for this driver. */

		if ( structures_to_show & MXF_REC_RECORD_STRUCT ) {
			/* We do not need a link for this case. */

			show_link = FALSE;
		} else
		if ( structures_to_show & MXF_REC_SUPERCLASS_STRUCT ) {
			strlcpy( link_name, "\\MxLinkRecordFields",
						sizeof(link_name) );
		} else
		if ( structures_to_show & MXF_REC_CLASS_STRUCT ) {

			superclass_driver = mx_get_superclass_driver_by_type(
							driver->mx_superclass );

			if ( superclass_driver == (MX_DRIVER *) NULL ) {
				fprintf(stderr,
				"Driver superclass %ld for MX driver '%s' "
				"was not found.\n",
					driver->mx_superclass, driver_name );
				exit(1);
			}

			create_latex_command( link_name, sizeof(link_name),
				"\\MxLink%sFields",
				capitalize_string( superclass_driver->name ) );
		} else
		if ( structures_to_show & MXF_REC_TYPE_STRUCT ) {

			class_driver = mx_get_class_driver_by_type(
							driver->mx_class );

			if ( class_driver == (MX_DRIVER *) NULL ) {
				fprintf(stderr,
				"Driver class %ld for MX driver '%s' "
				"was not found.\n",
					driver->mx_class, driver_name );
				exit(1);
			}

			create_latex_command( link_name, sizeof(link_name),
				"\\MxLink%sFields",
				capitalize_string( class_driver->name ) );
		} else {
			/* We do not need a link for this case. */

			show_link = FALSE;
		}
	}

	/* Print out a header for the LaTeX table. */

	create_latex_command( macro_name, sizeof(macro_name),
			"\\Mx%sDriverFields", capitalize_string(driver->name) );

	/* Create the command to display the field table. */

	printf( "  \\newcommand{%s}{\n", macro_name );
	printf( "    \\begin{tabularx}{1.0\\textwidth}{@{\\extracolsep{\\fill}} |c|c|c|c|X|}\n" );
	printf( "    \\hline\n" );
	printf(
	"    \\MxTextFieldName & \\MxTextFieldType & \\MxTextNumDimensions\n" );
	printf(
	"            & \\MxTextSizes & \\MxTextDescription \\\\\n" );
	printf( "    \\hline\n" );

	if ( show_link ) {
	    printf( "    \\multicolumn{5}{|c|}{%s} \\\\\n", link_name );
	    printf( "    \\hline\n" );
	}

	/* Show the field list. */

	field_defaults_array = *(driver->record_field_defaults_ptr);

	for ( i = 0; i < *(driver->num_record_fields); i++ ) {

		field_defaults = &field_defaults_array[i];

		if ( ( show_all_fields == TRUE )
		  || ( field_defaults->flags & MXFF_IN_DESCRIPTION ) )
		{
			if ( field_defaults->structure_id & structures_to_show )
			{
				status = show_latex_field( driver,
						field_defaults, debug );
			}
		}
	}

	printf( "  \\end{tabularx}\n" );
	printf( "}\n" );

	return SUCCESS;
}

static int
show_latex_field( MX_DRIVER *driver,
		MX_RECORD_FIELD_DEFAULTS *field_defaults,
		mx_bool_type debug )
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

	printf( "    \\textit{%s} & ", buffer );

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

	printf( "%s & ", buffer );

	/* Display the number of dimensions. */

	if ( field_defaults->num_dimensions < 0 ) {
		num_dimensions = MXU_FIELD_MAX_DIMENSIONS;

		printf( " \\textit{%s} & ", find_varargs_field_name( driver,
					field_defaults->num_dimensions,
					debug ) );
	} else {
		num_dimensions = field_defaults->num_dimensions;

		printf( "%ld & ", num_dimensions );
	}

	/* Display each of the dimensions in turn. */

	if ( num_dimensions == 0 ) {
		printf( "0" );
	} else
	if ( num_dimensions == 1 ) {
		dimension = field_defaults->dimension[0];

		if ( dimension < 0 ) {
			printf( "\\textit{%s}",
				find_varargs_field_name( driver,
				dimension, debug ) );
		} else {
			printf( "%ld", dimension );
		}
	} else {
		printf( "( " );

		for ( i = 0; i < num_dimensions; i++ ) {

			if ( i > 0 ) {
				printf( ", " );
			}

			dimension = field_defaults->dimension[i];

			if ( dimension < 0 ) {
				printf( "\\textit{%s}",
					find_varargs_field_name( driver,
					dimension, debug ) );
			} else {
				printf( "%ld", dimension );
			}
		}
		printf( " )" );
	}

	{
		char field_command[250];

		create_latex_command( field_command, sizeof(field_command),
			"\\MxField%s%s",
			capitalize_string(driver->name),
			capitalize_string(field_defaults->name) );

		printf(
		" & $\\ifthenelse{\\isundefined{%s}}{\\relax}"
		"{\\begin{varwidth}{1.0\\linewidth}"
			"\\raggedright"
			"\\vspace*{1mm}%s\\vspace*{3mm}"
		"\\end{varwidth}}$ \\\\\n",
			field_command, field_command );
	}

	printf( "    \\hline\n" );

	return SUCCESS;
}

/*--------------------------------------------------------------------------*/

static int
create_latex_command( char *buffer,
			size_t max_buffer_length,
			char *format,
			... )
{
	va_list args;
	int i, c, length, result;

	va_start( args, format );

	result = vsnprintf( buffer, max_buffer_length, format, args );

	if ( result < 0 ) {
		return result;
	}

	length = strlen(buffer);

	if ( length <= 1 ) {
		return -1;
	}

	/* The first letter _must_ be a backslash '\' character. */

	if ( buffer[0] != '\\' ) {
		fprintf( stderr,
		"The first character '%c' of Latex command string '%s' "
		"is not a backslash '\\' character.\n",
			buffer[0], buffer );

		return -1;
	}

	/* If they are longer than 1 character, Latex command names can
	 * only contain letters, so we must convert any invalid characters
	 * into valid characters.
	 */

	for ( i = 1; i < length; i++ ) {
		c = buffer[i];

		if ( isalpha(c) ) {
			/* Leave alphabetic characters alone. */
		} else
		if ( isdigit(c) ) {
			c = 'A' + c - '0';
		} else {
			c = 'Z';
		}

		buffer[i] = c;
	}

	return result;
}

/*--------------------------------------------------------------------------*/

static char *
capitalize_string( char *buffer )
{
	if ( islower( (int) buffer[0]) ) {
		buffer[0] = toupper( (int) buffer[0]);
	}

	return buffer;
}

/*--------------------------------------------------------------------------*/

