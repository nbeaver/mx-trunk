/*
 * Name:    mx_driver.c
 *
 * Purpose: Describes the list of interfaces and devices included in
 *          this version of the program.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_stdint.h"

#include "mx_driver.h"
#include "mx_hrt_debug.h"
#include "mx_interval_timer.h"
#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_semaphore.h"
#include "mx_image.h"

/* -- Include header files that define MX_XXX_FUNCTION_LIST structures. -- */

#include "mx_rs232.h"
#include "mx_gpib.h"
#include "mx_camac.h"
#include "mx_generic.h"
#include "mx_portio.h"
#include "mx_vme.h"
#include "mx_modbus.h"
#include "mx_usb.h"
#include "mx_camera_link.h"

#if HAVE_TCPIP
#include "mx_socket.h"
#endif

#include "mx_net.h"
#include "mx_spec.h"

#include "mx_callback.h"

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_encoder.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_amplifier.h"
#include "mx_relay.h"
#include "mx_mca.h"
#include "mx_mce.h"
#include "mx_mcs.h"
#include "mx_table.h"
#include "mx_autoscale.h"
#include "mx_pulse_generator.h"
#include "mx_sca.h"
#include "mx_sample_changer.h"
#include "mx_mcai.h"
#include "mx_ptz.h"
#include "mx_video_input.h"
#include "mx_area_detector.h"
#include "mx_waveform_input.h"
#include "mx_waveform_output.h"

#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "mx_scan_list.h"
#include "mx_scan_xafs.h"
#include "mx_scan_quick.h"
#include "mx_scan_area_detector.h"

#include "mx_variable.h"
#include "mx_vinline.h"
#include "mx_vnet.h"
#include "mx_vfile.h"
#include "mx_vfield.h"

#include "mx_bluice.h"

#include "mx_list_head.h"

#include "mx_dead_reckoning.h"

#include "mx_operation.h"

/* Include the header files for all of the interfaces and devices. */

#include "i_soft_rs232.h"
#include "i_network_rs232.h"
#include "i_tty.h"
#include "i_dos_com.h"
#include "i_fossil.h"
#include "i_ks3344.h"
#include "i_wago750_serial.h"

#ifdef OS_WIN32
#include "i_win32_com.h"
#endif

#ifdef OS_VMS
#include "i_vms_terminal.h"
#endif

#ifdef OS_VXWORKS
#include "i_vxworks_rs232.h"
#endif

#include "i_camera_link_rs232.h"
#include "i_spec_command.h"
#include "i_sim900_port.h"

#if HAVE_TCPIP
#include "i_tcp232.h"
#include "i_modbus_tcp.h"
#include "i_telnet.h"
#endif

#include "i_network_gpib.h"
#include "i_k500serial.h"
#include "i_micro488ex.h"
#include "i_prologix.h"

#include "i_soft_camac.h"
#include "i_dsp6001.h"

#include "i_linux_iopl.h"
#include "i_dos_portio.h"

#include "i_vxworks_vme.h"
#include "i_mmap_vme.h"
#include "i_rtems_vme.h"

#include "i_modbus_serial_rtu.h"

#include "i_soft_camera_link.h"
#include "i_camera_link_api.h"

#include "i_pdi40.h"
#include "i_pdi45.h"
#include "i_8255.h"
#include "i_6821.h"
#include "i_lpt.h"
#include "i_newport.h"
#include "i_pmac.h"
#include "i_compumotor.h"
#include "i_d8.h"
#include "i_vp9000.h"
#include "i_ortec974.h"
#include "i_am9513.h"
#include "i_hsc1.h"
#include "i_vme58.h"
#include "i_vsc16.h"
#include "i_pcstep.h"
#include "i_databox.h"
#include "i_sis3807.h"
#include "i_mardtb.h"
#include "i_uglide.h"
#include "i_aps_adcmod2.h"
#include "i_linux_parport.h"
#include "i_cxtilt02.h"
#include "i_iseries.h"
#include "i_phidget_old_stepper.h"
#include "i_pfcu.h"
#include "i_keithley2700.h"
#include "i_keithley2400.h"
#include "i_keithley2000.h"
#include "i_kohzu_sc.h"
#include "i_picomotor.h"
#include "i_sr630.h"
#include "i_tpg262.h"
#include "i_cm17a.h"
#include "i_sony_visca.h"
#include "i_panasonic_kx_dp702.h"
#include "i_spellman_df3.h"
#include "i_bkprecision_912x.h"
#include "i_isobus.h"
#include "i_itc503.h"
#include "i_ilm.h"
#include "i_linkam_t9x.h"
#include "i_sim900.h"

#include "d_ks3512.h"
#include "d_ks3112.h"
#include "d_ks3063.h"
#include "d_8255.h"
#include "d_6821.h"
#include "d_lpt.h"
#include "d_bit.h"
#include "d_soft_ainput.h"
#include "d_soft_aoutput.h"
#include "d_soft_dinput.h"
#include "d_soft_doutput.h"
#include "d_network_ainput.h"
#include "d_network_aoutput.h"
#include "d_network_dinput.h"
#include "d_network_doutput.h"
#include "d_vme_dio.h"
#include "d_portio_dio.h"
#include "d_pmac_aio.h"
#include "d_pmac_dio.h"
#include "d_compumotor_dio.h"
#include "d_mclennan_aio.h"
#include "d_mclennan_dio.h"
#include "d_mca_value.h"
#include "d_mca_weighted_sum.h"
#include "d_mardtb_status.h"
#include "d_mdrive_aio.h"
#include "d_mdrive_dio.h"
#include "d_linux_parport.h"
#include "d_pdi45_aio.h"
#include "d_pdi45_dio.h"
#include "d_cxtilt02.h"
#include "d_modbus_aio.h"
#include "d_modbus_dio.h"
#include "d_wago750_modbus_aout.h"
#include "d_wago750_modbus_dout.h"
#include "d_iseries_aio.h"
#include "d_iseries_dio.h"
#include "d_mcai_function.h"
#include "d_keithley2700_ainput.h"
#include "d_keithley2400_ainput.h"
#include "d_keithley2400_aoutput.h"
#include "d_keithley2400_doutput.h"
#include "d_keithley2000_ainput.h"
#include "d_pfcu_filter_summary.h"
#include "d_picomotor_aio.h"
#include "d_picomotor_dio.h"
#include "d_tracker_aio.h"
#include "d_tracker_dio.h"
#include "d_sr630_ainput.h"
#include "d_sr630_aoutput.h"
#include "d_p6000a.h"
#include "d_tpg262_pressure.h"
#include "d_cm17a_doutput.h"
#include "d_bluice_ion_chamber.h"
#include "d_spellman_df3_aio.h"
#include "d_spellman_df3_dio.h"
#include "d_bkprecision_912x_aio.h"
#include "d_bkprecision_912x_dio.h"
#include "d_ilm_ainput.h"
#include "d_ilm_status.h"
#include "d_ilm_sample_rate.h"
#include "d_sd_m107.h"
#include "d_linkam_t9x_pump.h"
#include "d_sim980.h"
#include "d_synaccess_netbooter_ainput.h"
#include "d_keyboard_dinput.h"
#include "d_file_dinput.h"
#include "d_ainput_as_dinput.h"
#include "d_aoutput_as_doutput.h"
#include "d_relay_as_doutput.h"
#include "d_digital_fanout.h"

#include "d_soft_motor.h"
#include "d_e500.h"
#include "d_smc24.h"
#include "d_panther.h"
#include "d_mmc32.h"
#include "d_pm304.h"
#include "d_newport.h"
#include "d_pmac.h"
#include "d_pmac_cs_axis.h"
#include "d_compumotor.h"
#include "d_network_motor.h"
#include "d_d8.h"
#include "d_vp9000.h"
#include "d_e662.h"
#include "d_hsc1.h"
#include "d_stp100.h"
#include "d_vme58.h"
#include "d_pcstep.h"
#include "d_dac_motor.h"
#include "d_lakeshore330_motor.h"
#include "d_disabled_motor.h"
#include "d_mardtb_motor.h"
#include "d_uglide.h"
#include "d_mclennan.h"
#include "d_mdrive.h"
#include "d_phidget_old_stepper.h"
#include "d_spec_motor.h"
#include "d_kohzu_sc.h"
#include "d_picomotor.h"
#include "d_mcu2.h"
#include "d_bluice_motor.h"
#include "d_ptz_motor.h"
#include "d_src_mono.h"
#include "d_linkam_t9x_motor.h"
#include "d_linkam_t9x_temp.h"
#include "d_sim960.h"

#include "d_energy.h"
#include "d_wavelength.h"
#include "d_wavenumber.h"
#include "d_slit_motor.h"
#include "d_trans_motor.h"
#include "d_xafs_wavenumber.h"
#include "d_delta_motor.h"
#include "d_elapsed_time.h"
#include "d_monochromator.h"
#include "d_linear_function.h"
#include "d_table_motor.h"
#include "d_theta_2theta.h"
#include "d_qmotor.h"
#include "d_segmented_move.h"
#include "d_tangent_arm.h"
#include "d_aframe_detector_motor.h"
#include "d_adsc_two_theta.h"
#include "d_record_field_motor.h"
#include "d_gated_backlash.h"
#include "d_polynomial_motor.h"
#include "d_cubic_spline_motor.h"
#include "d_limited_move.h"

#include "d_als_dewar_positioner.h"
#include "d_coordinated_angle.h"

#include "d_compumotor_linear.h"

#include "d_ks3640.h"
#include "d_soft_scaler.h"
#include "d_network_scaler.h"
#include "d_auto_scaler.h"
#include "d_gain_tracking_scaler.h"
#include "d_scaler_function.h"
#include "d_qs450.h"
#include "d_ortec974_scaler.h"
#include "d_ortec974_timer.h"
#include "d_vsc16_scaler.h"
#include "d_vsc16_timer.h"
#include "d_pdi45_scaler.h"
#include "d_pdi45_timer.h"
#include "d_soft_timer.h"
#include "d_network_timer.h"
#include "d_timer_fanout.h"
#include "d_rtc018.h"
#include "d_pfcu_shutter_timer.h"
#include "d_spec_scaler.h"
#include "d_spec_timer.h"
#include "d_gm10_scaler.h"
#include "d_gm10_timer.h"
#include "d_interval_timer.h"
#include "d_bluice_timer.h"
#include "d_fg_scaler.h"
#include "d_area_detector_timer.h"

#include "d_network_relay.h"
#include "d_generic_relay.h"
#include "d_blind_relay.h"
#include "d_pulsed_relay.h"
#include "d_pfcu.h"
#include "d_marccd_shutter.h"
#include "d_mardtb_shutter.h"
#include "d_bluice_shutter.h"
#include "d_synaccess_netbooter_relay.h"

#include "d_soft_amplifier.h"
#include "d_net_amplifier.h"
#include "d_keithley428.h"
#include "d_sr570.h"
#include "d_udt_tramp.h"
#include "d_keithley2700_amp.h"
#include "d_keithley2400_amp.h"
#include "d_sim983.h"

#include "d_adc_table.h"
#include "d_pdi40.h"

#include "d_aps_adcmod2_amplifier.h"
#include "d_aps_adcmod2_ainput.h"

#include "d_icplus.h"
#include "d_icplus_aio.h"
#include "d_icplus_dio.h"
#include "d_qbpm_mcai.h"

#include "d_cryostream600_motor.h"
#include "d_cryostream600_status.h"

#include "d_si9650_motor.h"
#include "d_si9650_status.h"

#include "d_itc503_motor.h"
#include "d_itc503_ainput.h"
#include "d_itc503_aoutput.h"
#include "d_itc503_doutput.h"

#include "d_am9513_motor.h"
#include "d_am9513_scaler.h"
#include "d_am9513_timer.h"

#include "d_smartmotor.h"
#include "d_smartmotor_aio.h"
#include "d_smartmotor_dio.h"

#include "d_soft_mca.h"
#include "d_network_mca.h"
#include "d_roentec_rcl_mca.h"
#include "d_monte_carlo_mca.h"

#include "d_sis3801.h"

#include "d_mca_channel.h"
#include "d_mca_roi_integral.h"
#include "d_mca_timer.h"
#include "d_mca_alt_time.h"

#include "d_databox_motor.h"
#include "d_databox_scaler.h"
#include "d_databox_timer.h"
#include "d_databox_mcs.h"
#include "d_databox_mce.h"

#include "i_i404.h"
#include "d_i404_amp.h"
#include "d_i404_mcai.h"

#include "i_synaccess_netbooter.h"

#include "d_network_pulser.h"
#include "d_sis3807.h"
#include "d_sis3801_pulser.h"
#include "d_pdi45_pulser.h"
#include "d_doutput_pulser.h"
#include "d_gittelson_pulser.h"

#include "d_soft_sca.h"
#include "d_network_sca.h"
#include "d_cyberstar_x1000.h"
#include "d_cyberstar_x1000_aout.h"


#include "d_soft_sample_changer.h"
#include "d_net_sample_changer.h"
#include "d_sercat_als_robot.h"

#include "d_network_mcai.h"

#include "d_soft_ptz.h"
#include "d_network_ptz.h"
#include "d_sony_visca_ptz.h"
#include "d_hitachi_kp_d20.h"
#include "d_panasonic_kx_dp702.h"

#include "d_soft_vinput.h"
#include "d_network_vinput.h"
#include "d_file_vinput.h"
#include "d_sony_snc.h"

#include "d_soft_area_detector.h"
#include "d_network_area_detector.h"
#include "d_aviex_pccd.h"
#include "d_marccd.h"
#include "d_marccd_server_socket.h"
#include "d_bluice_area_detector.h"
#include "d_mlfsom.h"
#include "d_mar345.h"

#include "d_network_wvin.h"

#include "d_network_wvout.h"
#include "d_bkprecision_912x_wvout.h"

#include "s_input.h"
#include "s_motor.h"
#include "s_theta_2theta.h"
#include "s_slit.h"
#include "s_pseudomotor.h"
#include "s_k_power_law.h"
#include "sl_file.h"
#include "sxafs_std.h"
#include "sq_mcs.h"
#include "sq_energy_mcs.h"
#include "sa_wedge.h"

#include "d_auto_amplifier.h"
#include "d_auto_filter.h"
#include "d_auto_filter_amplifier.h"
#include "d_auto_net.h"

#include "d_soft_mcs.h"
#include "d_network_mcs.h"
#include "d_scaler_function_mcs.h"
#include "d_mcs_scaler.h"
#include "d_mcs_timer.h"
#include "d_mcs_mce.h"
#include "d_mcs_time_mce.h"

#include "d_network_mce.h"
#include "d_pmac_mce.h"

#include "i_scipe.h"
#include "d_scipe_motor.h"
#include "d_scipe_scaler.h"
#include "d_scipe_timer.h"
#include "d_scipe_aio.h"
#include "d_scipe_dio.h"
#include "d_scipe_amplifier.h"

#include "v_mathop.h"

#include "v_position_select.h"

#include "v_indirect_string.h"

#include "v_pmac.h"
#include "v_spec.h"
#include "v_bluice_master.h"
#include "v_bluice_command.h"
#include "v_bluice_string.h"
#include "v_bluice_operation.h"
#include "v_bluice_self_operation.h"

#if HAVE_TCPIP
#include "n_tcpip.h"
#endif

#if HAVE_UNIX_DOMAIN_SOCKETS
#include "n_unix.h"
#endif

#if HAVE_TCPIP
#include "n_spec.h"
#include "n_bluice_dcss.h"
#include "n_bluice_dhs.h"
#include "n_bluice_dhs_manager.h"
#endif

#include "o_network.h"
#include "o_toast.h"

#include "z_external_command.h"

  /********************** Record Types **********************/

MX_DRIVER mx_type_table[] = {

{"list_head",      MXT_LIST_HEAD,    MXL_LIST_HEAD,      MXR_LIST_HEAD,
				&mxr_list_head_record_function_list,
				NULL,
				NULL,
				&mxr_list_head_num_record_fields,
				&mxr_list_head_rfield_def_ptr},

  /* =================== Interface types =================== */

{"soft_rs232",     MXI_232_SOFTWARE, MXI_RS232,          MXR_INTERFACE,
				&mxi_soft_rs232_record_function_list,
				NULL,
				&mxi_soft_rs232_rs232_function_list,
				&mxi_soft_rs232_num_record_fields,
				&mxi_soft_rs232_rfield_def_ptr},

{"network_rs232",  MXI_232_NETWORK,  MXI_RS232,          MXR_INTERFACE,
				&mxi_network_rs232_record_function_list,
				NULL,
				&mxi_network_rs232_rs232_function_list,
				&mxi_network_rs232_num_record_fields,
				&mxi_network_rs232_rfield_def_ptr},

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_RTEMS)
{"tty",            MXI_232_TTY,      MXI_RS232,          MXR_INTERFACE,
				&mxi_tty_record_function_list,
				NULL,
				&mxi_tty_rs232_function_list,
				&mxi_tty_num_record_fields,
				&mxi_tty_rfield_def_ptr},
#endif /* OS_UNIX */

#if defined(OS_WIN32)
{"win32_com",      MXI_232_WIN32COM, MXI_RS232,          MXR_INTERFACE,
				&mxi_win32com_record_function_list,
				NULL,
				&mxi_win32com_rs232_function_list,
				&mxi_win32com_num_record_fields,
				&mxi_win32com_rfield_def_ptr},
#endif /* OS_WIN32 */

#if defined(OS_MSDOS)
{"dos_com",        MXI_232_COM,      MXI_RS232,          MXR_INTERFACE,
				&mxi_com_record_function_list,
				NULL,
				&mxi_com_rs232_function_list,
				&mxi_com_num_record_fields,
				&mxi_com_rfield_def_ptr},

{"fossil",         MXI_232_FOSSIL,   MXI_RS232,          MXR_INTERFACE,
				&mxi_fossil_record_function_list,
				NULL,
				&mxi_fossil_rs232_function_list,
				&mxi_fossil_num_record_fields,
				&mxi_fossil_rfield_def_ptr},
#endif /* OS_MSDOS */

{"ks3344",         MXI_232_KS3344,   MXI_RS232,          MXR_INTERFACE,
				&mxi_ks3344_record_function_list,
				NULL,
				&mxi_ks3344_rs232_function_list,
				&mxi_ks3344_num_record_fields,
				&mxi_ks3344_rfield_def_ptr},

{"wago750_serial", MXI_232_WAGO750_SERIAL, MXI_RS232,    MXR_INTERFACE,
				&mxi_wago750_serial_record_function_list,
				NULL,
				&mxi_wago750_serial_rs232_function_list,
				&mxi_wago750_serial_num_record_fields,
				&mxi_wago750_serial_rfield_def_ptr},

#if HAVE_TCPIP

{"tcp232",         MXI_232_TCP232,   MXI_RS232,          MXR_INTERFACE,
				&mxi_tcp232_record_function_list,
				NULL,
				&mxi_tcp232_rs232_function_list,
				&mxi_tcp232_num_record_fields,
				&mxi_tcp232_rfield_def_ptr},

{"telnet",         MXI_232_TELNET,   MXI_RS232,          MXR_INTERFACE,
				&mxi_telnet_record_function_list,
				NULL,
				&mxi_telnet_rs232_function_list,
				&mxi_telnet_num_record_fields,
				&mxi_telnet_rfield_def_ptr},

#endif /* HAVE_TCPIP */

#if defined(OS_VMS)
{"vms_terminal",   MXI_232_VMS,      MXI_RS232,          MXR_INTERFACE,
				&mxi_vms_terminal_record_function_list,
				NULL,
				&mxi_vms_terminal_rs232_function_list,
				&mxi_vms_terminal_num_record_fields,
				&mxi_vms_terminal_rfield_def_ptr},
#endif /* OS_VMS */

#if defined(OS_VXWORKS)
{"vxworks_rs232",  MXI_232_VXWORKS,  MXI_RS232,          MXR_INTERFACE,
				&mxi_vxworks_rs232_record_function_list,
				NULL,
				&mxi_vxworks_rs232_rs232_function_list,
				&mxi_vxworks_rs232_num_record_fields,
				&mxi_vxworks_rs232_rfield_def_ptr},
#endif /* OS_VXWORKS */

{"camera_link_rs232", MXI_232_CAMERA_LINK, MXI_RS232,    MXR_INTERFACE,
				&mxi_camera_link_rs232_record_function_list,
				NULL,
				&mxi_camera_link_rs232_rs232_function_list,
				&mxi_camera_link_rs232_num_record_fields,
				&mxi_camera_link_rs232_rfield_def_ptr},

