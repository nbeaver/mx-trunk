/*
 * Name:    pr_handlers.h
 *
 * Purpose: Header for functions used to process MX record field events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004-2008, 2011, 2013-2014, 2017-2019, 2022
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __PR_HANDLERS_H__
#define __PR_HANDLERS_H__

extern mx_status_type mx_setup_amplifier_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_amplifier_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_analog_input_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_analog_input_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_analog_output_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_analog_output_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_area_detector_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_area_detector_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_autoscale_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_autoscale_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_digital_input_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_digital_input_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_digital_output_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_digital_output_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_gpib_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_gpib_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_list_head_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_list_head_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

extern mx_status_type mx_list_head_print_clients( MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_show_cpu_type( MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_show_process_memory( 
						MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_show_system_memory( 
						MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_report( MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_report_all( MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_update_all( MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_summary( MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_show_record_list(
						MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_fielddef( MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_show_field(MX_LIST_HEAD *list_head);

extern mx_status_type mx_list_head_record_show_handle(MX_LIST_HEAD *list_head);

extern mx_status_type mx_list_head_record_show_callbacks(
						MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_show_clbk_id(
						MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_show_socket_handlers(
						MX_LIST_HEAD *list_head );

extern mx_status_type mx_list_head_record_show_socket_id(
						MX_LIST_HEAD *list_head );

/*---*/

extern mx_status_type mx_setup_mca_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_mca_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_mcai_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_mcai_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_mce_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_mce_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_mcs_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_mcs_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_motor_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_motor_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_operation_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_operation_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_ptz_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_ptz_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_pulser_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_pulser_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_record_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_record_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_relay_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_relay_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_rs232_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_rs232_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_sample_changer_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_sample_changer_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_sca_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_sca_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_scaler_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_scaler_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_timer_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_timer_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_variable_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_variable_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_video_input_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_video_input_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_vme_process_functions( MX_RECORD *record_list );

extern mx_status_type mx_vme_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_waveform_input_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_waveform_input_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_waveform_output_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_waveform_output_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

/*---*/

extern mx_status_type mx_setup_clock_process_functions(
					MX_RECORD *record_list );

extern mx_status_type mx_clock_process_function(
					void *record, void *record_field,
					void *socket_handler, int operation );

#endif /* __PR_HANDLERS_H__ */
