/*
 * Name:    mdialog.h
 *
 * Purpose: Function definitions for user interface functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MDIALOG_H__
#define __MDIALOG_H__

extern int motor_get_int( FILE *file, char *prompt,
			int have_default, int default_value, int *value,
			int lower_limit, int upper_limit);

extern int motor_get_long( FILE *file, char *prompt,
			int have_default, long default_value, long *value,
			long lower_limit, long upper_limit);

extern int motor_get_double( FILE *file, char *prompt,
			int have_default, double default_value, double *value,
			double lower_limit, double upper_limit);

extern int motor_get_string( FILE *file, char *prompt, char *default_value,
			int *string_length, char *string );

extern int motor_get_string_from_list( FILE *file, char *prompt,
		int num_strings, int *min_length_array, char **string_array,
		int default_string_number, int *string_length,
		char *selected_string );

#endif /* __MDIALOG_H__ */