{"spec_command",   MXI_232_SPEC_COMMAND, MXI_RS232,      MXR_INTERFACE,
				&mxi_spec_command_record_function_list,
				NULL,
				&mxi_spec_command_rs232_function_list,
				&mxi_spec_command_num_record_fields,
				&mxi_spec_command_rfield_def_ptr},

{"sim900_port",   MXI_232_SIM900_PORT,   MXI_RS232,      MXR_INTERFACE,
				&mxi_sim900_port_record_function_list,
				NULL,
				&mxi_sim900_port_rs232_function_list,
				&mxi_sim900_port_num_record_fields,
				&mxi_sim900_port_rfield_def_ptr},

{"network_gpib",   MXI_GPIB_NETWORK, MXI_GPIB,         MXR_INTERFACE,
				&mxi_network_gpib_record_function_list,
				NULL,
				&mxi_network_gpib_gpib_function_list,
				&mxi_network_gpib_num_record_fields,
				&mxi_network_gpib_rfield_def_ptr},

{"k500serial",     MXI_GPIB_K500SERIAL, MXI_GPIB,         MXR_INTERFACE,
				&mxi_k500serial_record_function_list,
				NULL,
				&mxi_k500serial_gpib_function_list,
				&mxi_k500serial_num_record_fields,
				&mxi_k500serial_rfield_def_ptr},

{"micro488ex",     MXI_GPIB_MICRO488EX, MXI_GPIB,         MXR_INTERFACE,
				&mxi_micro488ex_record_function_list,
				NULL,
				&mxi_micro488ex_gpib_function_list,
				&mxi_micro488ex_num_record_fields,
				&mxi_micro488ex_rfield_def_ptr},

{"prologix",       MXI_GPIB_PROLOGIX,   MXI_GPIB,         MXR_INTERFACE,
				&mxi_prologix_record_function_list,
				NULL,
				&mxi_prologix_gpib_function_list,
				&mxi_prologix_num_record_fields,
				&mxi_prologix_rfield_def_ptr},

{"soft_camac",     MXI_CAM_SOFTWARE,  MXI_CAMAC,          MXR_INTERFACE,
				&mxi_scamac_record_function_list,
				NULL,
				&mxi_scamac_camac_function_list,
				&mxi_scamac_num_record_fields,
				&mxi_scamac_record_field_def_ptr},

{"dsp6001",        MXI_CAM_DSP6001,   MXI_CAMAC,          MXR_INTERFACE,
				&mxi_dsp6001_record_function_list,
				NULL,
				&mxi_dsp6001_camac_function_list,
				&mxi_dsp6001_num_record_fields,
				&mxi_dsp6001_rfield_def_ptr},

  /* === */

{"mm3000",         MXI_CTRL_MM3000,    MXI_CONTROLLER,        MXR_INTERFACE,
				&mxi_newport_record_function_list,
				NULL,
				NULL,
				&mxi_mm3000_num_record_fields,
				&mxi_mm3000_rfield_def_ptr},

{"mm4000",         MXI_CTRL_MM4000,    MXI_CONTROLLER,        MXR_INTERFACE,
				&mxi_newport_record_function_list,
				NULL,
				NULL,
				&mxi_mm4000_num_record_fields,
				&mxi_mm4000_rfield_def_ptr},

{"esp",            MXI_CTRL_ESP,       MXI_CONTROLLER,        MXR_INTERFACE,
				&mxi_newport_record_function_list,
				NULL,
				NULL,
				&mxi_esp_num_record_fields,
				&mxi_esp_rfield_def_ptr},

{"pmac",           MXI_CTRL_PMAC,      MXI_CONTROLLER,        MXR_INTERFACE,
                                &mxi_pmac_record_function_list,
                                NULL,
                                NULL,
				&mxi_pmac_num_record_fields,
				&mxi_pmac_rfield_def_ptr},

{"compumotor_int", MXI_CTRL_COMPUMOTOR, MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_compumotor_record_function_list,
				NULL,
				NULL,
				&mxi_compumotor_num_record_fields,
				&mxi_compumotor_rfield_def_ptr},

{"d8",             MXI_CTRL_D8,         MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_d8_record_function_list,
				NULL,
				&mxi_d8_generic_function_list,
				&mxi_d8_num_record_fields,
				&mxi_d8_rfield_def_ptr},

{"vp9000",         MXI_CTRL_VP9000,     MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_vp9000_record_function_list,
				NULL,
				&mxi_vp9000_generic_function_list,
				&mxi_vp9000_num_record_fields,
				&mxi_vp9000_rfield_def_ptr},

{"pdi40",          MXI_CTRL_PDI40,      MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_pdi40_record_function_list,
				NULL,
				&mxi_pdi40_generic_function_list,
				&mxi_pdi40_num_record_fields,
				&mxi_pdi40_rfield_def_ptr},

{"pdi45",          MXI_CTRL_PDI45,      MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_pdi45_record_function_list,
				NULL,
				NULL,
				&mxi_pdi45_num_record_fields,
				&mxi_pdi45_rfield_def_ptr},
{"i8255",          MXI_CTRL_8255,       MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_8255_record_function_list,
				NULL,
				&mxi_8255_generic_function_list,
				&mxi_8255_num_record_fields,
				&mxi_8255_rfield_def_ptr},

{"mc6821",         MXI_CTRL_6821,       MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_6821_record_function_list,
				NULL,
				&mxi_6821_generic_function_list,
				&mxi_6821_num_record_fields,
				&mxi_6821_rfield_def_ptr},

{"lpt",            MXI_CTRL_LPT,       MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_lpt_record_function_list,
				NULL,
				NULL,
				&mxi_lpt_num_record_fields,
				&mxi_lpt_rfield_def_ptr},

{"am9513",          MXI_CTRL_AM9513,       MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_am9513_record_function_list,
				NULL,
				&mxi_am9513_generic_function_list,
				&mxi_am9513_num_record_fields,
				&mxi_am9513_rfield_def_ptr},

{"hsc1",            MXI_CTRL_HSC1,         MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_hsc1_record_function_list,
				NULL,
				&mxi_hsc1_generic_function_list,
				&mxi_hsc1_num_record_fields,
				&mxi_hsc1_rfield_def_ptr},

{"vme58",           MXI_CTRL_VME58,        MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_vme58_record_function_list,
				NULL,
				NULL,
				&mxi_vme58_num_record_fields,
				&mxi_vme58_rfield_def_ptr},

{"vsc16",           MXI_CTRL_VSC16,        MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_vsc16_record_function_list,
				NULL,
				NULL,
				&mxi_vsc16_num_record_fields,
				&mxi_vsc16_rfield_def_ptr},

{"pcstep",          MXI_CTRL_PCSTEP,       MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_pcstep_record_function_list,
				NULL,
				&mxi_pcstep_generic_function_list,
				&mxi_pcstep_num_record_fields,
				&mxi_pcstep_rfield_def_ptr},

{"ortec974",       MXI_CTRL_ORTEC974,   MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_ortec974_record_function_list,
				NULL,
				&mxi_ortec974_generic_function_list,
				&mxi_ortec974_num_record_fields,
				&mxi_ortec974_rfield_def_ptr},

{"scipe_server",   MXI_CTRL_SCIPE,      MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_scipe_record_function_list,
				NULL,
				&mxi_scipe_generic_function_list,
				&mxi_scipe_num_record_fields,
				&mxi_scipe_rfield_def_ptr},

{"databox",        MXI_CTRL_DATABOX,    MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_databox_record_function_list,
				NULL,
				NULL,
				&mxi_databox_num_record_fields,
				&mxi_databox_rfield_def_ptr},

{"sis3807",        MXI_CTRL_SIS3807,    MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_sis3807_record_function_list,
				NULL,
				NULL,
				&mxi_sis3807_num_record_fields,
				&mxi_sis3807_rfield_def_ptr},

{"mardtb",         MXI_CTRL_MARDTB,     MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_mardtb_record_function_list,
				NULL,
				NULL,
				&mxi_mardtb_num_record_fields,
				&mxi_mardtb_rfield_def_ptr},

{"uglide",         MXI_CTRL_UGLIDE,     MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_uglide_record_function_list,
				NULL,
				NULL,
				&mxi_uglide_num_record_fields,
				&mxi_uglide_rfield_def_ptr},

{"aps_adcmod2",    MXI_CTRL_APS_ADCMOD2,    MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_aps_adcmod2_record_function_list,
				NULL,
				NULL,
				&mxi_aps_adcmod2_num_record_fields,
				&mxi_aps_adcmod2_rfield_def_ptr},

{"cxtilt02",       MXI_CTRL_CXTILT02,       MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_cxtilt02_record_function_list,
				NULL,
				NULL,
				&mxi_cxtilt02_num_record_fields,
				&mxi_cxtilt02_rfield_def_ptr},

{"iseries",  MXI_CTRL_ISERIES,  MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_iseries_record_function_list,
				NULL,
				NULL,
				&mxi_iseries_num_record_fields,
				&mxi_iseries_rfield_def_ptr},

{"phidget_old_stepper_controller",
		   MXI_CTRL_PHIDGET_OLD_STEPPER, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_phidget_old_stepper_record_function_list,
				NULL,
				NULL,
				&mxi_phidget_old_stepper_num_record_fields,
				&mxi_phidget_old_stepper_rfield_def_ptr},

{"pfcu",           MXI_CTRL_PFCU,  MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_pfcu_record_function_list,
				NULL,
				NULL,
				&mxi_pfcu_num_record_fields,
				&mxi_pfcu_rfield_def_ptr},

{"keithley2700",   MXI_CTRL_KEITHLEY2700, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_keithley2700_record_function_list,
				NULL,
				NULL,
				&mxi_keithley2700_num_record_fields,
				&mxi_keithley2700_rfield_def_ptr},

{"keithley2400",   MXI_CTRL_KEITHLEY2400, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_keithley2400_record_function_list,
				NULL,
				NULL,
				&mxi_keithley2400_num_record_fields,
				&mxi_keithley2400_rfield_def_ptr},

{"keithley2000",   MXI_CTRL_KEITHLEY2000, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_keithley2000_record_function_list,
				NULL,
				NULL,
				&mxi_keithley2000_num_record_fields,
				&mxi_keithley2000_rfield_def_ptr},

{"kohzu_sc",       MXI_CTRL_KOHZU_SC,      MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_kohzu_sc_record_function_list,
				NULL,
				NULL,
				&mxi_kohzu_sc_num_record_fields,
				&mxi_kohzu_sc_rfield_def_ptr},

{"picomotor_controller", MXI_CTRL_PICOMOTOR, MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_picomotor_record_function_list,
				NULL,
				NULL,
				&mxi_picomotor_num_record_fields,
				&mxi_picomotor_rfield_def_ptr},

{"sr630",          MXI_CTRL_SR630,         MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_sr630_record_function_list,
				NULL,
				NULL,
				&mxi_sr630_num_record_fields,
				&mxi_sr630_rfield_def_ptr},

{"tpg262",         MXI_CTRL_TPG262,        MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_tpg262_record_function_list,
				NULL,
				NULL,
				&mxi_tpg262_num_record_fields,
				&mxi_tpg262_rfield_def_ptr},

{"cm17a",          MXI_CTRL_CM17A,         MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_cm17a_record_function_list,
				NULL,
				NULL,
				&mxi_cm17a_num_record_fields,
				&mxi_cm17a_rfield_def_ptr},

{"sony_visca",     MXI_CTRL_SONY_VISCA,    MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_sony_visca_record_function_list,
				NULL,
				NULL,
				&mxi_sony_visca_num_record_fields,
				&mxi_sony_visca_rfield_def_ptr},

{"panasonic_kx_dp702", \
	MXI_CTRL_PANASONIC_KX_DP702,    MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_panasonic_kx_dp702_record_function_list,
				NULL,
				NULL,
				&mxi_panasonic_kx_dp702_num_record_fields,
				&mxi_panasonic_kx_dp702_rfield_def_ptr},

{"spellman_df3",  MXI_CTRL_SPELLMAN_DF3, MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_spellman_df3_record_function_list,
				NULL,
				NULL,
				&mxi_spellman_df3_num_record_fields,
				&mxi_spellman_df3_rfield_def_ptr},

{"bkprecision_912x", MXI_CTRL_BKPRECISION_912X, MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_bkprecision_912x_record_function_list,
				NULL,
				NULL,
				&mxi_bkprecision_912x_num_record_fields,
				&mxi_bkprecision_912x_rfield_def_ptr},

{"isobus",        MXI_CTRL_ISOBUS,        MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_isobus_record_function_list,
				NULL,
				NULL,
				&mxi_isobus_num_record_fields,
				&mxi_isobus_rfield_def_ptr},

{"ilm",           MXI_CTRL_ILM,           MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_ilm_record_function_list,
				NULL,
				NULL,
				&mxi_ilm_num_record_fields,
				&mxi_ilm_rfield_def_ptr},

{"itc503",        MXI_CTRL_ITC503,        MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_itc503_record_function_list,
				NULL,
				NULL,
				&mxi_itc503_num_record_fields,
				&mxi_itc503_rfield_def_ptr},

{"cryojet",       MXI_CTRL_CRYOJET,       MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_itc503_record_function_list,
				NULL,
				NULL,
				&mxi_itc503_num_record_fields,
				&mxi_itc503_rfield_def_ptr},

{"linkam_t9x",    MXI_CTRL_LINKAM_T9X,    MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_linkam_t9x_record_function_list,
				NULL,
				NULL,
				&mxi_linkam_t9x_num_record_fields,
				&mxi_linkam_t9x_rfield_def_ptr},

{"sim900",        MXI_CTRL_SIM900,        MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_sim900_record_function_list,
				NULL,
				NULL,
				&mxi_sim900_num_record_fields,
				&mxi_sim900_rfield_def_ptr},

{"i404",          MXI_CTRL_I404,          MXI_CONTROLLER,    MXR_INTERFACE,
				&mxi_i404_record_function_list,
				NULL,
				NULL,
				&mxi_i404_num_record_fields,
				&mxi_i404_rfield_def_ptr},

{"synaccess_netbooter",
		MXI_CTRL_SYNACCESS_NETBOOTER, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_synaccess_netbooter_record_function_list,
				NULL,
				NULL,
				&mxi_synaccess_netbooter_num_record_fields,
				&mxi_synaccess_netbooter_rfield_def_ptr},

#if ( defined(OS_LINUX) && (MX_GLIBC_VERSION >= 2002000L) )

{"linux_parport",  MXI_CTRL_LINUX_PARPORT, MXI_CONTROLLER,       MXR_INTERFACE,
				&mxi_linux_parport_record_function_list,
				NULL,
				NULL,
				&mxi_linux_parport_num_record_fields,
				&mxi_linux_parport_rfield_def_ptr},

#endif /* OS_LINUX */

#ifdef OS_LINUX

#if defined(__i386__) && defined(__GNUC__)

{"linux_iopl",     MXI_PIO_IOPL,      MXI_PORTIO,         MXR_INTERFACE,
				&mxi_linux_iopl_record_function_list,
				NULL,
				&mxi_linux_iopl_portio_function_list,
				&mxi_linux_iopl_num_record_fields,
				&mxi_linux_iopl_rfield_def_ptr},

{"linux_ioperm",   MXI_PIO_IOPERM,    MXI_PORTIO,         MXR_INTERFACE,
				&mxi_linux_iopl_record_function_list,
				NULL,
				&mxi_linux_iopl_portio_function_list,
				&mxi_linux_iopl_num_record_fields,
				&mxi_linux_iopl_rfield_def_ptr},

#endif /* __i386__ */

#endif /* OS_LINUX */

#if defined(OS_MSDOS) \
	|| ( defined(OS_WIN32) && !defined(__BORLANDC__) && !defined(__GNUC__) )
{"dos_portio",     MXI_PIO_DOS,       MXI_PORTIO,         MXR_INTERFACE,
				&mxi_dos_portio_record_function_list,
				NULL,
				&mxi_dos_portio_portio_function_list,
				&mxi_dos_portio_num_record_fields,
				&mxi_dos_portio_rfield_def_ptr},
#endif /* OS_MSDOS | OS_WIN32 */

#if defined(OS_VXWORKS)

{"vxworks_vme",    MXI_VME_VXWORKS,    MXI_VME,        MXR_INTERFACE,
				&mxi_vxworks_vme_record_function_list,
				NULL,
				&mxi_vxworks_vme_vme_function_list,
				&mxi_vxworks_vme_num_record_fields,
				&mxi_vxworks_vme_rfield_def_ptr},

#endif /* OS_VXWORKS */

#if defined(OS_LINUX)

{"mmap_vme",       MXI_VME_MMAP,       MXI_VME,        MXR_INTERFACE,
				&mxi_mmap_vme_record_function_list,
				NULL,
				&mxi_mmap_vme_vme_function_list,
				&mxi_mmap_vme_num_record_fields,
				&mxi_mmap_vme_rfield_def_ptr},

#endif /* OS_LINUX */

#if defined(OS_RTEMS) && defined(HAVE_RTEMS_VME)

{"rtems_vme",      MXI_VME_RTEMS,     MXI_VME,        MXR_INTERFACE,
				&mxi_rtems_vme_record_function_list,
				NULL,
				&mxi_rtems_vme_vme_function_list,
				&mxi_rtems_vme_num_record_fields,
				&mxi_rtems_vme_rfield_def_ptr},

#endif /* OS_RTEMS */

#if HAVE_TCPIP

{"modbus_tcp",     MXI_MOD_TCP,       MXI_MODBUS,     MXR_INTERFACE,
				&mxi_modbus_tcp_record_function_list,
				NULL,
				&mxi_modbus_tcp_modbus_function_list,
				&mxi_modbus_tcp_num_record_fields,
				&mxi_modbus_tcp_rfield_def_ptr},
#endif /* HAVE_TCPIP */

{"modbus_serial_rtu", MXI_MOD_SERIAL_RTU, MXI_MODBUS, MXR_INTERFACE,
				&mxi_modbus_serial_rtu_record_function_list,
				NULL,
				&mxi_modbus_serial_rtu_modbus_function_list,
				&mxi_modbus_serial_rtu_num_record_fields,
				&mxi_modbus_serial_rtu_rfield_def_ptr},

{"soft_camera_link", MXI_CL_SOFTWARE, MXI_CAMERA_LINK,  MXR_INTERFACE,
				&mxi_soft_camera_link_record_function_list,
				NULL,
				NULL,
				&mxi_soft_camera_link_num_record_fields,
				&mxi_soft_camera_link_rfield_def_ptr},

{"camera_link_api", MXI_CL_API,       MXI_CAMERA_LINK,  MXR_INTERFACE,
				&mxi_camera_link_api_record_function_list,
				NULL,
				NULL,
				&mxi_camera_link_api_num_record_fields,
				&mxi_camera_link_api_rfield_def_ptr},

  /* ==================== Device types ==================== */

{"soft_ainput",    MXT_AIN_SOFTWARE,   MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_soft_ainput_record_function_list,
				NULL,
				&mxd_soft_ainput_analog_input_function_list,
				&mxd_soft_ainput_num_record_fields,
				&mxd_soft_ainput_rfield_def_ptr},

{"soft_aoutput",   MXT_AOU_SOFTWARE,   MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_soft_aoutput_record_function_list,
				NULL,
				&mxd_soft_aoutput_analog_output_function_list,
				&mxd_soft_aoutput_num_record_fields,
				&mxd_soft_aoutput_rfield_def_ptr},

{"network_ainput", MXT_AIN_NETWORK,    MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_network_ainput_record_function_list,
				NULL,
				&mxd_network_ainput_analog_input_function_list,
				&mxd_network_ainput_num_record_fields,
				&mxd_network_ainput_rfield_def_ptr},

{"network_aoutput",MXT_AOU_NETWORK,    MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_network_aoutput_record_function_list,
				NULL,
			&mxd_network_aoutput_analog_output_function_list,
				&mxd_network_aoutput_num_record_fields,
				&mxd_network_aoutput_rfield_def_ptr},

{"ks3512",         MXT_AIN_KS3512,    MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_ks3512_record_function_list,
				NULL,
				&mxd_ks3512_analog_input_function_list,
				&mxd_ks3512_num_record_fields,
				&mxd_ks3512_rfield_def_ptr},

{"ks3112",         MXT_AOU_KS3112,    MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_ks3112_record_function_list,
				NULL,
				&mxd_ks3112_analog_output_function_list,
				&mxd_ks3112_num_record_fields,
				&mxd_ks3112_rfield_def_ptr},

