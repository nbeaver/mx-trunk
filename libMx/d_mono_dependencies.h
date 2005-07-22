/*
 * Name:    d_mono_dependencies.h
 *
 * Purpose: Private declarations of monochromator dependency type handlers.
 *
 *          This file is intended to be included only by the file
 *          'libMx/d_monochromator.c' and should not be include by
 *          any other file.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MONO_DEPENDENCIES_H__
#define __D_MONO_DEPENDENCIES_H__

static mx_status_type mxd_monochromator_get_pointers( MX_MOTOR *motor,
					MX_MONOCHROMATOR **monochromator,
					const char *calling_fname );

static mx_status_type mxd_monochromator_get_enable_status(
					MX_RECORD *list_record, 
					int *dependency_type );

static mx_status_type mxd_monochromator_get_dependency_type(
					MX_RECORD *list_record, 
					int *dependency_type );

static mx_status_type mxd_monochromator_get_param_array_from_list(
					MX_RECORD *list_record,
					int *num_parameters,
					double **parameter_array );

static mx_status_type mxd_monochromator_get_record_array_from_list(
					MX_RECORD *list_record,
					int *num_records,
					MX_RECORD ***record_array );

/* === */

static mx_status_type mxd_monochromator_get_theta_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *theta_position );
static mx_status_type mxd_monochromator_set_theta_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position );
static mx_status_type mxd_monochromator_move_theta(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_energy_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *theta_position );
static mx_status_type mxd_monochromator_set_energy_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position );
static mx_status_type mxd_monochromator_move_energy(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_polynomial_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *theta_position );
static mx_status_type mxd_monochromator_set_polynomial_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position );
static mx_status_type mxd_monochromator_move_polynomial(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_insertion_device_energy_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *id_energy_position );
static mx_status_type mxd_monochromator_set_insertion_device_energy_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double id_energy_position );
static mx_status_type mxd_monochromator_move_insertion_device_energy(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_bragg_normal_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *bragg_normal_position );
static mx_status_type mxd_monochromator_set_bragg_normal_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double bragg_normal_position );
static mx_status_type mxd_monochromator_move_bragg_normal(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_bragg_parallel_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *bragg_parallel_position );
static mx_status_type mxd_monochromator_set_bragg_parallel_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double bragg_parallel_position );
static mx_status_type mxd_monochromator_move_bragg_parallel(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_table_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *table_position );
static mx_status_type mxd_monochromator_set_table_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double table_position );
static mx_status_type mxd_monochromator_move_table(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_diffractometer_theta_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *theta_position );
static mx_status_type mxd_monochromator_set_diffractometer_theta_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position );
static mx_status_type mxd_monochromator_move_diffractometer_theta(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_e_polynomial_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *energy_polynomial_position );
static mx_status_type mxd_monochromator_set_e_polynomial_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double energy_polynomial_position );
static mx_status_type mxd_monochromator_move_e_polynomial(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

static mx_status_type mxd_monochromator_get_option_selector_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double *energy_polynomial_position );
static mx_status_type mxd_monochromator_set_option_selector_position(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double energy_polynomial_position );
static mx_status_type mxd_monochromator_move_option_selector(
					MX_MONOCHROMATOR *monochromator,
					MX_RECORD *list_record, 
					long dependency_number,
					double theta_position,
					double old_theta_position,
					int flags );

#endif /* __D_MONO_DEPENDENCIES_H__ */
