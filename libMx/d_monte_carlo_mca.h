/* 
 * Name:    d_monte_carlo_mca.h
 *
 * Purpose: Header file for MX software-emulated multichannel analyzers
 *          using simple Monte Carlo calculations.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2012, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MONTE_CARLO_MCA_H__
#define __D_MONTE_CARLO_MCA_H__

/*---*/

#define MXT_MONTE_CARLO_MCA_SOURCE_UNIFORM	1
#define MXT_MONTE_CARLO_MCA_SOURCE_POLYNOMIAL	2
#define MXT_MONTE_CARLO_MCA_SOURCE_GAUSSIAN	3
#define MXT_MONTE_CARLO_MCA_SOURCE_LORENTZIAN	4

/*---*/

typedef struct {
	double events_per_second;
} MX_MONTE_CARLO_MCA_SOURCE_UNIFORM;

typedef struct {
	double events_per_second;
	double a0;
	double a1;
	double a2;
	double a3;
} MX_MONTE_CARLO_MCA_SOURCE_POLYNOMIAL;

typedef struct {
	double events_per_second;
	double mean;
	double standard_deviation;
} MX_MONTE_CARLO_MCA_SOURCE_GAUSSIAN;

typedef struct {
	double events_per_second;
	double mean;
	double full_width_half_maximum;
} MX_MONTE_CARLO_MCA_SOURCE_LORENTZIAN;

/*---*/

struct mx_monte_carlo_mca_type;

typedef struct mx_monte_carlo_mca_source_type {
	unsigned long type;
	mx_status_type (*process)( MX_MCA *mca,
			struct mx_monte_carlo_mca_type *monte_carlo_mca,
			struct mx_monte_carlo_mca_source_type *source );
	union {
		MX_MONTE_CARLO_MCA_SOURCE_UNIFORM uniform;
		MX_MONTE_CARLO_MCA_SOURCE_POLYNOMIAL polynomial;
		MX_MONTE_CARLO_MCA_SOURCE_GAUSSIAN gaussian;
		MX_MONTE_CARLO_MCA_SOURCE_LORENTZIAN lorentzian;
	} u;
} MX_MONTE_CARLO_MCA_SOURCE;

/*---*/

#define MXU_MONTE_CARLO_MCA_MAX_SOURCE_STRING_LENGTH	80

typedef struct mx_monte_carlo_mca_type {
	MX_RECORD *record;

	double time_step_size;			/* in seconds */
	long num_sources;
	char **source_string_array;

	unsigned long sleep_microseconds;

	MX_MONTE_CARLO_MCA_SOURCE *source_array;

	MX_CLOCK_TICK start_time_in_clock_ticks;
	MX_CLOCK_TICK finish_time_in_clock_ticks;

	MX_MUTEX *mutex;
	MX_THREAD *event_thread;
	unsigned long *private_array;
} MX_MONTE_CARLO_MCA;

#define MXD_MONTE_CARLO_MCA_STANDARD_FIELDS \
  {-1, -1, "time_step_size", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MONTE_CARLO_MCA, time_step_size), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "num_sources", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MONTE_CARLO_MCA, num_sources), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "source_string_array", MXFT_STRING, NULL, \
    2, {MXU_VARARGS_LENGTH, MXU_MONTE_CARLO_MCA_MAX_SOURCE_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MONTE_CARLO_MCA, source_string_array),\
	{sizeof(char), sizeof(char *)}, NULL, \
	    (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY | MXFF_VARARGS) }

MX_API mx_status_type mxd_monte_carlo_mca_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_monte_carlo_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_monte_carlo_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_monte_carlo_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_monte_carlo_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_monte_carlo_mca_open( MX_RECORD *record );

MX_API mx_status_type mxd_monte_carlo_mca_start( MX_MCA *mca );
MX_API mx_status_type mxd_monte_carlo_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_monte_carlo_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_monte_carlo_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_monte_carlo_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_monte_carlo_mca_get_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_monte_carlo_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_monte_carlo_mca_mca_function_list;

extern long mxd_monte_carlo_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_monte_carlo_mca_rfield_def_ptr;

#endif /* __D_MONTE_CARLO_MCA_H__ */