{"pmac_ainput",    MXT_AIN_PMAC,      MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_pmac_ain_record_function_list,
				NULL,
				&mxd_pmac_ain_analog_input_function_list,
				&mxd_pmac_ain_num_record_fields,
				&mxd_pmac_ain_rfield_def_ptr},

{"pmac_aoutput",   MXT_AOU_PMAC,      MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_pmac_aout_record_function_list,
				NULL,
				&mxd_pmac_aout_analog_output_function_list,
				&mxd_pmac_aout_num_record_fields,
				&mxd_pmac_aout_rfield_def_ptr},

{"mclennan_ain",   MXT_AIN_MCLENNAN,  MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_mclennan_ain_record_function_list,
				NULL,
				&mxd_mclennan_ain_analog_input_function_list,
				&mxd_mclennan_ain_num_record_fields,
				&mxd_mclennan_ain_rfield_def_ptr},

{"mclennan_aout",  MXT_AOU_MCLENNAN,  MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_mclennan_aout_record_function_list,
				NULL,
				&mxd_mclennan_aout_analog_output_function_list,
				&mxd_mclennan_aout_num_record_fields,
				&mxd_mclennan_aout_rfield_def_ptr},

{"scipe_ain",      MXT_AIN_SCIPE,     MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_scipe_ain_record_function_list,
				NULL,
				&mxd_scipe_ain_analog_input_function_list,
				&mxd_scipe_ain_num_record_fields,
				&mxd_scipe_ain_rfield_def_ptr},

{"scipe_aout",     MXT_AOU_SCIPE,     MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_scipe_aout_record_function_list,
				NULL,
				&mxd_scipe_aout_analog_output_function_list,
				&mxd_scipe_aout_num_record_fields,
				&mxd_scipe_aout_rfield_def_ptr},

{"pdi45_ainput",   MXT_AIN_PDI45,     MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_pdi45_ain_record_function_list,
				NULL,
				&mxd_pdi45_ain_analog_input_function_list,
				&mxd_pdi45_ain_num_record_fields,
				&mxd_pdi45_ain_rfield_def_ptr},

{"pdi45_aoutput",  MXT_AOU_PDI45,     MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_pdi45_aout_record_function_list,
				NULL,
				&mxd_pdi45_aout_analog_output_function_list,
				&mxd_pdi45_aout_num_record_fields,
				&mxd_pdi45_aout_rfield_def_ptr},

{"mca_value",      MXT_AIN_MCA_VALUE, MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_mca_value_record_function_list,
				NULL,
				&mxd_mca_value_analog_input_function_list,
				&mxd_mca_value_num_record_fields,
				&mxd_mca_value_rfield_def_ptr},

{"mca_weighted_sum", MXT_AIN_MCA_WEIGHTED_SUM, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_mca_weighted_sum_record_function_list,
				NULL,
			&mxd_mca_weighted_sum_analog_input_function_list,
				&mxd_mca_weighted_sum_num_record_fields,
				&mxd_mca_weighted_sum_rfield_def_ptr},

{"mardtb_status",  MXT_AIN_MARDTB_STATUS, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_mardtb_status_record_function_list,
				NULL,
				&mxd_mardtb_status_analog_input_function_list,
				&mxd_mardtb_status_num_record_fields,
				&mxd_mardtb_status_rfield_def_ptr},

{"mdrive_ain",     MXT_AIN_MDRIVE,     MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_mdrive_ain_record_function_list,
				NULL,
				&mxd_mdrive_ain_analog_input_function_list,
				&mxd_mdrive_ain_num_record_fields,
				&mxd_mdrive_ain_rfield_def_ptr},

{"smartmotor_ain", MXT_AIN_SMARTMOTOR,   MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_smartmotor_ain_record_function_list,
				NULL,
			&mxd_smartmotor_ain_analog_input_function_list,
				&mxd_smartmotor_ain_num_record_fields,
				&mxd_smartmotor_ain_rfield_def_ptr},

{"smartmotor_aout",MXT_AOU_SMARTMOTOR,   MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_smartmotor_aout_record_function_list,
				NULL,
			&mxd_smartmotor_aout_analog_output_function_list,
				&mxd_smartmotor_aout_num_record_fields,
				&mxd_smartmotor_aout_rfield_def_ptr},

{"cxtilt02_angle", MXT_AIN_CXTILT02,     MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_cxtilt02_record_function_list,
				NULL,
				&mxd_cxtilt02_analog_input_function_list,
				&mxd_cxtilt02_num_record_fields,
				&mxd_cxtilt02_rfield_def_ptr},

{"modbus_ainput",  MXT_AIN_MODBUS,       MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_modbus_ain_record_function_list,
				NULL,
				&mxd_modbus_ain_analog_input_function_list,
				&mxd_modbus_ain_num_record_fields,
				&mxd_modbus_ain_rfield_def_ptr},

{"modbus_aoutput", MXT_AOU_MODBUS,       MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_modbus_aout_record_function_list,
				NULL,
				&mxd_modbus_aout_analog_output_function_list,
				&mxd_modbus_aout_num_record_fields,
				&mxd_modbus_aout_rfield_def_ptr},

{"wago750_modbus_aoutput", MXT_AOU_WAGO750_MODBUS, MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_wago750_modbus_aout_record_function_list,
				NULL,
			&mxd_wago750_modbus_aout_analog_output_function_list,
				&mxd_wago750_modbus_aout_num_record_fields,
				&mxd_wago750_modbus_aout_rfield_def_ptr},

{"iseries_ainput", MXT_AIN_ISERIES, MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_iseries_ain_record_function_list,
				NULL,
				&mxd_iseries_ain_analog_input_function_list,
				&mxd_iseries_ain_num_record_fields,
				&mxd_iseries_ain_rfield_def_ptr},

{"iseries_aoutput", MXT_AOU_ISERIES, MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_iseries_aout_record_function_list,
				NULL,
				&mxd_iseries_aout_analog_output_function_list,
				&mxd_iseries_aout_num_record_fields,
				&mxd_iseries_aout_rfield_def_ptr},

{"mcai_function", MXT_AIN_MCAI_FUNCTION, MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_mcai_function_record_function_list,
				NULL,
				&mxd_mcai_function_analog_input_function_list,
				&mxd_mcai_function_num_record_fields,
				&mxd_mcai_function_rfield_def_ptr},

{"keithley2700_ainput", MXT_AIN_KEITHLEY2700, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_keithley2700_ainput_record_function_list,
				NULL,
			    &mxd_keithley2700_ainput_analog_input_function_list,
				&mxd_keithley2700_ainput_num_record_fields,
				&mxd_keithley2700_ainput_rfield_def_ptr},

{"keithley2400_ainput", MXT_AIN_KEITHLEY2400, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_keithley2400_ainput_record_function_list,
				NULL,
			    &mxd_keithley2400_ainput_analog_input_function_list,
				&mxd_keithley2400_ainput_num_record_fields,
				&mxd_keithley2400_ainput_rfield_def_ptr},

{"keithley2400_aoutput", MXT_AOU_KEITHLEY2400, MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_keithley2400_aoutput_record_function_list,
				NULL,
		    &mxd_keithley2400_aoutput_analog_output_function_list,
				&mxd_keithley2400_aoutput_num_record_fields,
				&mxd_keithley2400_aoutput_rfield_def_ptr},

{"keithley2000_ainput", MXT_AIN_KEITHLEY2000, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_keithley2000_ainput_record_function_list,
				NULL,
			    &mxd_keithley2000_ainput_analog_input_function_list,
				&mxd_keithley2000_ainput_num_record_fields,
				&mxd_keithley2000_ainput_rfield_def_ptr},

{"picomotor_ainput",     MXT_AIN_PICOMOTOR, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_picomotor_ain_record_function_list,
				NULL,
				&mxd_picomotor_ain_analog_input_function_list,
				&mxd_picomotor_ain_num_record_fields,
				&mxd_picomotor_ain_rfield_def_ptr},

{"tracker_ainput", MXT_AIN_TRACKER, MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_tracker_ain_record_function_list,
				NULL,
				&mxd_tracker_ain_analog_input_function_list,
				&mxd_tracker_ain_num_record_fields,
				&mxd_tracker_ain_rfield_def_ptr},

{"tracker_aoutput", MXT_AOU_TRACKER, MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_tracker_aout_record_function_list,
				NULL,
				&mxd_tracker_aout_analog_output_function_list,
				&mxd_tracker_aout_num_record_fields,
				&mxd_tracker_aout_rfield_def_ptr},

{"sr630_ainput",   MXT_AIN_SR630, MXC_ANALOG_INPUT,  MXR_DEVICE,
				&mxd_sr630_ainput_record_function_list,
				NULL,
				&mxd_sr630_ainput_analog_input_function_list,
				&mxd_sr630_ainput_num_record_fields,
				&mxd_sr630_ainput_rfield_def_ptr},

{"sr630_aoutput",  MXT_AOU_SR630, MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_sr630_aoutput_record_function_list,
				NULL,
				&mxd_sr630_aoutput_analog_output_function_list,
				&mxd_sr630_aoutput_num_record_fields,
				&mxd_sr630_aoutput_rfield_def_ptr},

{"p6000a",         MXT_AIN_P6000A, MXC_ANALOG_INPUT,  MXR_DEVICE,
				&mxd_p6000a_record_function_list,
				NULL,
				&mxd_p6000a_analog_input_function_list,
				&mxd_p6000a_num_record_fields,
				&mxd_p6000a_rfield_def_ptr},

{"tpg262_pressure",MXT_AIN_TPG262, MXC_ANALOG_INPUT,  MXR_DEVICE,
				&mxd_tpg262_pressure_record_function_list,
				NULL,
				&mxd_tpg262_pressure_analog_input_function_list,
				&mxd_tpg262_pressure_num_record_fields,
				&mxd_tpg262_pressure_rfield_def_ptr},

{"bluice_dcss_ion_chamber",
		MXT_AIN_BLUICE_DCSS_ION_CHAMBER, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_bluice_ion_chamber_record_function_list,
				NULL,
			&mxd_bluice_ion_chamber_analog_input_function_list,
				&mxd_bluice_dcss_ion_chamber_num_record_fields,
				&mxd_bluice_dcss_ion_chamber_rfield_def_ptr},

{"bluice_dhs_ion_chamber",
		MXT_AIN_BLUICE_DHS_ION_CHAMBER, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_bluice_ion_chamber_record_function_list,
				NULL,
			&mxd_bluice_ion_chamber_analog_input_function_list,
				&mxd_bluice_dhs_ion_chamber_num_record_fields,
				&mxd_bluice_dhs_ion_chamber_rfield_def_ptr},

{"spellman_df3_ain", MXT_AIN_SPELLMAN_DF3,   MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_spellman_df3_ain_record_function_list,
				NULL,
			&mxd_spellman_df3_ain_analog_input_function_list,
				&mxd_spellman_df3_ain_num_record_fields,
				&mxd_spellman_df3_ain_rfield_def_ptr},

{"spellman_df3_aout",MXT_AOU_SPELLMAN_DF3,   MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_spellman_df3_aout_record_function_list,
				NULL,
			&mxd_spellman_df3_aout_analog_output_function_list,
				&mxd_spellman_df3_aout_num_record_fields,
				&mxd_spellman_df3_aout_rfield_def_ptr},

{"bkprecision_912x_ain", MXT_AIN_BKPRECISION_912X, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_bkprecision_912x_ain_record_function_list,
				NULL,
			&mxd_bkprecision_912x_ain_analog_input_function_list,
				&mxd_bkprecision_912x_ain_num_record_fields,
				&mxd_bkprecision_912x_ain_rfield_def_ptr},

{"bkprecision_912x_aout",MXT_AOU_BKPRECISION_912X, MXC_ANALOG_OUTPUT,MXR_DEVICE,
				&mxd_bkprecision_912x_aout_record_function_list,
				NULL,
			&mxd_bkprecision_912x_aout_analog_output_function_list,
				&mxd_bkprecision_912x_aout_num_record_fields,
				&mxd_bkprecision_912x_aout_rfield_def_ptr},

{"ilm_ainput",     MXT_AIN_ILM,        MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_ilm_ainput_record_function_list,
				NULL,
				&mxd_ilm_ainput_analog_input_function_list,
				&mxd_ilm_ainput_num_record_fields,
				&mxd_ilm_ainput_rfield_def_ptr},


{"soft_dinput",    MXT_DIN_SOFTWARE,   MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_soft_dinput_record_function_list,
				NULL,
				&mxd_soft_dinput_digital_input_function_list,
				&mxd_soft_dinput_num_record_fields,
				&mxd_soft_dinput_rfield_def_ptr},

{"soft_doutput",   MXT_DOU_SOFTWARE,   MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_soft_doutput_record_function_list,
				NULL,
				&mxd_soft_doutput_digital_output_function_list,
				&mxd_soft_doutput_num_record_fields,
				&mxd_soft_doutput_rfield_def_ptr},

{"network_dinput", MXT_DIN_NETWORK,    MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_network_dinput_record_function_list,
				NULL,
				&mxd_network_dinput_digital_input_function_list,
				&mxd_network_dinput_num_record_fields,
				&mxd_network_dinput_rfield_def_ptr},

{"network_doutput",MXT_DOU_NETWORK,    MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_network_doutput_record_function_list,
				NULL,
			&mxd_network_doutput_digital_output_function_list,
				&mxd_network_doutput_num_record_fields,
				&mxd_network_doutput_rfield_def_ptr},

{"ks3063_in",      MXT_DIN_KS3063,    MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_ks3063_in_record_function_list,
				NULL,
				&mxd_ks3063_in_digital_input_function_list,
				&mxd_ks3063_in_num_record_fields,
				&mxd_ks3063_in_rfield_def_ptr},

{"ks3063_out",     MXT_DOU_KS3063,    MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_ks3063_out_record_function_list,
				NULL,
				&mxd_ks3063_out_digital_output_function_list,
				&mxd_ks3063_out_num_record_fields,
				&mxd_ks3063_out_rfield_def_ptr},

{"i8255_in",       MXT_DIN_8255,      MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_8255_in_record_function_list,
				NULL,
				&mxd_8255_in_generic_function_list,
				&mxd_8255_in_num_record_fields,
				&mxd_8255_in_rfield_def_ptr},

{"i8255_out",      MXT_DOU_8255,      MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_8255_out_record_function_list,
				NULL,
				&mxd_8255_out_generic_function_list,
				&mxd_8255_out_num_record_fields,
				&mxd_8255_out_rfield_def_ptr},

{"mc6821_in",      MXT_DIN_6821,      MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_6821_in_record_function_list,
				NULL,
				&mxd_6821_in_digital_input_function_list,
				&mxd_6821_in_num_record_fields,
				&mxd_6821_in_rfield_def_ptr},

{"mc6821_out",     MXT_DOU_6821,      MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_6821_out_record_function_list,
				NULL,
				&mxd_6821_out_digital_output_function_list,
				&mxd_6821_out_num_record_fields,
				&mxd_6821_out_rfield_def_ptr},

{"lpt_in",         MXT_DIN_LPT,      MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_lpt_in_record_function_list,
				NULL,
				&mxd_lpt_in_digital_input_function_list,
				&mxd_lpt_in_num_record_fields,
				&mxd_lpt_in_rfield_def_ptr},

{"lpt_out",        MXT_DOU_LPT,      MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_lpt_out_record_function_list,
				NULL,
				&mxd_lpt_out_digital_output_function_list,
				&mxd_lpt_out_num_record_fields,
				&mxd_lpt_out_rfield_def_ptr},

{"bit_in",         MXT_DIN_BIT,       MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_bit_in_record_function_list,
				NULL,
				&mxd_bit_in_digital_input_function_list,
				&mxd_bit_in_num_record_fields,
				&mxd_bit_in_rfield_def_ptr},

{"bit_out",        MXT_DOU_BIT,       MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_bit_out_record_function_list,
				NULL,
				&mxd_bit_out_digital_output_function_list,
				&mxd_bit_out_num_record_fields,
				&mxd_bit_out_rfield_def_ptr},

{"vme_dinput",     MXT_DIN_VME,       MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_vme_din_record_function_list,
				NULL,
				&mxd_vme_din_digital_input_function_list,
				&mxd_vme_din_num_record_fields,
				&mxd_vme_din_rfield_def_ptr},

{"vme_doutput",    MXT_DOU_VME,       MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_vme_dout_record_function_list,
				NULL,
				&mxd_vme_dout_digital_output_function_list,
				&mxd_vme_dout_num_record_fields,
				&mxd_vme_dout_rfield_def_ptr},

{"portio_dinput",  MXT_DIN_PORTIO,    MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_portio_din_record_function_list,
				NULL,
				&mxd_portio_din_digital_input_function_list,
				&mxd_portio_din_num_record_fields,
				&mxd_portio_din_rfield_def_ptr},

{"portio_doutput", MXT_DOU_PORTIO,    MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_portio_dout_record_function_list,
				NULL,
				&mxd_portio_dout_digital_output_function_list,
				&mxd_portio_dout_num_record_fields,
				&mxd_portio_dout_rfield_def_ptr},

{"pmac_dinput",    MXT_DIN_PMAC,      MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_pmac_din_record_function_list,
				NULL,
				&mxd_pmac_din_digital_input_function_list,
				&mxd_pmac_din_num_record_fields,
				&mxd_pmac_din_rfield_def_ptr},

{"pmac_doutput",   MXT_DOU_PMAC,      MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_pmac_dout_record_function_list,
				NULL,
				&mxd_pmac_dout_digital_output_function_list,
				&mxd_pmac_dout_num_record_fields,
				&mxd_pmac_dout_rfield_def_ptr},

{"compumotor_din", MXT_DIN_COMPUMOTOR,MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_compumotor_din_record_function_list,
				NULL,
				&mxd_compumotor_din_digital_input_function_list,
				&mxd_compumotor_din_num_record_fields,
				&mxd_compumotor_din_rfield_def_ptr},

{"compumotor_dout",MXT_DOU_COMPUMOTOR,MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_compumotor_dout_record_function_list,
				NULL,
				&mxd_compumotor_dout_digital_output_function_list,
				&mxd_compumotor_dout_num_record_fields,
				&mxd_compumotor_dout_rfield_def_ptr},

{"mclennan_din",   MXT_DIN_MCLENNAN,  MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_mclennan_din_record_function_list,
				NULL,
				&mxd_mclennan_din_digital_input_function_list,
				&mxd_mclennan_din_num_record_fields,
				&mxd_mclennan_din_rfield_def_ptr},

{"mclennan_dout",  MXT_DOU_MCLENNAN,  MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_mclennan_dout_record_function_list,
				NULL,
				&mxd_mclennan_dout_digital_output_function_list,
				&mxd_mclennan_dout_num_record_fields,
				&mxd_mclennan_dout_rfield_def_ptr},

{"scipe_din",      MXT_DIN_SCIPE,     MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_scipe_din_record_function_list,
				NULL,
				&mxd_scipe_din_digital_input_function_list,
				&mxd_scipe_din_num_record_fields,
				&mxd_scipe_din_rfield_def_ptr},

{"scipe_dout",     MXT_DOU_SCIPE,     MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_scipe_dout_record_function_list,
				NULL,
				&mxd_scipe_dout_digital_output_function_list,
				&mxd_scipe_dout_num_record_fields,
				&mxd_scipe_dout_rfield_def_ptr},

{"mdrive_din",     MXT_DIN_MDRIVE,    MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_mdrive_din_record_function_list,
				NULL,
				&mxd_mdrive_din_digital_input_function_list,
				&mxd_mdrive_din_num_record_fields,
				&mxd_mdrive_din_rfield_def_ptr},

{"mdrive_dout",    MXT_DOU_MDRIVE,    MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_mdrive_dout_record_function_list,
				NULL,
				&mxd_mdrive_dout_digital_output_function_list,
				&mxd_mdrive_dout_num_record_fields,
				&mxd_mdrive_dout_rfield_def_ptr},

{"smartmotor_din", MXT_DIN_SMARTMOTOR,MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_smartmotor_din_record_function_list,
				NULL,
			&mxd_smartmotor_din_digital_input_function_list,
				&mxd_smartmotor_din_num_record_fields,
				&mxd_smartmotor_din_rfield_def_ptr},

{"smartmotor_dout",MXT_DOU_SMARTMOTOR,MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_smartmotor_dout_record_function_list,
				NULL,
			&mxd_smartmotor_dout_digital_output_function_list,
				&mxd_smartmotor_dout_num_record_fields,
				&mxd_smartmotor_dout_rfield_def_ptr},

