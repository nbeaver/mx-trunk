/*
 * Name:    motor.h
 *
 * Purpose: Header file for simple motor program main routine.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------
 *
 * Copyright 1999-2011, 2014-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MOTOR_H__
#define __MOTOR_H__

#define MOTOR_PROCESS_FIELDS	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#include "mx_osdef.h"
#include "mx_driver.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_motor.h"
#include "mx_scan.h"
#include "mx_measurement.h"
#include "command.h"

#if defined(OS_SOLARIS)
    /* Solaris uses the poisoned identifier sprintf() in its header files. */

#elif defined(OS_WIN32) && defined(__GNUC__)
    /* Windows headers use the poisoned identifier strcpy(), which trips
     * up compilation with MinGW.
     */
#else
#   include "mx_poison.h"
#endif

#ifndef SUCCESS
#define INTERRUPTED 2
#define SUCCESS     1
#define FAILURE     0
#endif

/* Command line processor macros. */

#define MX_CMDLINE_FGETS	1
#define MX_CMDLINE_READLINE	2

#if defined(OS_RTEMS) || defined(OS_VXWORKS)

#   define MX_CMDLINE_PROCESSOR  MX_CMDLINE_FGETS

#elif defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_DJGPP) \
	|| defined(OS_MINIX)

#   define MX_CMDLINE_PROCESSOR  MX_CMDLINE_READLINE

#elif defined(OS_WIN32)

#   define MX_CMDLINE_PROCESSOR  MX_CMDLINE_FGETS

#elif defined(OS_VMS)

#   define MX_CMDLINE_PROCESSOR  MX_CMDLINE_VMS

#else

#   define MX_CMDLINE_PROCESSOR  MX_CMDLINE_FGETS

#endif

/* Define function prototypes for readline if needed. */

#if ( MX_CMDLINE_PROCESSOR == MX_CMDLINE_READLINE )

    extern char *readline( char *prompt );
    void add_history( char *string );

#endif

/* Just in case TRUE and FALSE are not defined. */

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#if defined(OS_RTEMS)
extern void motor_rtems_reboot( void );
#endif

extern int motor_main( int argc, char *argv[] );

extern int motor_area_detector_fn( int argc, char *argv[] );
extern int motor_cd_fn( int argc, char *argv[] );
extern int motor_close_fn( int argc, char *argv[] );
extern int motor_copy_or_rename_fn( int argc, char *argv[] );
extern int motor_count_fn( int argc, char *argv[] );
extern int motor_debug_fn( int argc, char *argv[] );
extern int motor_delete_fn( int argc, char *argv[] );
extern int motor_exec_fn( int argc, char *argv[] );
extern int motor_execq_fn( int argc, char *argv[] );
extern int motor_exit_fn( int argc, char *argv[] );
extern int motor_gpib_fn( int argc, char *argv[] );
extern int motor_help_fn( int argc, char *argv[] );
extern int motor_home_fn( int argc, char *argv[] );
extern int motor_kill_fn( int argc, char *argv[] );
extern int motor_load_fn( int argc, char *argv[] );
extern int motor_mabs_fn( int argc, char *argv[] );
extern int motor_mca_fn( int argc, char *argv[] );
extern int motor_mcs_fn( int argc, char *argv[] );
extern int motor_measure_fn( int argc, char *argv[] );
extern int motor_mjog_fn( int argc, char *argv[] );
extern int motor_modify_fn( int argc, char *argv[] );
extern int motor_mrel_fn( int argc, char *argv[] );
extern int motor_open_fn( int argc, char *argv[] );
extern int motor_ptz_fn( int argc, char *argv[] );
extern int motor_readp_fn( int argc, char *argv[] );
extern int motor_resync_fn( int argc, char *argv[] );
extern int motor_rs232_fn( int argc, char *argv[] );
extern int motor_sample_changer_fn( int argc, char *argv[] );
extern int motor_save_fn( int argc, char *argv[] );
extern int motor_sca_fn( int argc, char *argv[] );
extern int motor_scan_fn( int argc, char *argv[] );
extern int motor_script_fn( int argc, char *argv[] );
extern int motor_set_fn( int argc, char *argv[] );
extern int motor_setup_fn( int argc, char *argv[] );
extern int motor_show_fn( int argc, char *argv[] );
extern int motor_showall_fn( int argc, char *argv[] );
extern int motor_start_fn( int argc, char *argv[] );
extern int motor_stop_fn( int argc, char *argv[] );
extern int motor_system_fn( int argc, char *argv[] );
extern int motor_test_fn( int argc, char *argv[] );
extern int motor_vinput_fn( int argc, char *argv[] );
extern int motor_writep_fn( int argc, char *argv[] );
extern int motor_wvout_fn( int argc, char *argv[] );

extern int motor_init( char *motor_savefile_name, int num_savefiles,
			char scan_savefile_array[][MXU_FILENAME_LENGTH+1],
			mx_bool_type init_hw_flags,
			mx_bool_type verify_drivers,
			unsigned long network_debug_flags );

extern int motor_exit_save_dialog( void );