{"pdi45_dinput",   MXT_DIN_PDI45,     MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_pdi45_din_record_function_list,
				NULL,
				&mxd_pdi45_din_digital_input_function_list,
				&mxd_pdi45_din_num_record_fields,
				&mxd_pdi45_din_rfield_def_ptr},

{"pdi45_doutput",  MXT_DOU_PDI45,     MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_pdi45_dout_record_function_list,
				NULL,
				&mxd_pdi45_dout_digital_output_function_list,
				&mxd_pdi45_dout_num_record_fields,
				&mxd_pdi45_dout_rfield_def_ptr},

{"modbus_dinput",  MXT_DIN_MODBUS,    MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_modbus_din_record_function_list,
				NULL,
				&mxd_modbus_din_digital_input_function_list,
				&mxd_modbus_din_num_record_fields,
				&mxd_modbus_din_rfield_def_ptr},

{"modbus_doutput", MXT_DOU_MODBUS,    MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_modbus_dout_record_function_list,
				NULL,
				&mxd_modbus_dout_digital_output_function_list,
				&mxd_modbus_dout_num_record_fields,
				&mxd_modbus_dout_rfield_def_ptr},

{"wago750_modbus_doutput", MXT_DOU_WAGO750_MODBUS,
					MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_wago750_modbus_dout_record_function_list,
				NULL,
			&mxd_wago750_modbus_dout_digital_output_function_list,
				&mxd_wago750_modbus_dout_num_record_fields,
				&mxd_wago750_modbus_dout_rfield_def_ptr},

{"iseries_dinput", MXT_DIN_ISERIES, MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_iseries_din_record_function_list,
				NULL,
				&mxd_iseries_din_digital_input_function_list,
				&mxd_iseries_din_num_record_fields,
				&mxd_iseries_din_rfield_def_ptr},

{"iseries_doutput", MXT_DOU_ISERIES, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_iseries_dout_record_function_list,
				NULL,
				&mxd_iseries_dout_digital_output_function_list,
				&mxd_iseries_dout_num_record_fields,
				&mxd_iseries_dout_rfield_def_ptr},

{"keithley2400_doutput", MXT_DOU_KEITHLEY2400, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_keithley2400_doutput_record_function_list,
				NULL,
			&mxd_keithley2400_doutput_digital_output_function_list,
				&mxd_keithley2400_doutput_num_record_fields,
				&mxd_keithley2400_doutput_rfield_def_ptr},

{"pfcu_filter_summary", MXT_DOU_PFCU_FILTER_SUMMARY,
					MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_pfcu_filter_summary_record_function_list,
				NULL,
			&mxd_pfcu_filter_summary_digital_output_function_list,
				&mxd_pfcu_filter_summary_num_record_fields,
				&mxd_pfcu_filter_summary_rfield_def_ptr},

{"picomotor_dinput", MXT_DIN_PICOMOTOR, MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_picomotor_din_record_function_list,
				NULL,
				&mxd_picomotor_din_digital_input_function_list,
				&mxd_picomotor_din_num_record_fields,
				&mxd_picomotor_din_rfield_def_ptr},

{"picomotor_doutput", MXT_DOU_PICOMOTOR, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_picomotor_dout_record_function_list,
				NULL,
			      &mxd_picomotor_dout_digital_output_function_list,
				&mxd_picomotor_dout_num_record_fields,
				&mxd_picomotor_dout_rfield_def_ptr},

{"tracker_dinput", MXT_DIN_TRACKER, MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_tracker_din_record_function_list,
				NULL,
				&mxd_tracker_din_digital_input_function_list,
				&mxd_tracker_din_num_record_fields,
				&mxd_tracker_din_rfield_def_ptr},

{"tracker_doutput", MXT_DOU_TRACKER, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_tracker_dout_record_function_list,
				NULL,
				&mxd_tracker_dout_digital_output_function_list,
				&mxd_tracker_dout_num_record_fields,
				&mxd_tracker_dout_rfield_def_ptr},

{"cm17a_doutput",   MXT_DOU_CM17A,   MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_cm17a_doutput_record_function_list,
				NULL,
				&mxd_cm17a_doutput_digital_output_function_list,
				&mxd_cm17a_doutput_num_record_fields,
				&mxd_cm17a_doutput_rfield_def_ptr},

{"spellman_df3_din", MXT_DIN_SPELLMAN_DF3,   MXC_DIGITAL_INPUT,   MXR_DEVICE,
				&mxd_spellman_df3_din_record_function_list,
				NULL,
			&mxd_spellman_df3_din_digital_input_function_list,
				&mxd_spellman_df3_din_num_record_fields,
				&mxd_spellman_df3_din_rfield_def_ptr},

{"spellman_df3_dout",MXT_DOU_SPELLMAN_DF3,   MXC_DIGITAL_OUTPUT,  MXR_DEVICE,
				&mxd_spellman_df3_dout_record_function_list,
				NULL,
			&mxd_spellman_df3_dout_digital_output_function_list,
				&mxd_spellman_df3_dout_num_record_fields,
				&mxd_spellman_df3_dout_rfield_def_ptr},

{"bkprecision_912x_din", MXT_DIN_BKPRECISION_912X,MXC_DIGITAL_INPUT, MXR_DEVICE,
				&mxd_bkprecision_912x_din_record_function_list,
				NULL,
			&mxd_bkprecision_912x_din_digital_input_function_list,
				&mxd_bkprecision_912x_din_num_record_fields,
				&mxd_bkprecision_912x_din_rfield_def_ptr},

{"bkprecision_912x_dout",MXT_DOU_BKPRECISION_912X,MXC_DIGITAL_OUTPUT,MXR_DEVICE,
				&mxd_bkprecision_912x_dout_record_function_list,
				NULL,
			&mxd_bkprecision_912x_dout_digital_output_function_list,
				&mxd_bkprecision_912x_dout_num_record_fields,
				&mxd_bkprecision_912x_dout_rfield_def_ptr},

{"ilm_status",       MXT_DIN_ILM, MXC_DIGITAL_INPUT,   MXR_DEVICE,
				&mxd_ilm_status_record_function_list,
				NULL,
				&mxd_ilm_status_digital_input_function_list,
				&mxd_ilm_status_num_record_fields,
				&mxd_ilm_status_rfield_def_ptr},

{"ilm_sample_rate",  MXT_DOU_ILM_SAMPLE_RATE, MXC_DIGITAL_OUTPUT,   MXR_DEVICE,
				&mxd_ilm_sample_rate_record_function_list,
				NULL,
			&mxd_ilm_sample_rate_digital_output_function_list,
				&mxd_ilm_sample_rate_num_record_fields,
				&mxd_ilm_sample_rate_rfield_def_ptr},

{"systron_donner_m107", MXT_AOU_SYSTRON_DONNER_M107,
					MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_sd_m107_record_function_list,
				NULL,
				&mxd_sd_m107_analog_output_function_list,
				&mxd_sd_m107_num_record_fields,
				&mxd_sd_m107_rfield_def_ptr},

{"linkam_t9x_pump", MXT_AOU_LINKAM_T9X_PUMP, MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_linkam_t9x_pump_record_function_list,
				NULL,
			      &mxd_linkam_t9x_pump_analog_output_function_list,
				&mxd_linkam_t9x_pump_num_record_fields,
				&mxd_linkam_t9x_pump_rfield_def_ptr},

{"sim980",          MXT_AIN_SIM980, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_sim980_record_function_list,
				NULL,
				&mxd_sim980_analog_input_function_list,
				&mxd_sim980_num_record_fields,
				&mxd_sim980_rfield_def_ptr},

{"synaccess_netbooter_ainput", \
		MXT_AIN_SYNACCESS_NETBOOTER, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_sa_netbooter_ainput_record_function_list,
				NULL,
			    &mxd_sa_netbooter_ainput_analog_input_function_list,
				&mxd_sa_netbooter_ainput_num_record_fields,
				&mxd_sa_netbooter_ainput_rfield_def_ptr},

{"keyboard_dinput",  MXT_DIN_KEYBOARD,   MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_keyboard_dinput_record_function_list,
				NULL,
			&mxd_keyboard_dinput_digital_input_function_list,
				&mxd_keyboard_dinput_num_record_fields,
				&mxd_keyboard_dinput_rfield_def_ptr},

{"file_dinput",      MXT_DIN_FILE,   MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_file_dinput_record_function_list,
				NULL,
				&mxd_file_dinput_digital_input_function_list,
				&mxd_file_dinput_num_record_fields,
				&mxd_file_dinput_rfield_def_ptr},

{"ainput_as_dinput", MXT_DIN_AINPUT,   MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_ainput_as_dinput_record_function_list,
				NULL,
			&mxd_ainput_as_dinput_digital_input_function_list,
				&mxd_ainput_as_dinput_num_record_fields,
				&mxd_ainput_as_dinput_rfield_def_ptr},

{"aoutput_as_doutput", MXT_DOU_AOUTPUT, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_aoutput_as_doutput_record_function_list,
				NULL,
			&mxd_aoutput_as_doutput_digital_output_function_list,
				&mxd_aoutput_as_doutput_num_record_fields,
				&mxd_aoutput_as_doutput_rfield_def_ptr},

{"relay_as_doutput", MXT_DOU_RELAY, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_relay_as_doutput_record_function_list,
				NULL,
			&mxd_relay_as_doutput_digital_output_function_list,
				&mxd_relay_as_doutput_num_record_fields,
				&mxd_relay_as_doutput_rfield_def_ptr},

{"digital_fanout", MXT_DOU_FANOUT, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_digital_fanout_record_function_list,
				NULL,
			&mxd_digital_fanout_digital_output_function_list,
				&mxd_digital_fanout_num_record_fields,
				&mxd_digital_fanout_rfield_def_ptr},

#if ( defined(OS_LINUX) && (MX_GLIBC_VERSION >= 2002000L) )

{"linux_parport_in", MXT_DIN_LINUX_PARPORT, MXC_DIGITAL_INPUT, MXR_DEVICE,
				&mxd_linux_parport_in_record_function_list,
				NULL,
			&mxd_linux_parport_in_digital_input_function_list,
				&mxd_linux_parport_in_num_record_fields,
				&mxd_linux_parport_in_rfield_def_ptr},

{"linux_parport_out", MXT_DOU_LINUX_PARPORT, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_linux_parport_out_record_function_list,
				NULL,
			&mxd_linux_parport_out_digital_output_function_list,
				&mxd_linux_parport_out_num_record_fields,
				&mxd_linux_parport_out_rfield_def_ptr},

#endif /* OS_LINUX */


{"soft_motor",     MXT_MTR_SOFTWARE,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_soft_motor_record_function_list,
				NULL,
				&mxd_soft_motor_motor_function_list,
				&mxd_soft_motor_num_record_fields,
				&mxd_soft_motor_rfield_def_ptr},
{"e500",           MXT_MTR_E500,      MXC_MOTOR,          MXR_DEVICE,
				&mxd_e500_record_function_list,
				NULL,
				&mxd_e500_motor_function_list,
				&mxd_e500_num_record_fields,
				&mxd_e500_rfield_def_ptr},
{"smc24",          MXT_MTR_SMC24,     MXC_MOTOR,          MXR_DEVICE,
				&mxd_smc24_record_function_list,
				NULL,
				&mxd_smc24_motor_function_list,
				&mxd_smc24_num_record_fields,
				&mxd_smc24_rfield_def_ptr},
{"panther_hi",     MXT_MTR_PANTHER_HI, MXC_MOTOR,          MXR_DEVICE,
				&mxd_panther_record_function_list,
				NULL,
				&mxd_panther_motor_function_list,
				&mxd_panther_hi_num_record_fields,
				&mxd_panther_hi_rfield_def_ptr},
{"panther_he",     MXT_MTR_PANTHER_HE, MXC_MOTOR,          MXR_DEVICE,
				&mxd_panther_record_function_list,
				NULL,
				&mxd_panther_motor_function_list,
				&mxd_panther_he_num_record_fields,
				&mxd_panther_he_rfield_def_ptr},
{"pdi40_motor",    MXT_MTR_PDI40,     MXC_MOTOR,          MXR_DEVICE,
				&mxd_pdi40motor_record_function_list,
				NULL,
				&mxd_pdi40motor_motor_function_list,
				&mxd_pdi40motor_num_record_fields,
				&mxd_pdi40motor_rfield_def_ptr},
{"mmc32",          MXT_MTR_MMC32,     MXC_MOTOR,          MXR_DEVICE,
				&mxd_mmc32_record_function_list,
				NULL,
				&mxd_mmc32_motor_function_list,
				&mxd_mmc32_num_record_fields,
				&mxd_mmc32_rfield_def_ptr},
{"pm304",          MXT_MTR_PM304,     MXC_MOTOR,          MXR_DEVICE,
				&mxd_pm304_record_function_list,
				NULL,
				&mxd_pm304_motor_function_list,
				&mxd_pm304_num_record_fields,
				&mxd_pm304_rfield_def_ptr},
{"mclennan",       MXT_MTR_MCLENNAN,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_mclennan_record_function_list,
				NULL,
				&mxd_mclennan_motor_function_list,
				&mxd_mclennan_num_record_fields,
				&mxd_mclennan_rfield_def_ptr},
{"mdrive",         MXT_MTR_MDRIVE,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_mdrive_record_function_list,
				NULL,
				&mxd_mdrive_motor_function_list,
				&mxd_mdrive_num_record_fields,
				&mxd_mdrive_rfield_def_ptr},
{"smartmotor",     MXT_MTR_SMARTMOTOR,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_smartmotor_record_function_list,
				NULL,
				&mxd_smartmotor_motor_function_list,
				&mxd_smartmotor_num_record_fields,
				&mxd_smartmotor_rfield_def_ptr},
{"pmac_motor",     MXT_MTR_PMAC,      MXC_MOTOR,          MXR_DEVICE,
                                &mxd_pmac_record_function_list,
                                NULL,
                                &mxd_pmac_motor_function_list,
				&mxd_pmac_num_record_fields,
				&mxd_pmac_rfield_def_ptr},
{"pmac_cs_axis",   MXT_MTR_PMAC_COORDINATE_SYSTEM,
				      MXC_MOTOR,          MXR_DEVICE,
                                &mxd_pmac_cs_axis_record_function_list,
                                NULL,
                                &mxd_pmac_cs_axis_motor_function_list,
				&mxd_pmac_cs_axis_num_record_fields,
				&mxd_pmac_cs_axis_rfield_def_ptr},
{"compumotor",     MXT_MTR_COMPUMOTOR, MXC_MOTOR,         MXR_DEVICE,
				&mxd_compumotor_record_function_list,
				NULL,
				&mxd_compumotor_motor_function_list,
				&mxd_compumotor_num_record_fields,
				&mxd_compumotor_rfield_def_ptr},
{"network_motor",  MXT_MTR_NETWORK,   MXC_MOTOR,          MXR_DEVICE,
				&mxd_network_motor_record_function_list,
				NULL,
				&mxd_network_motor_motor_function_list,
				&mxd_network_motor_num_record_fields,
				&mxd_network_motor_rfield_def_ptr},

{"d8_motor",       MXT_MTR_D8,        MXC_MOTOR,          MXR_DEVICE,
				&mxd_d8_motor_record_function_list,
				NULL,
				&mxd_d8_motor_motor_function_list,
				&mxd_d8_motor_num_record_fields,
				&mxd_d8_motor_rfield_def_ptr},

{"vp9000_motor",   MXT_MTR_VP9000,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_vp9000_record_function_list,
				NULL,
				&mxd_vp9000_motor_function_list,
				&mxd_vp9000_num_record_fields,
				&mxd_vp9000_rfield_def_ptr},

{"e662",           MXT_MTR_E662,      MXC_MOTOR,          MXR_DEVICE,
				&mxd_e662_record_function_list,
				NULL,
				&mxd_e662_motor_function_list,
				&mxd_e662_num_record_fields,
				&mxd_e662_rfield_def_ptr},

{"hsc1_motor",     MXT_MTR_HSC1,      MXC_MOTOR,          MXR_DEVICE,
				&mxd_hsc1_record_function_list,
				NULL,
				&mxd_hsc1_motor_function_list,
				&mxd_hsc1_num_record_fields,
				&mxd_hsc1_rfield_def_ptr},

{"stp100_motor",   MXT_MTR_STP100,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_stp100_motor_record_function_list,
				NULL,
				&mxd_stp100_motor_motor_function_list,
				&mxd_stp100_motor_num_record_fields,
				&mxd_stp100_motor_rfield_def_ptr},

{"vme58_motor",    MXT_MTR_VME58,     MXC_MOTOR,          MXR_DEVICE,
				&mxd_vme58_record_function_list,
				NULL,
				&mxd_vme58_motor_function_list,
				&mxd_vme58_num_record_fields,
				&mxd_vme58_rfield_def_ptr},

{"pcstep_motor",   MXT_MTR_PCSTEP,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_pcstep_record_function_list,
				NULL,
				&mxd_pcstep_motor_function_list,
				&mxd_pcstep_num_record_fields,
				&mxd_pcstep_rfield_def_ptr},

{"scipe_motor",    MXT_MTR_SCIPE,     MXC_MOTOR,          MXR_DEVICE,
				&mxd_scipe_motor_record_function_list,
				NULL,
				&mxd_scipe_motor_motor_function_list,
				&mxd_scipe_motor_num_record_fields,
				&mxd_scipe_motor_rfield_def_ptr},

/* The Newport MM3000 and MM4000 share a driver. */

{"mm3000_motor",   MXT_MTR_MM3000,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_newport_record_function_list,
				NULL,
				&mxd_newport_motor_function_list,
				&mxd_mm3000_num_record_fields,
				&mxd_mm3000_rfield_def_ptr},
{"mm4000_motor",   MXT_MTR_MM4000,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_newport_record_function_list,
				NULL,
				&mxd_newport_motor_function_list,
				&mxd_mm4000_num_record_fields,
				&mxd_mm4000_rfield_def_ptr},
{"esp_motor",      MXT_MTR_ESP,       MXC_MOTOR,          MXR_DEVICE,
				&mxd_newport_record_function_list,
				NULL,
				&mxd_newport_motor_function_list,
				&mxd_esp_num_record_fields,
				&mxd_esp_rfield_def_ptr},


{"dac_motor",      MXT_MTR_DAC_MOTOR, MXC_MOTOR,          MXR_DEVICE,
				&mxd_dac_motor_record_function_list,
				NULL,
				&mxd_dac_motor_motor_function_list,
				&mxd_dac_motor_num_record_fields,
				&mxd_dac_motor_rfield_def_ptr},
{"ls330_motor",    MXT_MTR_LAKESHORE330, MXC_MOTOR,       MXR_DEVICE,
				&mxd_ls330_motor_record_function_list,
				NULL,
				&mxd_ls330_motor_motor_function_list,
				&mxd_ls330_motor_num_record_fields,
				&mxd_ls330_motor_rfield_def_ptr},

{"databox_motor",  MXT_MTR_DATABOX,   MXC_MOTOR,       MXR_DEVICE,
				&mxd_databox_motor_record_function_list,
				NULL,
				&mxd_databox_motor_motor_function_list,
				&mxd_databox_motor_num_record_fields,
				&mxd_databox_motor_rfield_def_ptr},

{"disabled_motor", MXT_MTR_DISABLED,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_disabled_motor_record_function_list,
				NULL,
				&mxd_disabled_motor_motor_function_list,
				&mxd_disabled_motor_num_record_fields,
				&mxd_disabled_motor_rfield_def_ptr},

{"mardtb_motor",   MXT_MTR_MARDTB,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_mardtb_motor_record_function_list,
				NULL,
				&mxd_mardtb_motor_motor_function_list,
				&mxd_mardtb_motor_num_record_fields,
				&mxd_mardtb_motor_rfield_def_ptr},

{"uglide_motor",   MXT_MTR_UGLIDE,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_uglide_record_function_list,
				NULL,
				&mxd_uglide_motor_function_list,
				&mxd_uglide_num_record_fields,
				&mxd_uglide_rfield_def_ptr},

{"phidget_old_stepper", MXT_MTR_PHIDGET_OLD_STEPPER, MXC_MOTOR, MXR_DEVICE,
				&mxd_phidget_old_stepper_record_function_list,
				NULL,
				&mxd_phidget_old_stepper_motor_function_list,
				&mxd_phidget_old_stepper_num_record_fields,
				&mxd_phidget_old_stepper_rfield_def_ptr},

{"spec_motor",     MXT_MTR_SPEC,      MXC_MOTOR,          MXR_DEVICE,
				&mxd_spec_motor_record_function_list,
				NULL,
				&mxd_spec_motor_motor_function_list,
				&mxd_spec_motor_num_record_fields,
				&mxd_spec_motor_rfield_def_ptr},

{"kohzu_sc_motor", MXT_MTR_KOHZU_SC,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_kohzu_sc_record_function_list,
				NULL,
				&mxd_kohzu_sc_motor_function_list,
				&mxd_kohzu_sc_num_record_fields,
				&mxd_kohzu_sc_rfield_def_ptr},

{"picomotor", MXT_MTR_PICOMOTOR,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_picomotor_record_function_list,
				NULL,
				&mxd_picomotor_motor_function_list,
				&mxd_picomotor_num_record_fields,
				&mxd_picomotor_rfield_def_ptr},

{"mcu2",           MXT_MTR_MCU2,      MXC_MOTOR,          MXR_DEVICE,
				&mxd_mcu2_record_function_list,
				NULL,
				&mxd_mcu2_motor_function_list,
				&mxd_mcu2_num_record_fields,
				&mxd_mcu2_rfield_def_ptr},

{"bluice_dcss_motor",  MXT_MTR_BLUICE_DCSS, MXC_MOTOR,    MXR_DEVICE,
				&mxd_bluice_motor_record_function_list,
				NULL,
				&mxd_bluice_motor_motor_function_list,
				&mxd_bluice_dcss_motor_num_record_fields,
				&mxd_bluice_dcss_motor_rfield_def_ptr},

{"bluice_dhs_motor",  MXT_MTR_BLUICE_DHS,   MXC_MOTOR,    MXR_DEVICE,
				&mxd_bluice_motor_record_function_list,
				NULL,
				&mxd_bluice_motor_motor_function_list,
				&mxd_bluice_dhs_motor_num_record_fields,
				&mxd_bluice_dhs_motor_rfield_def_ptr},

{"ptz_motor",      MXT_MTR_PTZ,       MXC_MOTOR,          MXR_DEVICE,
				&mxd_ptz_motor_record_function_list,
				NULL,
				&mxd_ptz_motor_motor_function_list,
				&mxd_ptz_motor_num_record_fields,
				&mxd_ptz_motor_rfield_def_ptr},

{"src_mono",       MXT_MTR_SRC_MONO,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_src_mono_record_function_list,
				NULL,
				&mxd_src_mono_motor_function_list,
				&mxd_src_mono_num_record_fields,
				&mxd_src_mono_rfield_def_ptr},

{"linkam_t9x_motor", MXT_MTR_LINKAM_T9X_MOTOR,  MXC_MOTOR,  MXR_DEVICE,
				&mxd_linkam_t9x_motor_record_function_list,
				NULL,
				&mxd_linkam_t9x_motor_motor_function_list,
				&mxd_linkam_t9x_motor_num_record_fields,
				&mxd_linkam_t9x_motor_rfield_def_ptr},

{"linkam_t9x_temp", MXT_MTR_LINKAM_T9X_TEMPERATURE,  MXC_MOTOR,  MXR_DEVICE,
				&mxd_linkam_t9x_temp_record_function_list,
				NULL,
				&mxd_linkam_t9x_temp_motor_function_list,
				&mxd_linkam_t9x_temp_num_record_fields,
				&mxd_linkam_t9x_temp_rfield_def_ptr},

{"sim960",          MXT_MTR_SIM960,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_sim960_record_function_list,
				NULL,
				&mxd_sim960_motor_function_list,
				&mxd_sim960_num_record_fields,
				&mxd_sim960_rfield_def_ptr},

/* Pseudo motors. */

{"energy_motor",   MXT_MTR_ENERGY,    MXC_MOTOR,          MXR_DEVICE,
				&mxd_energy_motor_record_function_list,
				NULL,
				&mxd_energy_motor_motor_function_list,
				&mxd_energy_motor_num_record_fields,
				&mxd_energy_motor_rfield_def_ptr},
{"wavelength_motor", MXT_MTR_WAVELENGTH, MXC_MOTOR,       MXR_DEVICE,
				&mxd_wavelength_motor_record_function_list,
				NULL,
				&mxd_wavelength_motor_motor_function_list,
				&mxd_wavelength_motor_num_record_fields,
				&mxd_wavelength_motor_rfield_def_ptr},
{"wavenumber_motor", MXT_MTR_WAVENUMBER, MXC_MOTOR,       MXR_DEVICE,
				&mxd_wavenumber_motor_record_function_list,
				NULL,
				&mxd_wavenumber_motor_motor_function_list,
				&mxd_wavenumber_motor_num_record_fields,
				&mxd_wavenumber_motor_rfield_def_ptr},
{"slit_motor",     MXT_MTR_SLIT_MOTOR, MXC_MOTOR,         MXR_DEVICE,
				&mxd_slit_motor_record_function_list,
				NULL,
				&mxd_slit_motor_motor_function_list,
				&mxd_slit_motor_num_record_fields,
				&mxd_slit_motor_rfield_def_ptr},
{"translation_mtr", MXT_MTR_TRANSLATION, MXC_MOTOR,       MXR_DEVICE,
				&mxd_trans_motor_record_function_list,
				NULL,
				&mxd_trans_motor_motor_function_list,
				&mxd_trans_motor_num_record_fields,
				&mxd_trans_motor_rfield_def_ptr},
{"xafs_wavenumber", MXT_MTR_XAFS_WAVENUMBER,
                                      MXC_MOTOR,          MXR_DEVICE,
				&mxd_xafswn_motor_record_function_list,
				NULL,
				&mxd_xafswn_motor_motor_function_list,
				&mxd_xafswn_motor_num_record_fields,
				&mxd_xafswn_motor_rfield_def_ptr},
{"delta_motor",    MXT_MTR_DELTA,     MXC_MOTOR,          MXR_DEVICE,
				&mxd_delta_motor_record_function_list,
				NULL,
				&mxd_delta_motor_motor_function_list,
				&mxd_delta_motor_num_record_fields,
				&mxd_delta_motor_rfield_def_ptr},
{"elapsed_time",   MXT_MTR_ELAPSED_TIME, MXC_MOTOR,       MXR_DEVICE,
				&mxd_elapsed_time_record_function_list,
				NULL,
				&mxd_elapsed_time_motor_function_list,
				&mxd_elapsed_time_num_record_fields,
				&mxd_elapsed_time_rfield_def_ptr},
{"monochromator",  MXT_MTR_MONOCHROMATOR, MXC_MOTOR,      MXR_DEVICE,
				&mxd_monochromator_record_function_list,
				NULL,
				&mxd_monochromator_motor_function_list,
				&mxd_monochromator_num_record_fields,
				&mxd_monochromator_rfield_def_ptr},
{"linear_function", MXT_MTR_LINEAR_FUNCTION, MXC_MOTOR,   MXR_DEVICE,
				&mxd_linear_function_record_function_list,
				NULL,
				&mxd_linear_function_motor_function_list,
				&mxd_linear_function_num_record_fields,
				&mxd_linear_function_rfield_def_ptr},
{"table_motor",    MXT_MTR_TABLE,     MXC_MOTOR,   MXR_DEVICE,
				&mxd_table_motor_record_function_list,
				NULL,
				&mxd_table_motor_motor_function_list,
				&mxd_table_motor_num_record_fields,
				&mxd_table_motor_rfield_def_ptr},
{"theta_2theta",   MXT_MTR_THETA_2THETA,     MXC_MOTOR,   MXR_DEVICE,
				&mxd_theta_2theta_motor_record_function_list,
				NULL,
				&mxd_theta_2theta_motor_motor_function_list,
				&mxd_theta_2theta_motor_num_record_fields,
				&mxd_theta_2theta_motor_rfield_def_ptr},
{"q_motor",        MXT_MTR_Q,                MXC_MOTOR,   MXR_DEVICE,
				&mxd_q_motor_record_function_list,
				NULL,
				&mxd_q_motor_motor_function_list,
				&mxd_q_motor_num_record_fields,
				&mxd_q_motor_rfield_def_ptr},
{"segmented_move", MXT_MTR_SEGMENTED_MOVE,   MXC_MOTOR,   MXR_DEVICE,
				&mxd_segmented_move_record_function_list,
				NULL,
				&mxd_segmented_move_motor_function_list,
				&mxd_segmented_move_num_record_fields,
				&mxd_segmented_move_rfield_def_ptr},
{"tangent_arm",    MXT_MTR_TANGENT_ARM,      MXC_MOTOR,   MXR_DEVICE,
				&mxd_tangent_arm_record_function_list,
				NULL,
				&mxd_tangent_arm_motor_function_list,
				&mxd_tangent_arm_num_record_fields,
				&mxd_tangent_arm_rfield_def_ptr},
{"sine_arm",       MXT_MTR_SINE_ARM,         MXC_MOTOR,   MXR_DEVICE,
				&mxd_tangent_arm_record_function_list,
				NULL,
				&mxd_tangent_arm_motor_function_list,
				&mxd_tangent_arm_num_record_fields,
				&mxd_tangent_arm_rfield_def_ptr},
{"aframe_det_motor", MXT_MTR_AFRAME_DETECTOR_MOTOR, MXC_MOTOR, MXR_DEVICE,
				&mxd_aframe_det_motor_record_function_list,
				NULL,
				&mxd_aframe_det_motor_motor_function_list,
				&mxd_aframe_det_motor_num_record_fields,
				&mxd_aframe_det_motor_rfield_def_ptr},

{"adsc_two_theta", MXT_MTR_ADSC_TWO_THETA, MXC_MOTOR,     MXR_DEVICE,
				&mxd_adsc_two_theta_record_function_list,
				NULL,
				&mxd_adsc_two_theta_motor_function_list,
				&mxd_adsc_two_theta_num_record_fields,
				&mxd_adsc_two_theta_rfield_def_ptr},

{"record_field_motor", MXT_MTR_RECORD_FIELD, MXC_MOTOR,     MXR_DEVICE,
				&mxd_record_field_motor_record_function_list,
				NULL,
				&mxd_record_field_motor_motor_function_list,
				&mxd_record_field_motor_num_record_fields,
				&mxd_record_field_motor_rfield_def_ptr},

{"gated_backlash",    MXT_MTR_GATED_BACKLASH, MXC_MOTOR,     MXR_DEVICE,
				&mxd_gated_backlash_record_function_list,
				NULL,
				&mxd_gated_backlash_motor_function_list,
				&mxd_gated_backlash_num_record_fields,
				&mxd_gated_backlash_rfield_def_ptr},

{"polynomial_motor", MXT_MTR_POLYNOMIAL, MXC_MOTOR,     MXR_DEVICE,
				&mxd_polynomial_motor_record_function_list,
				NULL,
				&mxd_polynomial_motor_motor_function_list,
				&mxd_polynomial_motor_num_record_fields,
				&mxd_polynomial_motor_rfield_def_ptr},

{"cubic_spline_motor", MXT_MTR_CUBIC_SPLINE, MXC_MOTOR,     MXR_DEVICE,
				&mxd_cubic_spline_motor_record_function_list,
				NULL,
				&mxd_cubic_spline_motor_motor_function_list,
				&mxd_cubic_spline_motor_num_record_fields,
				&mxd_cubic_spline_motor_rfield_def_ptr},

{"limited_move",       MXT_MTR_LIMITED_MOVE, MXC_MOTOR,     MXR_DEVICE,
				&mxd_limited_move_record_function_list,
				NULL,
				&mxd_limited_move_motor_function_list,
				&mxd_limited_move_num_record_fields,
				&mxd_limited_move_rfield_def_ptr},

{"als_dewar_positioner", MXT_MTR_ALS_DEWAR_POSITIONER, MXC_MOTOR, MXR_DEVICE,
				&mxd_als_dewar_positioner_record_function_list,
				NULL,
				&mxd_als_dewar_positioner_motor_function_list,
				&mxd_als_dewar_positioner_num_record_fields,
				&mxd_als_dewar_positioner_rfield_def_ptr},

{"coordinated_angle", MXT_MTR_COORDINATED_ANGLE, MXC_MOTOR, MXR_DEVICE,
				&mxd_coordinated_angle_record_function_list,
				NULL,
				&mxd_coordinated_angle_motor_function_list,
				&mxd_coordinated_angle_num_record_fields,
				&mxd_coordinated_angle_rfield_def_ptr},

{"compumotor_lin", MXT_MTR_COMPUMOTOR_LINEAR, MXC_MOTOR, MXR_DEVICE,
				&mxd_compumotor_linear_record_function_list,
				NULL,
				&mxd_compumotor_linear_motor_function_list,
				&mxd_compumotor_linear_num_record_fields,
				&mxd_compumotor_linear_rfield_def_ptr},

#if 0
{"encoder_soft",   MXT_ENC_SOFTWARE,  MXC_ENCODER,        MXR_DEVICE,
				NULL, NULL, NULL, NULL, NULL},
#endif

{"ks3640",         MXT_ENC_KS3640,    MXC_ENCODER,        MXR_DEVICE,
				&mxd_ks3640_record_function_list,
				NULL,
				&mxd_ks3640_encoder_function_list,
				&mxd_ks3640_num_record_fields,
				&mxd_ks3640_rfield_def_ptr},

{"soft_scaler",    MXT_SCL_SOFTWARE,  MXC_SCALER,         MXR_DEVICE,
				&mxd_soft_scaler_record_function_list,
				NULL,
				&mxd_soft_scaler_scaler_function_list,
				&mxd_soft_scaler_num_record_fields,
				&mxd_soft_scaler_rfield_def_ptr},

{"network_scaler", MXT_SCL_NETWORK,   MXC_SCALER,         MXR_DEVICE,
				&mxd_network_scaler_record_function_list,
				NULL,
				&mxd_network_scaler_scaler_function_list,
				&mxd_network_scaler_num_record_fields,
				&mxd_network_scaler_rfield_def_ptr},

{"autoscale_scaler", MXT_SCL_AUTOSCALE, MXC_SCALER,       MXR_DEVICE,
				&mxd_autoscale_scaler_record_function_list,
				NULL,
				&mxd_autoscale_scaler_scaler_function_list,
				&mxd_autoscale_scaler_num_record_fields,
				&mxd_autoscale_scaler_rfield_def_ptr},

{"gain_tracking",  MXT_SCL_GAIN_TRACKING, MXC_SCALER,       MXR_DEVICE,
				&mxd_gain_tracking_scaler_record_function_list,
				NULL,
				&mxd_gain_tracking_scaler_scaler_function_list,
				&mxd_gain_tracking_scaler_num_record_fields,
				&mxd_gain_tracking_scaler_rfield_def_ptr},

{"scaler_function", MXT_SCL_SCALER_FUNCTION, MXC_SCALER,       MXR_DEVICE,
				&mxd_scaler_function_record_function_list,
				NULL,
				&mxd_scaler_function_scaler_function_list,
				&mxd_scaler_function_num_record_fields,
				&mxd_scaler_function_rfield_def_ptr},

/* The DSP QS450 and the KS 3610 share a driver. */

{"qs450",          MXT_SCL_QS450,     MXC_SCALER,         MXR_DEVICE,
				&mxd_qs450_record_function_list,
				NULL,
				&mxd_qs450_scaler_function_list,
				&mxd_qs450_num_record_fields,
				&mxd_qs450_record_field_def_ptr},

{"ks3610",         MXT_SCL_KS3610,    MXC_SCALER,         MXR_DEVICE,
				&mxd_qs450_record_function_list,
				NULL,
				&mxd_qs450_scaler_function_list,
				&mxd_ks3610_num_record_fields,
				&mxd_ks3610_record_field_def_ptr},

{"ortec974_scaler", MXT_SCL_ORTEC974, MXC_SCALER,         MXR_DEVICE,
				&mxd_ortec974_scaler_record_function_list,
				NULL,
				&mxd_ortec974_scaler_scaler_function_list,
				&mxd_ortec974_scaler_num_record_fields,
				&mxd_ortec974_scaler_rfield_def_ptr},

{"ortec974_timer", MXT_TIM_ORTEC974,  MXC_TIMER,          MXR_DEVICE,
				&mxd_ortec974_timer_record_function_list,
				NULL,
				&mxd_ortec974_timer_timer_function_list,
				&mxd_ortec974_timer_num_record_fields,
				&mxd_ortec974_timer_rfield_def_ptr},

{"vsc16_scaler",   MXT_SCL_VSC16,     MXC_SCALER,         MXR_DEVICE,
				&mxd_vsc16_scaler_record_function_list,
				NULL,
				&mxd_vsc16_scaler_scaler_function_list,
				&mxd_vsc16_scaler_num_record_fields,
				&mxd_vsc16_scaler_rfield_def_ptr},

{"vsc16_timer",    MXT_TIM_VSC16,     MXC_TIMER,          MXR_DEVICE,
				&mxd_vsc16_timer_record_function_list,
				NULL,
				&mxd_vsc16_timer_timer_function_list,
				&mxd_vsc16_timer_num_record_fields,
				&mxd_vsc16_timer_rfield_def_ptr},

{"pdi45_scaler",   MXT_SCL_PDI45,    MXC_SCALER,         MXR_DEVICE,
				&mxd_pdi45_scaler_record_function_list,
				NULL,
				&mxd_pdi45_scaler_scaler_function_list,
				&mxd_pdi45_scaler_num_record_fields,
				&mxd_pdi45_scaler_rfield_def_ptr},

{"pdi45_timer",    MXT_TIM_PDI45,    MXC_TIMER,          MXR_DEVICE,
				&mxd_pdi45_timer_record_function_list,
				NULL,
				&mxd_pdi45_timer_timer_function_list,
				&mxd_pdi45_timer_num_record_fields,
				&mxd_pdi45_timer_rfield_def_ptr},

{"scipe_scaler",   MXT_SCL_SCIPE,     MXC_SCALER,         MXR_DEVICE,
				&mxd_scipe_scaler_record_function_list,
				NULL,
				&mxd_scipe_scaler_scaler_function_list,
				&mxd_scipe_scaler_num_record_fields,
				&mxd_scipe_scaler_rfield_def_ptr},

{"scipe_timer",    MXT_TIM_SCIPE,     MXC_TIMER,          MXR_DEVICE,
				&mxd_scipe_timer_record_function_list,
				NULL,
				&mxd_scipe_timer_timer_function_list,
				&mxd_scipe_timer_num_record_fields,
				&mxd_scipe_timer_rfield_def_ptr},

{"mca_channel",    MXT_SCL_MCA_CHANNEL, MXC_SCALER,       MXR_DEVICE,
				&mxd_mca_channel_record_function_list,
				NULL,
				&mxd_mca_channel_scaler_function_list,
				&mxd_mca_channel_num_record_fields,
				&mxd_mca_channel_rfield_def_ptr},

{"mca_roi_integral", MXT_SCL_MCA_ROI_INTEGRAL, MXC_SCALER, MXR_DEVICE,
				&mxd_mca_roi_integral_record_function_list,
				NULL,
				&mxd_mca_roi_integral_scaler_function_list,
				&mxd_mca_roi_integral_num_record_fields,
				&mxd_mca_roi_integral_rfield_def_ptr},

{"mca_alt_time",   MXT_SCL_MCA_ALTERNATE_TIME, MXC_SCALER, MXR_DEVICE,
				&mxd_mca_alt_time_record_function_list,
				NULL,
				&mxd_mca_alt_time_scaler_function_list,
				&mxd_mca_alt_time_num_record_fields,
				&mxd_mca_alt_time_rfield_def_ptr},

{"mcs_scaler",     MXT_SCL_MCS,       MXC_SCALER,         MXR_DEVICE,
				&mxd_mcs_scaler_record_function_list,
				NULL,
				&mxd_mcs_scaler_scaler_function_list,
				&mxd_mcs_scaler_num_record_fields,
				&mxd_mcs_scaler_rfield_def_ptr},

{"databox_scaler", MXT_SCL_DATABOX,   MXC_SCALER,         MXR_DEVICE,
				&mxd_databox_scaler_record_function_list,
				NULL,
				&mxd_databox_scaler_scaler_function_list,
				&mxd_databox_scaler_num_record_fields,
				&mxd_databox_scaler_rfield_def_ptr},

{"spec_scaler",    MXT_SCL_SPEC,      MXC_SCALER,         MXR_DEVICE,
				&mxd_spec_scaler_record_function_list,
				NULL,
				&mxd_spec_scaler_scaler_function_list,
				&mxd_spec_scaler_num_record_fields,
				&mxd_spec_scaler_rfield_def_ptr},