extern int motor_expand_pathname( char *filename, int max_filename_length );
extern int motor_exec_script( char *script_name, int verbose_flag );
extern int motor_exec_common( char *script_name, int verbose_flag );
extern int motor_set_motor( MX_RECORD *motor_record, int argc, char *argv[] );
extern int motor_set_amplifier( MX_RECORD *motor_record,
						int argc, char *argv[] );
extern int motor_show_all_devices( void );
extern int motor_show_records( long superclass, long class, long type,
				char *match_string );
extern int motor_show_record( long superclass, long class, long type,
				char *record_type_name, char *record_name,
				int show_all, int update_all );
extern int motor_show_field( char *fieldname );
extern int motor_show_modules( char *module_name );

extern int motor_check_for_datafile_name_collision( MX_SCAN *scan );
extern int motor_prompt_for_scan_header( MX_SCAN *scan );

extern int motor_setup_scan_motor_names( char *buffer,
				size_t buffer_length,
				long old_scan_num_motors,
				long scan_num_motors,
				MX_RECORD **motor_record_array,
				char **motor_name_array );

extern int motor_setup_input_devices( MX_SCAN *old_scan,
				long scan_class, long scan_type,
				char *input_devices_buffer,
				size_t input_devices_buffer_length,
				MX_RECORD **first_input_device_record );

extern int motor_prompt_for_input_devices( MX_SCAN *old_scan,
				long scan_class, long scan_type,
				char *input_devices_buffer,
				size_t input_devices_buffer_length,
				MX_RECORD **first_input_device_record );

extern int motor_copy_input_devices( MX_SCAN *old_scan,
				long scan_class, long scan_type,
				char *input_devices_buffer,
				size_t input_devices_buffer_length,
				MX_RECORD **first_input_device_record );

extern int motor_setup_measurement_parameters( MX_SCAN *old_scan,
				char *measurement_parameters_buffer,
				size_t measurement_parameters_buffer_length,
				int setup_measurement_interval );

extern int motor_setup_preset_time_measurement( MX_SCAN *old_scan,
				char *measurement_parameters_buffer,
				size_t measurement_parameters_buffer_length,
				int setup_measurement_interval );

extern int motor_setup_preset_count_measurement( MX_SCAN *old_scan,
				char *measurement_parameters_buffer,
				size_t measurement_parameters_buffer_length,
				int setup_measurement_interval );

extern int motor_setup_datafile_and_plot_parameters( MX_SCAN *old_scan,
				long scan_class, long scan_type,
				char *datafile_and_plot_buffer,
				size_t datafile_and_plot_buffer_length );

extern int motor_setup_linear_scan_parameters( char *scan_name,
				MX_SCAN *old_scan,
				char *buffer,
				size_t buffer_length,
				char *input_devices_string,
				char *measurement_parameters_string,
				char *datafile_and_plot_parameters_string );

extern int motor_setup_list_scan_parameters( char *scan_name,
				MX_SCAN *old_scan,
				char *buffer,
				size_t buffer_length,
				char *input_devices_string,
				char *measurement_parameters_string,
				char *datafile_and_plot_parameters_string );

extern int motor_setup_xafs_scan_parameters( char *scan_name,
				MX_SCAN *old_scan,
				char *buffer,
				size_t buffer_length,
				char *input_devices_string,
				char *measurement_parameters_string,
				char *datafile_and_plot_parameters_string );

extern int motor_setup_quick_scan_parameters( char *scan_name,
				MX_SCAN *old_scan,
				char *buffer,
				size_t buffer_length,
				char *input_devices_string,
				char *measurement_parameters_string,
				char *datafile_and_plot_parameters_string );

extern int motor_setup_area_detector_scan_parameters( char *scan_name,
				MX_SCAN *old_scan,
				char *buffer,
				size_t buffer_length,
				char *input_devices_string,
				char *measurement_parameters_string,
				char *datafile_and_plot_parameters_string );

extern mx_status_type motor_move_report_function(
	unsigned long flags, long num_motors, MX_RECORD **motor_record );

extern mx_status_type motor_copy_file( char *source_filename,
			char *destination_filename );
extern mx_status_type motor_make_backup_copy( char *filename_to_backup );

extern mx_status_type motor_scan_pause_request_handler( MX_SCAN *scan );

/* Global variables. */

extern MX_RECORD *motor_record_list;
extern char motor_savefile[MXU_FILENAME_LENGTH + 1];
extern char scan_savefile[MXU_FILENAME_LENGTH + 1];

extern FILE *input, *output;

extern COMMAND command_list[];
extern int command_list_length;

extern int allow_motor_database_updates;
extern int allow_scan_database_updates;
extern int motor_has_unsaved_scans;
extern int motor_exit_save_policy;
extern int motor_install_signal_handlers;

extern int motor_header_prompt_on;
extern int motor_overwrite_on;
extern int motor_autosave_on;
extern int motor_estimate_on;
extern int motor_bypass_limit_switch;
extern int motor_parameter_warning_flag;

extern int motor_default_precision;

/* End of global variables. */

#endif /* __MOTOR_H__ */