{"gm10_scaler",    MXT_SCL_GM10,      MXC_SCALER,         MXR_DEVICE,
				&mxd_gm10_scaler_record_function_list,
				NULL,
				&mxd_gm10_scaler_scaler_function_list,
				&mxd_gm10_scaler_num_record_fields,
				&mxd_gm10_scaler_rfield_def_ptr},

{"function_generator_scaler",
		   MXT_SCL_FUNCTION_GENERATOR, MXC_SCALER, MXR_DEVICE,
				&mxd_fg_scaler_record_function_list,
				NULL,
				&mxd_fg_scaler_scaler_function_list,
				&mxd_fg_scaler_num_record_fields,
				&mxd_fg_scaler_rfield_def_ptr},

{"mcs_encoder",    MXT_MCE_MCS,       MXC_MULTICHANNEL_ENCODER, MXR_DEVICE,
				&mxd_mcs_encoder_record_function_list,
				NULL,
				&mxd_mcs_encoder_mce_function_list,
				&mxd_mcs_encoder_num_record_fields,
				&mxd_mcs_encoder_rfield_def_ptr},

{"databox_encoder",MXT_MCE_DATABOX,   MXC_MULTICHANNEL_ENCODER, MXR_DEVICE,
				&mxd_databox_encoder_record_function_list,
				NULL,
				&mxd_databox_encoder_mce_function_list,
				&mxd_databox_encoder_num_record_fields,
				&mxd_databox_encoder_rfield_def_ptr},

{"network_mce",    MXT_MCE_NETWORK,   MXC_MULTICHANNEL_ENCODER, MXR_DEVICE,
				&mxd_network_mce_record_function_list,
				NULL,
				&mxd_network_mce_mce_function_list,
				&mxd_network_mce_num_record_fields,
				&mxd_network_mce_rfield_def_ptr},

{"mcs_time_mce",   MXT_MCE_MCS_TIME,  MXC_MULTICHANNEL_ENCODER, MXR_DEVICE,
				&mxd_mcs_time_mce_record_function_list,
				NULL,
				&mxd_mcs_time_mce_mce_function_list,
				&mxd_mcs_time_mce_num_record_fields,
				&mxd_mcs_time_mce_rfield_def_ptr},

{"pmac_mce",       MXT_MCE_PMAC,      MXC_MULTICHANNEL_ENCODER, MXR_DEVICE,
				&mxd_pmac_mce_record_function_list,
				NULL,
				&mxd_pmac_mce_mce_function_list,
				&mxd_pmac_mce_num_record_fields,
				&mxd_pmac_mce_rfield_def_ptr},

{"soft_timer",     MXT_TIM_SOFTWARE,  MXC_TIMER,          MXR_DEVICE,
				&mxd_soft_timer_record_function_list,
				NULL,
				&mxd_soft_timer_timer_function_list,
				&mxd_soft_timer_num_record_fields,
				&mxd_soft_timer_rfield_def_ptr},

{"network_timer",  MXT_TIM_NETWORK,   MXC_TIMER,          MXR_DEVICE,
				&mxd_network_timer_record_function_list,
				NULL,
				&mxd_network_timer_timer_function_list,
				&mxd_network_timer_num_record_fields,
				&mxd_network_timer_rfield_def_ptr},

{"timer_fanout",   MXT_TIM_FANOUT,    MXC_TIMER,          MXR_DEVICE,
				&mxd_timer_fanout_record_function_list,
				NULL,
				&mxd_timer_fanout_timer_function_list,
				&mxd_timer_fanout_num_record_fields,
				&mxd_timer_fanout_rfield_def_ptr},

{"rtc018",         MXT_TIM_RTC018,    MXC_TIMER,          MXR_DEVICE,
				&mxd_rtc018_record_function_list,
				NULL,
				&mxd_rtc018_timer_function_list,
				&mxd_rtc018_num_record_fields,
				&mxd_rtc018_rfield_def_ptr},

{"mca_timer",      MXT_TIM_MCA,       MXC_TIMER,          MXR_DEVICE,
				&mxd_mca_timer_record_function_list,
				NULL,
				&mxd_mca_timer_timer_function_list,
				&mxd_mca_timer_num_record_fields,
				&mxd_mca_timer_rfield_def_ptr},

{"mcs_timer",      MXT_TIM_MCS,       MXC_TIMER,          MXR_DEVICE,
				&mxd_mcs_timer_record_function_list,
				NULL,
				&mxd_mcs_timer_timer_function_list,
				&mxd_mcs_timer_num_record_fields,
				&mxd_mcs_timer_rfield_def_ptr},

{"databox_timer",  MXT_TIM_DATABOX,   MXC_TIMER,          MXR_DEVICE,
				&mxd_databox_timer_record_function_list,
				NULL,
				&mxd_databox_timer_timer_function_list,
				&mxd_databox_timer_num_record_fields,
				&mxd_databox_timer_rfield_def_ptr},

{"pfcu_shutter_timer", MXT_TIM_PFCU_SHUTTER, MXC_TIMER,   MXR_DEVICE,
				&mxd_pfcu_shutter_timer_record_function_list,
				NULL,
				&mxd_pfcu_shutter_timer_timer_function_list,
				&mxd_pfcu_shutter_timer_num_record_fields,
				&mxd_pfcu_shutter_timer_rfield_def_ptr},

{"spec_timer",     MXT_TIM_SPEC,      MXC_TIMER,          MXR_DEVICE,
				&mxd_spec_timer_record_function_list,
				NULL,
				&mxd_spec_timer_timer_function_list,
				&mxd_spec_timer_num_record_fields,
				&mxd_spec_timer_rfield_def_ptr},

{"gm10_timer",     MXT_TIM_GM10,      MXC_TIMER,          MXR_DEVICE,
				&mxd_gm10_timer_record_function_list,
				NULL,
				&mxd_gm10_timer_timer_function_list,
				&mxd_gm10_timer_num_record_fields,
				&mxd_gm10_timer_rfield_def_ptr},

{"interval_timer", MXT_TIM_INTERVAL,      MXC_TIMER,          MXR_DEVICE,
				&mxd_interval_timer_record_function_list,
				NULL,
				&mxd_interval_timer_timer_function_list,
				&mxd_interval_timer_num_record_fields,
				&mxd_interval_timer_rfield_def_ptr},

{"bluice_dcss_timer",   MXT_TIM_BLUICE_DCSS, MXC_TIMER,       MXR_DEVICE,
				&mxd_bluice_timer_record_function_list,
				NULL,
				&mxd_bluice_timer_timer_function_list,
				&mxd_bluice_dcss_timer_num_record_fields,
				&mxd_bluice_dcss_timer_rfield_def_ptr},

{"bluice_dhs_timer",    MXT_TIM_BLUICE_DHS,  MXC_TIMER,       MXR_DEVICE,
				&mxd_bluice_timer_record_function_list,
				NULL,
				&mxd_bluice_timer_timer_function_list,
				&mxd_bluice_dhs_timer_num_record_fields,
				&mxd_bluice_dhs_timer_rfield_def_ptr},

{"area_detector_timer", MXT_TIM_AREA_DETECTOR, MXC_TIMER,      MXR_DEVICE,
				&mxd_area_detector_timer_record_function_list,
				NULL,
				&mxd_area_detector_timer_timer_function_list,
				&mxd_area_detector_timer_num_record_fields,
				&mxd_area_detector_timer_rfield_def_ptr},

{"soft_amplifier", MXT_AMP_SOFTWARE,  MXC_AMPLIFIER,      MXR_DEVICE,
				&mxd_soft_amplifier_record_function_list,
				NULL,
				&mxd_soft_amplifier_amplifier_function_list,
				&mxd_soft_amplifier_num_record_fields,
				&mxd_soft_amplifier_rfield_def_ptr},

{"network_amplifier", MXT_AMP_NETWORK, MXC_AMPLIFIER,      MXR_DEVICE,
				&mxd_network_amplifier_record_function_list,
				NULL,
				&mxd_network_amplifier_amplifier_function_list,
				&mxd_network_amplifier_num_record_fields,
				&mxd_network_amplifier_rfield_def_ptr},

/* The old name for network_amplifier. */

{"net_amplifier",  MXT_AMP_NETWORK,   MXC_AMPLIFIER,      MXR_DEVICE,
				&mxd_network_amplifier_record_function_list,
				NULL,
				&mxd_network_amplifier_amplifier_function_list,
				&mxd_network_amplifier_num_record_fields,
				&mxd_network_amplifier_rfield_def_ptr},

{"keithley428",    MXT_AMP_KEITHLEY428, MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_keithley428_record_function_list,
				NULL,
				&mxd_keithley428_amplifier_function_list,
				&mxd_keithley428_num_record_fields,
				&mxd_keithley428_rfield_def_ptr},

{"sr570",          MXT_AMP_SR570,       MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_sr570_record_function_list,
				NULL,
				&mxd_sr570_amplifier_function_list,
				&mxd_sr570_num_record_fields,
				&mxd_sr570_rfield_def_ptr},

{"udt_tramp",      MXT_AMP_UDT_TRAMP,   MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_udt_tramp_record_function_list,
				NULL,
				&mxd_udt_tramp_amplifier_function_list,
				&mxd_udt_tramp_num_record_fields,
				&mxd_udt_tramp_rfield_def_ptr},

{"keithley2700_amp", MXT_AMP_KEITHLEY2700, MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_keithley2700_amp_record_function_list,
				NULL,
				&mxd_keithley2700_amp_amplifier_function_list,
				&mxd_keithley2700_amp_num_record_fields,
				&mxd_keithley2700_amp_rfield_def_ptr},

{"keithley2400_amp", MXT_AMP_KEITHLEY2400, MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_keithley2400_amp_record_function_list,
				NULL,
				&mxd_keithley2400_amp_amplifier_function_list,
				&mxd_keithley2400_amp_num_record_fields,
				&mxd_keithley2400_amp_rfield_def_ptr},

{"sim983",           MXT_AMP_SIM983,       MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_sim983_record_function_list,
				NULL,
				&mxd_sim983_amplifier_function_list,
				&mxd_sim983_num_record_fields,
				&mxd_sim983_rfield_def_ptr},

{"i404_amp",         MXT_AMP_I404,         MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_i404_amp_record_function_list,
				NULL,
				&mxd_i404_amp_amplifier_function_list,
				&mxd_i404_amp_num_record_fields,
				&mxd_i404_amp_rfield_def_ptr},

{"scipe_amplifier",MXT_AMP_SCIPE,       MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_scipe_amplifier_record_function_list,
				NULL,
				&mxd_scipe_amplifier_amplifier_function_list,
				&mxd_scipe_amplifier_num_record_fields,
				&mxd_scipe_amplifier_rfield_def_ptr},

{"aps_adcmod2_amplifier", MXT_AMP_APS_ADCMOD2, MXC_AMPLIFIER, MXR_DEVICE,
				&mxd_aps_adcmod2_record_function_list,
				NULL,
				&mxd_aps_adcmod2_amplifier_function_list,
				&mxd_aps_adcmod2_num_record_fields,
				&mxd_aps_adcmod2_rfield_def_ptr},

{"aps_adcmod2_ainput", MXT_AIN_APS_ADCMOD2, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_aps_adcmod2_ainput_record_function_list,
				NULL,
			&mxd_aps_adcmod2_ainput_analog_input_function_list,
				&mxd_aps_adcmod2_ainput_num_record_fields,
				&mxd_aps_adcmod2_ainput_rfield_def_ptr},

{"icplus",         MXT_AMP_ICPLUS,      MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_icplus_record_function_list,
				NULL,
				&mxd_icplus_amplifier_function_list,
				&mxd_icplus_num_record_fields,
				&mxd_icplus_rfield_def_ptr},

{"icplus_current", MXT_AIN_ICPLUS,    MXC_ANALOG_INPUT,  MXR_DEVICE,
				&mxd_icplus_ain_record_function_list,
				NULL,
				&mxd_icplus_ain_analog_input_function_list,
				&mxd_icplus_ain_num_record_fields,
				&mxd_icplus_ain_rfield_def_ptr},

{"icplus_voltage", MXT_AOU_ICPLUS,    MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_icplus_aout_record_function_list,
				NULL,
				&mxd_icplus_aout_analog_output_function_list,
				&mxd_icplus_aout_num_record_fields,
				&mxd_icplus_aout_rfield_def_ptr},

{"icplus_din",     MXT_DIN_ICPLUS,    MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_icplus_din_record_function_list,
				NULL,
				&mxd_icplus_din_digital_input_function_list,
				&mxd_icplus_din_num_record_fields,
				&mxd_icplus_din_rfield_def_ptr},

{"icplus_dout",    MXT_DOU_ICPLUS,    MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_icplus_dout_record_function_list,
				NULL,
				&mxd_icplus_dout_digital_output_function_list,
				&mxd_icplus_dout_num_record_fields,
				&mxd_icplus_dout_rfield_def_ptr},

{"qbpm",           MXT_AMP_QBPM,      MXC_AMPLIFIER,    MXR_DEVICE,
				&mxd_icplus_record_function_list,
				NULL,
				&mxd_icplus_amplifier_function_list,
				&mxd_qbpm_num_record_fields,
				&mxd_qbpm_rfield_def_ptr},

{"qbpm_ain",       MXT_AIN_QBPM,    MXC_ANALOG_INPUT,  MXR_DEVICE,
				&mxd_icplus_ain_record_function_list,
				NULL,
				&mxd_icplus_ain_analog_input_function_list,
				&mxd_icplus_ain_num_record_fields,
				&mxd_icplus_ain_rfield_def_ptr},

{"qbpm_aout",      MXT_AOU_QBPM,    MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_icplus_aout_record_function_list,
				NULL,
				&mxd_icplus_aout_analog_output_function_list,
				&mxd_icplus_aout_num_record_fields,
				&mxd_icplus_aout_rfield_def_ptr},

{"qbpm_din",       MXT_DIN_QBPM,    MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_icplus_din_record_function_list,
				NULL,
				&mxd_icplus_din_digital_input_function_list,
				&mxd_icplus_din_num_record_fields,
				&mxd_icplus_din_rfield_def_ptr},

{"qbpm_dout",      MXT_DOU_QBPM,    MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_icplus_dout_record_function_list,
				NULL,
				&mxd_icplus_dout_digital_output_function_list,
				&mxd_icplus_dout_num_record_fields,
				&mxd_icplus_dout_rfield_def_ptr},

{"qbpm_mcai",      MXT_MCAI_QBPM,   MXC_MULTICHANNEL_ANALOG_INPUT, MXR_DEVICE,
				&mxd_qbpm_mcai_record_function_list,
				NULL,
				&mxd_qbpm_mcai_mcai_function_list,
				&mxd_qbpm_mcai_num_record_fields,
				&mxd_qbpm_mcai_rfield_def_ptr},

{"soft_mca",       MXT_MCA_SOFTWARE,     MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_soft_mca_record_function_list,
				NULL,
				&mxd_soft_mca_mca_function_list,
				&mxd_soft_mca_num_record_fields,
				&mxd_soft_mca_rfield_def_ptr},

{"monte_carlo_mca", MXT_MCA_MONTE_CARLO,  MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_monte_carlo_mca_record_function_list,
				NULL,
				&mxd_monte_carlo_mca_mca_function_list,
				&mxd_monte_carlo_mca_num_record_fields,
				&mxd_monte_carlo_mca_rfield_def_ptr},

{"network_mca",    MXT_MCA_NETWORK,   MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_network_mca_record_function_list,
				NULL,
				&mxd_network_mca_mca_function_list,
				&mxd_network_mca_num_record_fields,
				&mxd_network_mca_rfield_def_ptr},

{"roentec_rcl_mca", MXT_MCA_ROENTEC_RCL, MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_roentec_rcl_record_function_list,
				NULL,
				&mxd_roentec_rcl_mca_function_list,
				&mxd_roentec_rcl_num_record_fields,
				&mxd_roentec_rcl_rfield_def_ptr},


{"soft_mcs",       MXT_MCS_SOFTWARE,  MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				&mxd_soft_mcs_record_function_list,
				NULL,
				&mxd_soft_mcs_mcs_function_list,
				&mxd_soft_mcs_num_record_fields,
				&mxd_soft_mcs_rfield_def_ptr},

{"network_mcs",    MXT_MCS_NETWORK,   MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				&mxd_network_mcs_record_function_list,
				NULL,
				&mxd_network_mcs_mcs_function_list,
				&mxd_network_mcs_num_record_fields,
				&mxd_network_mcs_rfield_def_ptr},

{"scaler_function_mcs", MXT_MCS_SCALER_FUNCTION,
				      MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				&mxd_scaler_function_mcs_record_function_list,
				NULL,
				&mxd_scaler_function_mcs_mcs_function_list,
				&mxd_scaler_function_mcs_num_record_fields,
				&mxd_scaler_function_mcs_rfield_def_ptr},

{"databox_mcs",    MXT_MCS_DATABOX,   MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				&mxd_databox_mcs_record_function_list,
				NULL,
				&mxd_databox_mcs_mcs_function_list,
				&mxd_databox_mcs_num_record_fields,
				&mxd_databox_mcs_rfield_def_ptr},

{"sis3801",        MXT_MCS_SIS3801,   MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				&mxd_sis3801_record_function_list,
				NULL,
				&mxd_sis3801_mcs_function_list,
				&mxd_sis3801_num_record_fields,
				&mxd_sis3801_rfield_def_ptr},

{"network_relay",  MXT_RLY_NETWORK,   MXC_RELAY,          MXR_DEVICE,
				&mxd_network_relay_record_function_list,
				NULL,
				&mxd_network_relay_relay_function_list,
				&mxd_network_relay_num_record_fields,
				&mxd_network_relay_rfield_def_ptr},

{"generic_relay",  MXT_RLY_GENERIC,   MXC_RELAY,          MXR_DEVICE,
				&mxd_generic_relay_record_function_list,
				NULL,
				&mxd_generic_relay_relay_function_list,
				&mxd_generic_relay_num_record_fields,
				&mxd_generic_relay_rfield_def_ptr},

{"blind_relay",    MXT_RLY_BLIND,     MXC_RELAY,          MXR_DEVICE,
				&mxd_blind_relay_record_function_list,
				NULL,
				&mxd_blind_relay_relay_function_list,
				&mxd_blind_relay_num_record_fields,
				&mxd_blind_relay_rfield_def_ptr},

{"pulsed_relay",   MXT_RLY_PULSED,     MXC_RELAY,          MXR_DEVICE,
				&mxd_pulsed_relay_record_function_list,
				NULL,
				&mxd_pulsed_relay_relay_function_list,
				&mxd_pulsed_relay_num_record_fields,
				&mxd_pulsed_relay_rfield_def_ptr},

{"pfcu_filter",    MXT_RLY_PFCU_FILTER, MXC_RELAY,        MXR_DEVICE,
				&mxd_pfcu_record_function_list,
				NULL,
				&mxd_pfcu_relay_function_list,
				&mxd_pfcu_filter_num_record_fields,
				&mxd_pfcu_filter_rfield_def_ptr},

{"pfcu_shutter",   MXT_RLY_PFCU_SHUTTER, MXC_RELAY,       MXR_DEVICE,
				&mxd_pfcu_record_function_list,
				NULL,
				&mxd_pfcu_relay_function_list,
				&mxd_pfcu_shutter_num_record_fields,
				&mxd_pfcu_shutter_rfield_def_ptr},

{"marccd_shutter", MXT_RLY_MARCCD_SHUTTER, MXC_RELAY,          MXR_DEVICE,
				&mxd_marccd_shutter_record_function_list,
				NULL,
				&mxd_marccd_shutter_rly_function_list,
				&mxd_marccd_shutter_num_record_fields,
				&mxd_marccd_shutter_rfield_def_ptr},

{"mardtb_shutter", MXT_RLY_MARDTB_SHUTTER, MXC_RELAY,     MXR_DEVICE,
				&mxd_mardtb_shutter_record_function_list,
				NULL,
				&mxd_mardtb_shutter_relay_function_list,
				&mxd_mardtb_shutter_num_record_fields,
				&mxd_mardtb_shutter_rfield_def_ptr},

{"bluice_dcss_shutter", MXT_RLY_BLUICE_DCSS_SHUTTER, MXC_RELAY, MXR_DEVICE,
				&mxd_bluice_shutter_record_function_list,
				NULL,
				&mxd_bluice_shutter_relay_function_list,
				&mxd_bluice_shutter_num_record_fields,
				&mxd_bluice_shutter_rfield_def_ptr},

{"bluice_dhs_shutter", MXT_RLY_BLUICE_DHS_SHUTTER, MXC_RELAY, MXR_DEVICE,
				&mxd_bluice_shutter_record_function_list,
				NULL,
				&mxd_bluice_shutter_relay_function_list,
				&mxd_bluice_shutter_num_record_fields,
				&mxd_bluice_shutter_rfield_def_ptr},

{"synaccess_netbooter_relay",MXT_RLY_SYNACCESS_NETBOOTER, MXC_RELAY, MXR_DEVICE,
				&mxd_sa_netbooter_relay_record_function_list,
				NULL,
				&mxd_sa_netbooter_relay_relay_function_list,
				&mxd_sa_netbooter_relay_num_record_fields,
				&mxd_sa_netbooter_relay_rfield_def_ptr},

{"adc_table",      MXT_TAB_ADC,       MXC_TABLE,          MXR_DEVICE,
				&mxd_adc_table_record_function_list,
				NULL,
				&mxd_adc_table_table_function_list,
				&mxd_adc_table_num_record_fields,
				&mxd_adc_table_rfield_def_ptr},

{"autoscale_filter",MXT_AUT_FILTER,   MXC_AUTOSCALE,      MXR_DEVICE,
				&mxd_auto_filter_record_function_list,
				NULL,
				&mxd_auto_filter_autoscale_function_list,
				&mxd_auto_filter_num_record_fields,
				&mxd_auto_filter_rfield_def_ptr},

{"autoscale_amp",  MXT_AUT_AMPLIFIER, MXC_AUTOSCALE,      MXR_DEVICE,
				&mxd_auto_amplifier_record_function_list,
				NULL,
				&mxd_auto_amplifier_autoscale_function_list,
				&mxd_auto_amplifier_num_record_fields,
				&mxd_auto_amplifier_rfield_def_ptr},

{"autoscale_net",  MXT_AUT_NETWORK,   MXC_AUTOSCALE,      MXR_DEVICE,
				&mxd_auto_network_record_function_list,
				NULL,
				&mxd_auto_network_autoscale_function_list,
				&mxd_auto_network_num_record_fields,
				&mxd_auto_network_rfield_def_ptr},

{"auto_filter_amp",MXT_AUT_FILTER_AMPLIFIER, MXC_AUTOSCALE, MXR_DEVICE,
				&mxd_auto_filter_amp_record_function_list,
				NULL,
				&mxd_auto_filter_amp_autoscale_function_list,
				&mxd_auto_filter_amp_num_record_fields,
				&mxd_auto_filter_amp_rfield_def_ptr},

{"network_pulser", MXT_PGN_NETWORK,   MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_network_pulser_record_function_list,
				NULL,
			&mxd_network_pulser_pulse_generator_function_list,
				&mxd_network_pulser_num_record_fields,
				&mxd_network_pulser_rfield_def_ptr},

{"sis3807_pulser", MXT_PGN_SIS3807,   MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_sis3807_record_function_list,
				NULL,
				&mxd_sis3807_pulse_generator_function_list,
				&mxd_sis3807_num_record_fields,
				&mxd_sis3807_rfield_def_ptr},

{"sis3801_pulser", MXT_PGN_SIS3801,   MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_sis3801_pulser_record_function_list,
				NULL,
			&mxd_sis3801_pulser_pulse_generator_function_list,
				&mxd_sis3801_pulser_num_record_fields,
				&mxd_sis3801_pulser_rfield_def_ptr},

{"pdi45_pulser",   MXT_PGN_PDI45,     MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_pdi45_pulser_record_function_list,
				NULL,
				&mxd_pdi45_pulser_pulse_generator_function_list,
				&mxd_pdi45_pulser_num_record_fields,
				&mxd_pdi45_pulser_rfield_def_ptr},

{"digital_output_pulser", MXT_PGN_DIGITAL_OUTPUT,
						MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_doutput_pulser_record_function_list,
				NULL,
				&mxd_doutput_pulser_pulser_function_list,
				&mxd_doutput_pulser_num_record_fields,
				&mxd_doutput_pulser_rfield_def_ptr},

{"gittelson_pulser", MXT_PGN_GITTELSON, MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_gittelson_pulser_record_function_list,
				NULL,
				&mxd_gittelson_pulser_pulser_function_list,
				&mxd_gittelson_pulser_num_record_fields,
				&mxd_gittelson_pulser_rfield_def_ptr},

{"soft_sca",       MXT_SCA_SOFTWARE,  MXC_SINGLE_CHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_soft_sca_record_function_list,
				NULL,
				&mxd_soft_sca_sca_function_list,
				&mxd_soft_sca_num_record_fields,
				&mxd_soft_sca_rfield_def_ptr},

{"network_sca",    MXT_SCA_NETWORK,   MXC_SINGLE_CHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_network_sca_record_function_list,
				NULL,
				&mxd_network_sca_sca_function_list,
				&mxd_network_sca_num_record_fields,
				&mxd_network_sca_rfield_def_ptr},

{"cyberstar_x1000", MXT_SCA_CYBERSTAR_X1000,\
					MXC_SINGLE_CHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_cyberstar_x1000_record_function_list,
				NULL,
				&mxd_cyberstar_x1000_sca_function_list,
				&mxd_cyberstar_x1000_num_record_fields,
				&mxd_cyberstar_x1000_rfield_def_ptr},

{"cyberstar_x1000_aout", MXT_AOU_CYBERSTAR_X1000,\
					MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_cyberstar_x1000_aout_record_function_list,
				NULL,
			&mxd_cyberstar_x1000_aout_analog_output_function_list,
				&mxd_cyberstar_x1000_aout_num_record_fields,
				&mxd_cyberstar_x1000_aout_rfield_def_ptr},

{"soft_sample_changer",  MXT_CHG_SOFTWARE, MXC_SAMPLE_CHANGER, MXR_DEVICE,
				&mxd_soft_sample_changer_record_function_list,
				NULL,
			&mxd_soft_sample_changer_sample_changer_function_list,
				&mxd_soft_sample_changer_num_record_fields,
				&mxd_soft_sample_changer_rfield_def_ptr},

{"network_sample_changer",MXT_CHG_NETWORK, MXC_SAMPLE_CHANGER, MXR_DEVICE,
				&mxd_net_sample_changer_record_function_list,
				NULL,
			&mxd_net_sample_changer_sample_changer_function_list,
				&mxd_net_sample_changer_num_record_fields,
				&mxd_net_sample_changer_rfield_def_ptr},

{"sercat_als_robot", MXT_CHG_SERCAT_ALS_ROBOT, MXC_SAMPLE_CHANGER, MXR_DEVICE,
				&mxd_sercat_als_robot_record_function_list,
				NULL,
			&mxd_sercat_als_robot_sample_changer_function_list,
				&mxd_sercat_als_robot_num_record_fields,
				&mxd_sercat_als_robot_rfield_def_ptr},

{"network_mcai",   MXT_MCAI_NETWORK, MXC_MULTICHANNEL_ANALOG_INPUT, MXR_DEVICE,
				&mxd_network_mcai_record_function_list,
				NULL,
				&mxd_network_mcai_mcai_function_list,
				&mxd_network_mcai_num_record_fields,
				&mxd_network_mcai_rfield_def_ptr},

{"i404_mcai",      MXT_MCAI_I404,   MXC_MULTICHANNEL_ANALOG_INPUT, MXR_DEVICE,
				&mxd_i404_mcai_record_function_list,
				NULL,
				&mxd_i404_mcai_mcai_function_list,
				&mxd_i404_mcai_num_record_fields,
				&mxd_i404_mcai_rfield_def_ptr},

{"soft_ptz",       MXT_PTZ_SOFTWARE,  MXC_PAN_TILT_ZOOM, MXR_DEVICE,
				&mxd_soft_ptz_record_function_list,
				NULL,
				&mxd_soft_ptz_ptz_function_list,
				&mxd_soft_ptz_num_record_fields,
				&mxd_soft_ptz_rfield_def_ptr},

{"network_ptz",    MXT_PTZ_NETWORK,  MXC_PAN_TILT_ZOOM, MXR_DEVICE,
				&mxd_network_ptz_record_function_list,
				NULL,
				&mxd_network_ptz_ptz_function_list,
				&mxd_network_ptz_num_record_fields,
				&mxd_network_ptz_rfield_def_ptr},

{"sony_visca_ptz", MXT_PTZ_SONY_VISCA, MXC_PAN_TILT_ZOOM, MXR_DEVICE,
				&mxd_sony_visca_ptz_record_function_list,
				NULL,
				&mxd_sony_visca_ptz_ptz_function_list,
				&mxd_sony_visca_ptz_num_record_fields,
				&mxd_sony_visca_ptz_rfield_def_ptr},

{"hitachi_kp_d20", MXT_PTZ_HITACHI_KP_D20, MXC_PAN_TILT_ZOOM, MXR_DEVICE,
				&mxd_hitachi_kp_d20_record_function_list,
				NULL,
				&mxd_hitachi_kp_d20_ptz_function_list,
				&mxd_hitachi_kp_d20_num_record_fields,
				&mxd_hitachi_kp_d20_rfield_def_ptr},

{"panasonic_kx_dp702_ptz",\
		MXT_PTZ_PANASONIC_KX_DP702, MXC_PAN_TILT_ZOOM, MXR_DEVICE,
				&mxd_panasonic_kx_dp702_record_function_list,
				NULL,
				&mxd_panasonic_kx_dp702_ptz_function_list,
				&mxd_panasonic_kx_dp702_num_record_fields,
				&mxd_panasonic_kx_dp702_rfield_def_ptr},

{"soft_vinput",     MXT_VIN_SOFTWARE,     MXC_VIDEO_INPUT,  MXR_DEVICE,
				&mxd_soft_vinput_record_function_list,
				NULL,
				NULL,
				&mxd_soft_vinput_num_record_fields,
				&mxd_soft_vinput_rfield_def_ptr},

{"network_vinput",  MXT_VIN_NETWORK,      MXC_VIDEO_INPUT,  MXR_DEVICE,
				&mxd_network_vinput_record_function_list,
				NULL,
				NULL,
				&mxd_network_vinput_num_record_fields,
				&mxd_network_vinput_rfield_def_ptr},

{"file_vinput",     MXT_VIN_FILE,         MXC_VIDEO_INPUT,  MXR_DEVICE,
				&mxd_file_vinput_record_function_list,
				NULL,
				NULL,
				&mxd_file_vinput_num_record_fields,
				&mxd_file_vinput_rfield_def_ptr},

{"sony_snc",        MXT_VIN_SONY_SNC,     MXC_VIDEO_INPUT,  MXR_DEVICE,
				&mxd_sony_snc_record_function_list,
				NULL,
				NULL,
				&mxd_sony_snc_num_record_fields,
				&mxd_sony_snc_rfield_def_ptr},

{"soft_area_detector",     MXT_AD_SOFTWARE,  MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_soft_area_detector_record_function_list,
				NULL,
				&mxd_soft_area_detector_ad_function_list,
				&mxd_soft_area_detector_num_record_fields,
				&mxd_soft_area_detector_rfield_def_ptr},

{"network_area_detector",  MXT_AD_NETWORK,   MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_network_area_detector_record_function_list,
				NULL,
				&mxd_network_area_detector_ad_function_list,
				&mxd_network_area_detector_num_record_fields,
				&mxd_network_area_detector_rfield_def_ptr},

{"pccd_170170",    MXT_AD_PCCD_170170, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_aviex_pccd_record_function_list,
				NULL,
				&mxd_aviex_pccd_ad_function_list,
				&mxd_aviex_pccd_170170_num_record_fields,
				&mxd_aviex_pccd_170170_rfield_def_ptr},

{"pccd_4824",      MXT_AD_PCCD_4824, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_aviex_pccd_record_function_list,
				NULL,
				&mxd_aviex_pccd_ad_function_list,
				&mxd_aviex_pccd_4824_num_record_fields,
				&mxd_aviex_pccd_4824_rfield_def_ptr},

{"pccd_16080",     MXT_AD_PCCD_16080, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_aviex_pccd_record_function_list,
				NULL,
				&mxd_aviex_pccd_ad_function_list,
				&mxd_aviex_pccd_16080_num_record_fields,
				&mxd_aviex_pccd_16080_rfield_def_ptr},

{"pccd_9785",      MXT_AD_PCCD_9785, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_aviex_pccd_record_function_list,
				NULL,
				&mxd_aviex_pccd_ad_function_list,
				&mxd_aviex_pccd_9785_num_record_fields,
				&mxd_aviex_pccd_9785_rfield_def_ptr},

{"marccd",         MXT_AD_MARCCD,      MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_marccd_record_function_list,
				NULL,
				&mxd_marccd_ad_function_list,
				&mxd_marccd_num_record_fields,
				&mxd_marccd_rfield_def_ptr},

{"marccd_server_socket",
		MXT_AD_MARCCD_SERVER_SOCKET, MXC_AREA_DETECTOR, MXR_DEVICE,
				&mxd_marccd_server_socket_record_function_list,
				NULL,
				&mxd_marccd_server_socket_ad_function_list,
				&mxd_marccd_server_socket_num_record_fields,
				&mxd_marccd_server_socket_rfield_def_ptr},

{"bluice_dcss_area_detector", MXT_AD_BLUICE_DCSS, MXC_AREA_DETECTOR, MXR_DEVICE,
				&mxd_bluice_area_detector_record_function_list,
				NULL,
				&mxd_bluice_area_detector_ad_function_list,
			      &mxd_bluice_dcss_area_detector_num_record_fields,
				&mxd_bluice_dcss_area_detector_rfield_def_ptr},

{"bluice_dhs_area_detector", MXT_AD_BLUICE_DHS, MXC_AREA_DETECTOR, MXR_DEVICE,
				&mxd_bluice_area_detector_record_function_list,
				NULL,
				&mxd_bluice_area_detector_ad_function_list,
				&mxd_bluice_dhs_area_detector_num_record_fields,
				&mxd_bluice_dhs_area_detector_rfield_def_ptr},

{"mlfsom",         MXT_AD_MLFSOM,      MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_mlfsom_record_function_list,
				NULL,
				&mxd_mlfsom_ad_function_list,
				&mxd_mlfsom_num_record_fields,
				&mxd_mlfsom_rfield_def_ptr},

{"mar345",         MXT_AD_MAR345,      MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_mar345_record_function_list,
				NULL,
				&mxd_mar345_ad_function_list,
				&mxd_mar345_num_record_fields,
				&mxd_mar345_rfield_def_ptr},

{"cryostream600_status", MXT_AIN_CRYOSTREAM600, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_cryostream600_status_record_function_list,
				NULL,
			&mxd_cryostream600_status_analog_input_function_list,
				&mxd_cryostream600_status_num_record_fields,
				&mxd_cryostream600_status_rfield_def_ptr},

{"cryostream600_motor", MXT_MTR_CRYOSTREAM600, MXC_MOTOR, MXR_DEVICE,
				&mxd_cryostream600_motor_record_function_list,
				NULL,
				&mxd_cryostream600_motor_motor_function_list,
				&mxd_cryostream600_motor_num_record_fields,
				&mxd_cryostream600_motor_rfield_def_ptr},


{"si9650_status",  MXT_AIN_SI9650,    MXC_ANALOG_INPUT,  MXR_DEVICE,
				&mxd_si9650_status_record_function_list,
				NULL,
				&mxd_si9650_status_analog_input_function_list,
				&mxd_si9650_status_num_record_fields,
				&mxd_si9650_status_rfield_def_ptr},

{"si9650_motor",   MXT_MTR_SI9650,    MXC_MOTOR,         MXR_DEVICE,
				&mxd_si9650_motor_record_function_list,
				NULL,
				&mxd_si9650_motor_motor_function_list,
				&mxd_si9650_motor_num_record_fields,
				&mxd_si9650_motor_rfield_def_ptr},

/*---*/

{"itc503_ainput",  MXT_AIN_ITC503,    MXC_ANALOG_INPUT,  MXR_DEVICE,
				&mxd_itc503_ainput_record_function_list,
				NULL,
				&mxd_itc503_ainput_analog_input_function_list,
				&mxd_itc503_ainput_num_record_fields,
				&mxd_itc503_ainput_rfield_def_ptr},

{"itc503_aoutput", MXT_AOU_ITC503,    MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_itc503_aoutput_record_function_list,
				NULL,
				&mxd_itc503_aoutput_analog_output_function_list,
				&mxd_itc503_aoutput_num_record_fields,
				&mxd_itc503_aoutput_rfield_def_ptr},

{"itc503_doutput", MXT_DOU_ITC503,    MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_itc503_doutput_record_function_list,
				NULL,
			    &mxd_itc503_doutput_digital_output_function_list,
				&mxd_itc503_doutput_num_record_fields,
				&mxd_itc503_doutput_rfield_def_ptr},

{"itc503_motor",   MXT_MTR_ITC503,    MXC_MOTOR,         MXR_DEVICE,
				&mxd_itc503_motor_record_function_list,
				NULL,
				&mxd_itc503_motor_motor_function_list,
				&mxd_itc503_motor_num_record_fields,
				&mxd_itc503_motor_rfield_def_ptr},

/*---*/

/* Cryojet devices use the ITC503 drivers. */

{"cryojet_ainput", MXT_AIN_CRYOJET,   MXC_ANALOG_INPUT,  MXR_DEVICE,
				&mxd_itc503_ainput_record_function_list,
				NULL,
				&mxd_itc503_ainput_analog_input_function_list,
				&mxd_itc503_ainput_num_record_fields,
				&mxd_itc503_ainput_rfield_def_ptr},

{"cryojet_aoutput", MXT_AOU_CRYOJET,  MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_itc503_aoutput_record_function_list,
				NULL,
				&mxd_itc503_aoutput_analog_output_function_list,
				&mxd_itc503_aoutput_num_record_fields,
				&mxd_itc503_aoutput_rfield_def_ptr},

{"cryojet_doutput", MXT_DOU_CRYOJET,  MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_itc503_doutput_record_function_list,
				NULL,
			    &mxd_itc503_doutput_digital_output_function_list,
				&mxd_itc503_doutput_num_record_fields,
				&mxd_itc503_doutput_rfield_def_ptr},

{"cryojet_motor",   MXT_MTR_CRYOJET,   MXC_MOTOR,         MXR_DEVICE,
				&mxd_itc503_motor_record_function_list,
				NULL,
				&mxd_itc503_motor_motor_function_list,
				&mxd_itc503_motor_num_record_fields,
				&mxd_itc503_motor_rfield_def_ptr},

/*---*/

{"am9513_motor",   MXT_MTR_AM9513,    MXC_MOTOR,         MXR_DEVICE,
				&mxd_am9513_motor_record_function_list,
				NULL,
				&mxd_am9513_motor_motor_function_list,
				&mxd_am9513_motor_num_record_fields,
				&mxd_am9513_motor_rfield_def_ptr},

{"am9513_scaler",  MXT_SCL_AM9513,    MXC_SCALER,         MXR_DEVICE,
				&mxd_am9513_scaler_record_function_list,
				NULL,
				&mxd_am9513_scaler_scaler_function_list,
				&mxd_am9513_scaler_num_record_fields,
				&mxd_am9513_scaler_rfield_def_ptr},

{"am9513_timer",   MXT_TIM_AM9513,    MXC_TIMER,          MXR_DEVICE,
				&mxd_am9513_timer_record_function_list,
				NULL,
				&mxd_am9513_timer_timer_function_list,
				&mxd_am9513_timer_num_record_fields,
				&mxd_am9513_timer_rfield_def_ptr},

{"network_wvin",  MXT_WVI_NETWORK,   MXC_WAVEFORM_INPUT, MXR_DEVICE,
			&mxd_network_wvin_record_function_list,
			NULL,
			&mxd_network_wvin_wvin_function_list,
			&mxd_network_wvin_num_record_fields,
			&mxd_network_wvin_rfield_def_ptr},

{"network_wvout",  MXT_WVO_NETWORK,   MXC_WAVEFORM_OUTPUT, MXR_DEVICE,
			&mxd_network_wvout_record_function_list,
			NULL,
			&mxd_network_wvout_wvout_function_list,
			&mxd_network_wvout_num_record_fields,
			&mxd_network_wvout_rfield_def_ptr},

{"bkprecision_912x_wvout",
		MXT_WVO_BKPRECISION_912X, MXC_WAVEFORM_OUTPUT, MXR_DEVICE,
			&mxd_bkprecision_912x_wvout_record_function_list,
			NULL,
			&mxd_bkprecision_912x_wvout_wvout_function_list,
			&mxd_bkprecision_912x_wvout_num_record_fields,
			&mxd_bkprecision_912x_wvout_rfield_def_ptr},

  /* ===================== Scan types ===================== */

{"input_scan",     MXS_LIN_INPUT,     MXS_LINEAR_SCAN,    MXR_SCAN,
				&mxs_linear_scan_record_function_list,
				&mxs_linear_scan_scan_function_list,
				&mxs_input_linear_scan_function_list,
				&mxs_input_linear_scan_num_record_fields,
				&mxs_input_linear_scan_def_ptr},
{"motor_scan",     MXS_LIN_MOTOR,     MXS_LINEAR_SCAN,    MXR_SCAN,
				&mxs_linear_scan_record_function_list,
				&mxs_linear_scan_scan_function_list,
				&mxs_motor_linear_scan_function_list,
				&mxs_motor_linear_scan_num_record_fields,
				&mxs_motor_linear_scan_def_ptr},
{"2theta_scan",    MXS_LIN_2THETA,    MXS_LINEAR_SCAN,    MXR_SCAN,
				&mxs_linear_scan_record_function_list,
				&mxs_linear_scan_scan_function_list,
				&mxs_2theta_linear_scan_function_list,
				&mxs_2theta_linear_scan_num_record_fields,
				&mxs_2theta_linear_scan_def_ptr},
{"slit_scan",      MXS_LIN_SLIT,      MXS_LINEAR_SCAN,    MXR_SCAN,
				&mxs_linear_scan_record_function_list,
				&mxs_linear_scan_scan_function_list,
				&mxs_slit_linear_scan_function_list,
				&mxs_slit_linear_scan_num_record_fields,
				&mxs_slit_linear_scan_def_ptr},

{"pseudomotor_scan", MXS_LIN_PSEUDOMOTOR,MXS_LINEAR_SCAN, MXR_SCAN,
				&mxs_linear_scan_record_function_list,
				&mxs_linear_scan_scan_function_list,
				&mxs_pseudomotor_linear_scan_function_list,
				&mxs_pseudomotor_linear_scan_num_record_fields,
				&mxs_pseudomotor_linear_scan_def_ptr},

{"file_list_scan", MXS_LST_FILE,      MXS_LIST_SCAN,      MXR_SCAN,
				&mxs_list_scan_record_function_list,
				&mxs_list_scan_scan_function_list,
				&mxs_file_list_scan_function_list,
				&mxs_file_list_scan_num_record_fields,
				&mxs_file_list_scan_def_ptr},

{"xafs_scan",      MXS_XAF_STANDARD,  MXS_XAFS_SCAN,      MXR_SCAN,
				&mxs_xafs_scan_record_function_list,
				&mxs_xafs_scan_scan_function_list,
				NULL,
				&mxs_xafs_std_scan_num_record_fields,
				&mxs_xafs_std_scan_def_ptr},

{"mcs_qscan",      MXS_QUI_MCS,       MXS_QUICK_SCAN,     MXR_SCAN,
				&mxs_mcs_quick_scan_record_function_list,
				&mxs_mcs_quick_scan_scan_function_list,
				NULL,
				&mxs_mcs_quick_scan_num_record_fields,
				&mxs_mcs_quick_scan_def_ptr},

{"energy_mcs_qscan", MXS_QUI_ENERGY_MCS, MXS_QUICK_SCAN,  MXR_SCAN,
				&mxs_energy_mcs_quick_scan_record_function_list,
				&mxs_energy_mcs_quick_scan_scan_function_list,
				NULL,
				&mxs_energy_mcs_quick_scan_num_record_fields,
				&mxs_energy_mcs_quick_scan_def_ptr},

{"wedge_scan",     MXS_AD_WEDGE,      MXS_AREA_DETECTOR_SCAN, MXR_SCAN,
				&mxs_area_detector_scan_record_function_list,
				&mxs_area_detector_scan_scan_function_list,
				&mxs_wedge_area_detector_scan_function_list,
				&mxs_wedge_scan_num_record_fields,
				&mxs_wedge_scan_def_ptr},

  /* =================== Variable types ================== */

{"string",         MXV_INL_STRING,    MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_string_variable_num_record_fields,
				&mxv_inline_string_variable_def_ptr},
{"char",           MXV_INL_CHAR,      MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_char_variable_num_record_fields,
				&mxv_inline_char_variable_def_ptr},
{"uchar",          MXV_INL_UCHAR,     MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_uchar_variable_num_record_fields,
				&mxv_inline_uchar_variable_def_ptr},
{"short",          MXV_INL_SHORT,     MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_short_variable_num_record_fields,
				&mxv_inline_short_variable_def_ptr},
{"ushort",         MXV_INL_USHORT,    MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_ushort_variable_num_record_fields,
				&mxv_inline_ushort_variable_def_ptr},
{"bool",           MXV_INL_BOOL,     MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_bool_variable_num_record_fields,
				&mxv_inline_bool_variable_def_ptr},
{"long",           MXV_INL_LONG,      MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_long_variable_num_record_fields,
				&mxv_inline_long_variable_def_ptr},
{"ulong",          MXV_INL_ULONG,     MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_ulong_variable_num_record_fields,
				&mxv_inline_ulong_variable_def_ptr},
{"int64",          MXV_INL_INT64,     MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_int64_variable_num_record_fields,
				&mxv_inline_int64_variable_def_ptr},
{"uint64",         MXV_INL_UINT64,    MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_uint64_variable_num_record_fields,
				&mxv_inline_uint64_variable_def_ptr},
{"float",          MXV_INL_FLOAT,     MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_float_variable_num_record_fields,
				&mxv_inline_float_variable_def_ptr},
{"double",         MXV_INL_DOUBLE,    MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_double_variable_num_record_fields,
				&mxv_inline_double_variable_def_ptr},
{"record",         MXV_INL_RECORD,    MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_record_variable_num_record_fields,
				&mxv_inline_record_variable_def_ptr},

{"net_string",     MXV_NET_STRING,    MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_string_variable_num_record_fields,
				&mxv_network_string_variable_dptr},
{"net_char",       MXV_NET_CHAR,      MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_char_variable_num_record_fields,
				&mxv_network_char_variable_dptr},
{"net_uchar",      MXV_NET_UCHAR,     MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_uchar_variable_num_record_fields,
				&mxv_network_uchar_variable_dptr},
{"net_short",      MXV_NET_SHORT,     MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_short_variable_num_record_fields,
				&mxv_network_short_variable_dptr},
{"net_ushort",     MXV_NET_USHORT,    MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_ushort_variable_num_record_fields,
				&mxv_network_ushort_variable_dptr},
{"net_bool",       MXV_NET_BOOL,      MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_bool_variable_num_record_fields,
				&mxv_network_bool_variable_dptr},
{"net_long",       MXV_NET_LONG,      MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_long_variable_num_record_fields,
				&mxv_network_long_variable_dptr},
{"net_ulong",      MXV_NET_ULONG,     MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_ulong_variable_num_record_fields,
				&mxv_network_ulong_variable_dptr},
{"net_int64",      MXV_NET_INT64,     MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_int64_variable_num_record_fields,
				&mxv_network_int64_variable_dptr},
{"net_uint64",     MXV_NET_UINT64,    MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_uint64_variable_num_record_fields,
				&mxv_network_uint64_variable_dptr},
{"net_float",      MXV_NET_FLOAT,     MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_float_variable_num_record_fields,
				&mxv_network_float_variable_dptr},
{"net_double",     MXV_NET_DOUBLE,    MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_double_variable_num_record_fields,
				&mxv_network_double_variable_dptr},
{"net_record",     MXV_NET_RECORD,    MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				NULL, NULL,
				&mxv_network_record_variable_num_record_fields,
				&mxv_network_record_variable_dptr},

{"mathop",         MXV_CAL_MATHOP,    MXV_CALC,          MXR_VARIABLE,
				&mxv_mathop_record_function_list,
				&mxv_mathop_variable_function_list,
				NULL,
				&mxv_mathop_num_record_fields,
				&mxv_mathop_rfield_def_ptr},

{"position_select", MXV_CAL_POSITION_SELECT, MXV_CALC,   MXR_VARIABLE,
				&mxv_position_select_record_function_list,
				&mxv_position_select_variable_function_list,
				NULL,
				&mxv_position_select_num_record_fields,
				&mxv_position_select_rfield_def_ptr},

{"indirect_string", MXV_CAL_INDIRECT_STRING, MXV_CALC,   MXR_VARIABLE,
				&mxv_indirect_string_record_function_list,
				&mxv_indirect_string_variable_function_list,
				NULL,
				&mxv_indirect_string_num_record_fields,
				&mxv_indirect_string_rfield_def_ptr},

/*----*/

{"pmac_long",      MXV_PMA_LONG,      MXV_PMAC,          MXR_VARIABLE,
				&mxv_pmac_record_function_list,
				&mxv_pmac_variable_function_list,
				NULL,
				&mxv_pmac_long_num_record_fields,
				&mxv_pmac_long_rfield_def_ptr},

{"pmac_ulong",     MXV_PMA_ULONG,     MXV_PMAC,          MXR_VARIABLE,
				&mxv_pmac_record_function_list,
				&mxv_pmac_variable_function_list,
				NULL,
				&mxv_pmac_ulong_num_record_fields,
				&mxv_pmac_ulong_rfield_def_ptr},

{"pmac_double",    MXV_PMA_DOUBLE,    MXV_PMAC,          MXR_VARIABLE,
				&mxv_pmac_record_function_list,
				&mxv_pmac_variable_function_list,
				NULL,
				&mxv_pmac_double_num_record_fields,
				&mxv_pmac_double_rfield_def_ptr},

/*----*/

{"spec_string",    MXV_SPEC_STRING,   MXV_SPEC, MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_string_num_record_fields,
				&mxv_spec_string_rfield_def_ptr},

{"spec_char",      MXV_SPEC_CHAR,     MXV_SPEC, MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_char_num_record_fields,
				&mxv_spec_char_rfield_def_ptr},

{"spec_uchar",     MXV_SPEC_UCHAR,    MXV_SPEC,  MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_uchar_num_record_fields,
				&mxv_spec_uchar_rfield_def_ptr},

{"spec_short",      MXV_SPEC_SHORT,    MXV_SPEC,  MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_short_num_record_fields,
				&mxv_spec_short_rfield_def_ptr},

{"spec_ushort",     MXV_SPEC_USHORT,    MXV_SPEC, MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_ushort_num_record_fields,
				&mxv_spec_ushort_rfield_def_ptr},

{"spec_long",      MXV_SPEC_LONG,     MXV_SPEC,   MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_long_num_record_fields,
				&mxv_spec_long_rfield_def_ptr},

{"spec_ulong",     MXV_SPEC_ULONG,    MXV_SPEC,   MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_ulong_num_record_fields,
				&mxv_spec_ulong_rfield_def_ptr},

{"spec_float",    MXV_SPEC_FLOAT,   MXV_SPEC,     MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_float_num_record_fields,
				&mxv_spec_float_rfield_def_ptr},

{"spec_double",    MXV_SPEC_DOUBLE,   MXV_SPEC,   MXR_VARIABLE,
				&mxv_spec_property_record_function_list,
				&mxv_spec_property_variable_function_list,
				NULL,
				&mxv_spec_double_num_record_fields,
				&mxv_spec_double_rfield_def_ptr},

/*----*/

{"bluice_master",  MXV_BLUICE_MASTER, MXV_BLUICE, MXR_VARIABLE,
				&mxv_bluice_master_record_function_list,
				&mxv_bluice_master_variable_function_list,
				NULL,
				&mxv_bluice_master_num_record_fields,
				&mxv_bluice_master_rfield_def_ptr},

{"bluice_command", MXV_BLUICE_COMMAND, MXV_BLUICE, MXR_VARIABLE,
				&mxv_bluice_command_record_function_list,
				&mxv_bluice_command_variable_function_list,
				NULL,
				&mxv_bluice_command_num_record_fields,
				&mxv_bluice_command_rfield_def_ptr},

{"bluice_dcss_string", MXV_BLUICE_DCSS_STRING, MXV_BLUICE, MXR_VARIABLE,
				&mxv_bluice_string_record_function_list,
				&mxv_bluice_string_variable_function_list,
				NULL,
				&mxv_bluice_string_num_record_fields,
				&mxv_bluice_string_rfield_def_ptr},

{"bluice_dhs_string",  MXV_BLUICE_DHS_STRING, MXV_BLUICE, MXR_VARIABLE,
				&mxv_bluice_string_record_function_list,
				&mxv_bluice_string_variable_function_list,
				NULL,
				&mxv_bluice_string_num_record_fields,
				&mxv_bluice_string_rfield_def_ptr},

{"bluice_self_string",  MXV_BLUICE_SELF_STRING, MXV_BLUICE, MXR_VARIABLE,
				&mxv_bluice_string_record_function_list,
				&mxv_bluice_string_variable_function_list,
				NULL,
				&mxv_bluice_string_num_record_fields,
				&mxv_bluice_string_rfield_def_ptr},

{"bluice_dcss_operation", MXV_BLUICE_DCSS_OPERATION, MXV_BLUICE, MXR_VARIABLE,
				&mxv_bluice_operation_record_function_list,
				&mxv_bluice_operation_variable_function_list,
				NULL,
				&mxv_bluice_operation_num_record_fields,
				&mxv_bluice_operation_rfield_def_ptr},

{"bluice_dhs_operation",  MXV_BLUICE_DHS_OPERATION, MXV_BLUICE, MXR_VARIABLE,
				&mxv_bluice_operation_record_function_list,
				&mxv_bluice_operation_variable_function_list,
				NULL,
				&mxv_bluice_operation_num_record_fields,
				&mxv_bluice_operation_rfield_def_ptr},

{"bluice_self_operation", MXV_BLUICE_SELF_OPERATION, MXV_BLUICE, MXR_VARIABLE,
				&mxv_bluice_self_operation_record_function_list,
			&mxv_bluice_self_operation_variable_function_list,
				NULL,
				&mxv_bluice_self_operation_num_record_fields,
				&mxv_bluice_self_operation_rfield_def_ptr},

/*----*/

{"file_string",    MXV_FILE_STRING,   MXV_FILE, MXR_VARIABLE,
				&mxv_file_variable_record_function_list,
				&mxv_file_variable_variable_function_list,
				NULL,
				&mxv_file_string_variable_num_record_fields,
				&mxv_file_string_variable_dptr},

{"file_long",      MXV_FILE_LONG,     MXV_FILE,   MXR_VARIABLE,
				&mxv_file_variable_record_function_list,
				&mxv_file_variable_variable_function_list,
				NULL,
				&mxv_file_long_variable_num_record_fields,
				&mxv_file_long_variable_dptr},

{"file_ulong",     MXV_FILE_ULONG,    MXV_FILE,   MXR_VARIABLE,
				&mxv_file_variable_record_function_list,
				&mxv_file_variable_variable_function_list,
				NULL,
				&mxv_file_ulong_variable_num_record_fields,
				&mxv_file_ulong_variable_dptr},

{"file_double",    MXV_FILE_DOUBLE,   MXV_FILE,   MXR_VARIABLE,
				&mxv_file_variable_record_function_list,
				&mxv_file_variable_variable_function_list,
				NULL,
				&mxv_file_double_variable_num_record_fields,
				&mxv_file_double_variable_dptr},

/*----*/

{"field_string",     MXV_FIELD_STRING,    MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_string_variable_num_record_fields,
				&mxv_field_string_variable_dptr},
{"field_char",       MXV_FIELD_CHAR,      MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_char_variable_num_record_fields,
				&mxv_field_char_variable_dptr},
{"field_uchar",      MXV_FIELD_UCHAR,     MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_uchar_variable_num_record_fields,
				&mxv_field_uchar_variable_dptr},
{"field_short",      MXV_FIELD_SHORT,     MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_short_variable_num_record_fields,
				&mxv_field_short_variable_dptr},
{"field_ushort",     MXV_FIELD_USHORT,    MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_ushort_variable_num_record_fields,
				&mxv_field_ushort_variable_dptr},
{"field_bool",       MXV_FIELD_BOOL,      MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_bool_variable_num_record_fields,
				&mxv_field_bool_variable_dptr},
{"field_long",       MXV_FIELD_LONG,      MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_long_variable_num_record_fields,
				&mxv_field_long_variable_dptr},
{"field_ulong",      MXV_FIELD_ULONG,     MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_ulong_variable_num_record_fields,
				&mxv_field_ulong_variable_dptr},
{"field_int64",      MXV_FIELD_INT64,     MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_int64_variable_num_record_fields,
				&mxv_field_int64_variable_dptr},
{"field_uint64",     MXV_FIELD_UINT64,    MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_uint64_variable_num_record_fields,
				&mxv_field_uint64_variable_dptr},
{"field_float",      MXV_FIELD_FLOAT,     MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_float_variable_num_record_fields,
				&mxv_field_float_variable_dptr},
{"field_double",     MXV_FIELD_DOUBLE,    MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				&mxv_field_variable_variable_function_list,
				NULL,
				&mxv_field_double_variable_num_record_fields,
				&mxv_field_double_variable_dptr},
{"field_record",     MXV_FIELD_RECORD,    MXV_FIELD,         MXR_VARIABLE,
				&mxv_field_variable_record_function_list,
				NULL, NULL,
				&mxv_field_record_variable_num_record_fields,
				&mxv_field_record_variable_dptr},

  /* =================== Server types ================== */

#if HAVE_TCPIP
{"tcp_server",     MXN_NET_TCPIP,     MXN_NETWORK_SERVER, MXR_SERVER,
				&mxn_tcpip_server_record_function_list,
				NULL, NULL,
				&mxn_tcpip_server_num_record_fields,
				&mxn_tcpip_server_rfield_def_ptr},

/* The following is an older alternate name for the driver. */

{"tcpip_server",   MXN_NET_TCPIP,     MXN_NETWORK_SERVER, MXR_SERVER,
				&mxn_tcpip_server_record_function_list,
				NULL, NULL,
				&mxn_tcpip_server_num_record_fields,
				&mxn_tcpip_server_rfield_def_ptr},
#endif

#if HAVE_UNIX_DOMAIN_SOCKETS
{"unix_server",    MXN_NET_UNIX,      MXN_NETWORK_SERVER, MXR_SERVER,
				&mxn_unix_server_record_function_list,
				NULL, NULL,
				&mxn_unix_server_num_record_fields,
				&mxn_unix_server_rfield_def_ptr},
#endif

#if HAVE_TCPIP
{"spec_server",     MXN_SPEC_SERVER,  MXN_SPEC,           MXR_SERVER,
				&mxn_spec_server_record_function_list,
				NULL, NULL,
				&mxn_spec_server_num_record_fields,
				&mxn_spec_server_rfield_def_ptr},

{"bluice_dcss_server", MXN_BLUICE_DCSS_SERVER,  MXN_BLUICE,  MXR_SERVER,
				&mxn_bluice_dcss_server_record_function_list,
				NULL, NULL,
				&mxn_bluice_dcss_server_num_record_fields,
				&mxn_bluice_dcss_server_rfield_def_ptr},

{"bluice_dhs_server", MXN_BLUICE_DHS_SERVER,  MXN_BLUICE,  MXR_SERVER,
				&mxn_bluice_dhs_server_record_function_list,
				NULL, NULL,
				&mxn_bluice_dhs_server_num_record_fields,
				&mxn_bluice_dhs_server_rfield_def_ptr},

{"bluice_dhs_manager", MXN_BLUICE_DHS_MANAGER,  MXI_CONTROLLER,  MXR_SERVER,
				&mxn_bluice_dhs_manager_record_function_list,
				NULL, NULL,
				&mxn_bluice_dhs_manager_num_record_fields,
				&mxn_bluice_dhs_manager_rfield_def_ptr},
#endif

  /* =================== Operation driver types ================== */

{"network_operation",  MXO_NETWORK,   MXO_OPERATION,  MXR_OPERATION,
				&mxo_network_record_function_list,
				&mxo_network_operation_function_list,
				NULL,
				&mxo_network_num_record_fields,
				&mxo_network_rfield_def_ptr },

{"toast",        MXO_TOAST,   MXO_OPERATION,  MXR_OPERATION,
				&mxo_toast_record_function_list,
				&mxo_toast_operation_function_list,
				NULL,
				&mxo_toast_num_record_fields,
				&mxo_toast_rfield_def_ptr },

  /* =================== Special driver types ================== */

{"external_command", MXZ_PRG_EXTERNAL_COMMAND, MXZ_PROGRAM, MXR_SPECIAL,
				&mxz_external_command_record_function_list,
				NULL, NULL,
				&mxz_external_command_num_record_fields,
				&mxz_external_command_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

