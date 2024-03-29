#
# This makefile is normally intended to be invoked by the makefile in
# the top level directory.  If you want to invoke it directly for some
# reason, use commands like
#
# make MX_ARCH=linux mx_clean
# make MX_ARCH=linux mx_depend
# make MX_ARCH=linux
# make MX_ARCH=linux MX_INSTALL_DIR=/opt/mx mx_install
#

#
# List all of the source code files used to build libMx.
#

MX_CORE_SRCS = mx_amplifier.c mx_analog_input.c mx_analog_output.c \
	mx_area_detector.c mx_area_detector_correction.c \
	mx_area_detector_rdi.c \
	mx_array.c mx_atomic.c mx_autoscale.c mx_bit.c mx_bluice.c mx_boot.c \
	mx_callback.c mx_camac.c mx_camera_link.c mx_cfn.c mx_circular_buffer.c\
	mx_clock.c mx_clock_tick.c mx_condition_variable.c \
	mx_console.c mx_coprocess.c mx_cpu.c mx_cpu_arch.c \
	mx_datafile.c mx_dead_reckoning.c mx_debug.c mx_debugger.c \
	mx_dictionary.c \
	mx_digital_input.c mx_digital_output.c mx_dirent.c \
	mx_driver_tables.c mx_dynamic_library.c \
	mx_encoder.c mx_error.c mx_export.c \
	mx_field.c mx_fvarargs.c \
	mx_generic.c mx_gpib.c mx_handle.c mx_hash_table.c mx_heap.c \
	mx_hrt.c mx_hrt_debug.c \
	mx_image.c mx_image_noir.c \
	mx_info.c mx_interval_timer.c mx_io.c mx_json.c mx_key.c \
	mx_log.c mx_list.c mx_list_head.c \
	mx_malloc.c mx_math.c mx_mca.c mx_mcai.c mx_mce.c mx_mcs.c \
	mx_measurement.c mx_memory_process.c mx_memory_system.c \
	mx_mfault.c mx_modbus.c mx_module.c mx_motor.c mx_mpermit.c mx_multi.c \
	mx_mutex.c mx_net.c mx_netdb.c mx_net_interface.c mx_net_socket.c \
	mx_operation.c mx_os_version.c \
	mx_pipe.c mx_plot.c mx_portio.c mx_process.c \
	mx_ptz.c mx_pulse_generator.c \
	mx_record.c mx_relay.c mx_rs232.c \
	mx_sample_changer.c mx_sca.c mx_scaler.c \
	mx_scan.c mx_scan_area_detector.c mx_scan_linear.c mx_scan_list.c \
	mx_scan_mca.c mx_scan_quick.c mx_scan_xafs.c \
	mx_security.c mx_semaphore.c mx_server_connect.c mx_service.c \
	mx_signal.c mx_signal_alloc.c mx_sleep.c mx_socket.c mx_spawn.c \
	mx_spec.c mx_stack.c mx_syslog.c \
	mx_table.c mx_test.c mx_thread.c mx_time.c mx_timer.c \
	mx_umx.c mx_update.c mx_usb.c mx_user_interrupt.c \
	mx_url.c mx_util.c mx_util_cfaqs.c mx_util_file.c mx_util_poison.c \
	mx_variable.c mx_version.c mx_vfield.c mx_vfile.c \
	mx_video_input.c mx_vinline.c \
	mx_virtual_timer.c mx_vm_alloc.c mx_vme.c mx_vnet.c mx_warning.c \
	mx_waveform_input.c mx_waveform_output.c \
	pr_amplifier.c pr_analog_input.c pr_analog_output.c \
	pr_area_detector.c pr_autoscale.c pr_clock.c \
	pr_digital_input.c pr_digital_output.c pr_gpib.c \
	pr_list_head.c pr_mca.c pr_mcai.c pr_mce.c pr_mcs.c \
	pr_motor.c pr_operation.c pr_ptz.c pr_pulser.c pr_record.c \
	pr_relay.c pr_rs232.c pr_sample_changer.c pr_sca.c pr_scaler.c \
	pr_timer.c pr_variable.c pr_video_input.c pr_vme.c \
	pr_waveform_input.c pr_waveform_output.c \
	fh_autoscale.c fh_simple.c \
	f_child.c f_custom.c f_none.c f_sff.c f_text.c f_xafs.c \
	m_count.c m_k_power_law.c m_none.c m_pulse_period.c m_time.c \
	p_child.c p_custom.c p_gnuplot.c p_gnuplot_xafs.c p_none.c \
	ph_aps_topup.c ph_simple.c \
	s_input.c s_motor.c s_pseudomotor.c s_slit.c s_theta_2theta.c \
	sa_wedge.c sl_file.c sxafs_kpl.c sxafs_std.c sq_energy_mcs.c sq_mcs.c \
	z_dictionary.c z_external_command.c

MX_DRIVER_SRCS = d_6821.c d_8255.c d_adc_table.c d_adsc_two_theta.c \
	d_aframe_detector_motor.c d_ainput_as_dinput.c \
	d_als_dewar_positioner.c \
	d_am9513_motor.c d_am9513_scaler.c d_am9513_timer.c \
	d_amptek_dp4_mca.c d_amptek_dp5_mca.c \
	d_aoutput_as_doutput.c \
	d_aps_adcmod2_ainput.c d_aps_adcmod2_amplifier.c \
	d_area_detector_timer.c d_arduino_ms3_dc_motor.c \
	d_auto_amplifier.c d_auto_filter.c \
	d_auto_filter_amplifier.c d_auto_net.c d_auto_scaler.c \
	d_bipolar_relay.c d_bit.c d_bkprecision_912x_aio.c \
	d_bkprecision_912x_dio.c d_bkprecision_912x_wvout.c d_blind_relay.c \
	d_bluice_area_detector.c d_bluice_ion_chamber.c d_bluice_motor.c \
	d_bluice_shutter.c d_bluice_timer.c \
	d_cm17a_doutput.c d_clock_timer.c \
	d_compumotor.c d_compumotor_dio.c d_compumotor_linear.c \
	d_coordinated_angle.c \
	d_cryostream600_motor.c d_cryostream600_status.c \
	d_cubic_spline_motor.c \
	d_cyberstar_x1000.c d_cyberstar_x1000_aout.c d_cxtilt02.c \
	d_d8.c d_dac_motor.c d_databox_mce.c d_databox_mcs.c d_databox_motor.c \
	d_databox_scaler.c d_databox_timer.c \
	d_dcc_cab.c d_delta_motor.c d_dg645_pulser.c \
	d_digital_fanin.c d_digital_fanout.c \
	d_disabled_motor.c d_doutput_pulser.c d_dtu_stage.c d_e500.c \
	d_e662.c d_eiger.c d_elapsed_time.c d_energy.c d_fg_scaler.c \
	d_field_motor.c d_field_relay.c d_file_dinput.c d_file_vinput.c \
	d_flowbus_valve.c \
	d_gain_tracking_scaler.c d_gated_backlash.c d_generic_relay.c \
	d_gittelsohn_pulser.c d_gm10_scaler.c d_gm10_timer.c \
	d_hitachi_kp_d20.c d_hsc1.c d_hrt_clock.c d_hrt_motor.c \
	d_i404_amp.c d_i404_mcai.c d_icplus.c d_icplus_aio.c d_icplus_dio.c \
	d_ilm_ainput.c d_ilm_sample_rate.c d_ilm_status.c \
	d_interval_timer.c d_iseries_aio.c d_iseries_dio.c \
	d_itc503_ainput.c d_itc503_aoutput.c d_itc503_doutput.c \
	d_itc503_motor.c \
	d_keithley199_ainput.c \
	d_keithley2000_ainput.c \
	d_keithley2400_ainput.c d_keithley2400_amp.c \
	d_keithley2400_aoutput.c d_keithley2400_doutput.c \
	d_keithley2600_ainput.c d_keithley2600_aoutput.c \
	d_keithley2700_ainput.c d_keithley2700_amp.c d_keithley428.c \
	d_keyboard_dinput.c \
	d_kohzu_sc.c d_ks3063.c d_ks3112.c d_ks3512.c d_ks3640.c \
	d_lakeshore330_motor.c d_limited_move.c d_linear_function.c \
	d_linkam_t9x_motor.c d_linkam_t9x_pump.c d_linkam_t9x_temp.c \
	d_linux_parport.c d_lpt.c \
	d_mar345.c d_marccd.c d_marccd_server_socket.c d_marccd_shutter.c \
	d_mardtb_motor.c d_mardtb_shutter.c d_mardtb_status.c \
	d_mca_alt_time.c d_mca_channel.c d_mca_roi_integral.c \
	d_mca_timer.c d_mca_value.c d_mca_weighted_sum.c d_mcai_function.c \
	d_mclennan.c d_mclennan_aio.c d_mclennan_dio.c \
	d_mcs_mce.c d_mcs_time_mce.c d_mcs_scaler.c d_mcs_timer.c \
	d_mcu2.c d_mdrive.c d_mdrive_aio.c d_mdrive_dio.c \
	d_merlin_medipix.c d_mlfsom.c d_mmc32.c d_modbus_aio.c d_modbus_dio.c \
	d_monochromator.c d_monte_carlo_mca.c d_motor_relay.c \
	d_net_amplifier.c d_net_sample_changer.c \
	d_network_ainput.c d_network_aoutput.c d_network_area_detector.c \
	d_network_dinput.c d_network_doutput.c d_network_mca.c \
	d_network_mcai.c d_network_mce.c d_network_mcs.c d_network_motor.c \
	d_network_ptz.c d_network_pulser.c d_network_relay.c d_network_sca.c \
	d_network_scaler.c d_network_timer.c d_network_vinput.c \
	d_network_wvin.c d_network_wvout.c d_newport.c \
	d_numato_gpio_ainput.c d_numato_gpio_dinput.c d_numato_gpio_doutput.c \
	d_ortec974_scaler.c d_ortec974_timer.c d_os_clock.c d_p6000a.c \
	d_panasonic_kx_dp702.c d_panther.c d_pcstep.c \
	d_pdi40.c d_pdi45_aio.c d_pdi45_dio.c d_pdi45_pulser.c \
	d_pdi45_scaler.c d_pdi45_timer.c \
	d_pfcu.c d_pfcu_filter_summary.c d_pfcu_shutter_timer.c \
	d_phidget_old_stepper.c d_pilatus.c \
	d_picomotor.c d_picomotor_aio.c d_picomotor_dio.c d_pm304.c \
	d_pmac.c d_pmac_aio.c d_pmac_dio.c d_pmac_mce.c d_pmac_cs_axis.c \
	d_polynomial_motor.c d_portio_dio.c \
	d_ptz_motor.c d_pulsed_relay.c \
	d_qbpm_mcai.c d_qmotor.c d_qs450.c \
	d_relay_as_doutput.c d_relay_pulser.c \
	d_roentec_rcl_mca.c d_rtc018.c \
	d_scaler_function.c d_scaler_function_mcs.c \
	d_scipe_aio.c d_scipe_amplifier.c d_scipe_dio.c d_scipe_motor.c \
	d_scipe_scaler.c d_scipe_timer.c d_sd_m107.c d_segmented_move.c \
	d_sercat_als_robot.c \
	d_si9650_motor.c d_si9650_status.c d_sim960.c d_sim980.c d_sim983.c \
	d_sis3801.c d_sis3801_pulser.c d_sis3807.c d_sis3820.c \
	d_slit_motor.c \
	d_smartmotor.c d_smartmotor_aio.c d_smartmotor_dio.c \
	d_smc24.c d_soft_amplifier.c d_soft_ainput.c d_soft_aoutput.c \
	d_soft_area_detector.c d_soft_dinput.c d_soft_doutput.c \
	d_soft_mca.c d_soft_mce.c d_soft_mcs.c d_soft_motor.c d_soft_ptz.c \
	d_soft_pulser.c \
	d_soft_sample_changer.c d_soft_sca.c d_soft_scaler.c d_soft_timer.c \
	d_soft_vinput.c d_sony_snc.c d_sony_visca_ptz.c \
	d_spec_motor.c d_spec_scaler.c d_spec_timer.c \
	d_spellman_df3_aio.c d_spellman_df3_dio.c \
	d_sr570.c d_sr630_ainput.c d_sr630_aoutput.c d_stp100.c \
	d_synaccess_netbooter_ainput.c d_synaccess_netbooter_relay.c \
	d_table_motor.c d_tangent_arm.c d_theta_2theta.c d_timer_fanout.c \
	d_tpg262_pressure.c d_tracker_aio.c d_tracker_dio.c \
	d_trans_motor.c d_udt_tramp.c d_uglide.c \
	d_umx_ainput.c d_umx_aoutput.c d_umx_dinput.c d_umx_doutput.c \
	d_umx_pulser.c d_umx_rc_servo.c \
	d_vme58.c d_vme_dio.c d_vp9000.c d_vsc16_scaler.c d_vsc16_timer.c \
	d_wago750_modbus_aout.c d_wago750_modbus_dout.c \
	d_wavelength.c d_wavenumber.c d_wti_nps_relay.c d_xafs_wavenumber.c \
	i_6821.c i_8255.c i_am9513.c \
	i_amptek_dp5.c i_aps_adcmod2.c \
	i_bkprecision_912x.c i_camera_link_api.c i_camera_link_rs232.c \
	i_cm17a.c i_compumotor.c i_cxtilt02.c i_d8.c i_databox.c i_dcc_base.c \
	i_dg645.c i_dos_com.c i_dos_portio.c i_dsp6001.c \
	i_file_rs232.c i_flowbus.c i_flowbus_format.c i_fossil.c i_hsc1.c \
	i_i404.c i_ilm.c i_iseries.c i_isobus.c i_itc503.c i_k500serial.c \
	i_keithley.c i_keithley199.c i_keithley2000.c i_keithley2400.c \
	i_keithley2600.c i_keithley2700.c \
	i_kohzu_sc.c i_ks3344.c i_linkam_t9x.c \
	i_linux_iopl.c i_linux_parport.c i_linux_usbtmc.c i_lpt.c \
	i_mardtb.c i_micro488ex.c i_mmap_vme.c \
	i_modbus_rtu.c i_modbus_tcp.c \
	i_network_gpib.c i_network_rs232.c i_newport.c \
	i_numato_gpio.c i_ortec974.c \
	i_panasonic_kx_dp702.c i_pcstep.c i_pdi40.c i_pdi45.c \
	i_pfcu.c i_phidget_old_stepper.c i_picomotor.c \
	i_pmac.c i_prologix.c i_rtems_vme.c \
	i_scipe.c i_sim900.c i_sim900_port.c i_sis3807.c \
	i_soft_camac.c i_soft_camera_link.c i_soft_rs232.c \
	i_sony_visca.c i_spec_command.c i_spellman_df3.c \
	i_sr630.c i_synaccess_netbooter.c \
	i_tcp232.c i_telnet.c i_tpg262.c i_tty.c i_uglide.c \
	i_vme58.c i_vms_terminal.c i_vp9000.c i_vsc16.c \
	i_vxworks_rs232.c i_vxworks_vme.c \
	i_wago750_serial.c i_win32_com.c i_wti_nps.c \
	n_bluice_dcss.c n_bluice_dhs.c n_bluice_dhs_manager.c \
	n_http.c n_spec.c n_tcpip.c n_umx.c n_unix.c \
	o_network.c o_toast.c \
	v_bluice_command.c v_bluice_master.c \
	v_bluice_operation.c v_bluice_self_operation.c v_bluice_string.c \
	v_fix_regions.c v_flowbus.c v_flowbus_capacity.c \
	v_indirect_string.c v_mathop.c \
	v_pmac.c v_position_select.c v_spec.c

MX_LIB_SRCS = mx_driver.c $(MX_CORE_SRCS) $(MX_DRIVER_SRCS)

#
#--------------------------------------------------------------------------
#
# The default makefile target must come before the includes since they
# contain makefile targets too.
#

all: mx_build

#
#--------------------------------------------------------------------------
#
# Definitions specific to a particular architecture are included here.
#

include Makefile.version

include Makehead.$(MX_ARCH)

EXTRA_FLAGS=$(DEFINE)__MX_LIBRARY__ $(EXPORT_FLAGS)

#
# Override the value of $(INCLUDES) here.  Under most circumstances,
# the variable LIBMX_INCLUDES will not be defined, so do not be surprised
# if you cannot find it for your platform.
#

INCLUDES = $(LIBMX_INCLUDES)

#
#--------------------------------------------------------------------------
#
# Makefile targets.
#

mx_build: mx_touch $(MX_LIBRARY_NAME)

mx_clean:
	-$(RM) *.$(OBJ)
	-$(RM) *.bak
	-$(RM) *.dbg
	-$(RM) *.exe
	-$(RM) *.ilk
	-$(RM) *.manifest
	-$(RM) *.map
	-$(RM) *.pdb
	-$(RM) *.tds
	-$(RM) core*
	-$(MX_LIBRARY_DELETE)
	-$(RM) $(MX_PRIVATE_VERSION_NAME)

mx_distclean: mx_clean
	-$(MAKEDEPEND_CLEAN)
	-$(RM) mx_private_version.h
	-$(RM) mx_private_revision.h

mx_depend: mx_touch
	$(MAKEDEPEND)

mx_install: library_install

mx_touch:
	$(TOUCH) mx_private_version.h
	$(TOUCH) mx_private_revision.h

#
# Include standard header file dependencies here.  If 'Makefile.depend' does
# not already exist for some reason, then just create a zero length file
# called 'Makefile.depend' and then run "make mx_depend" to create the real 
# 'Makefile.depend' file.
#

include Makefile.depend

#
# Now, list targets with non-standard rules or dependencies.
#

i_camera_link_api.$(OBJ): i_camera_link_api.c
	$(COMPILE) $(CFLAGS) i_camera_link_api.c

i_linux_iopl.$(OBJ): i_linux_iopl.c
	$(COMPILE) $(CFLAGS) $(LINUX_IOPL_FLAGS) i_linux_iopl.c

i_pmac.$(OBJ): i_pmac.c i_pmac.h
	$(COMPILE) $(CFLAGS) $(POWERPMAC_INCLUDES) i_pmac.c

mx_cfn.$(OBJ): mx_cfn.c
	$(COMPILE) $(CFLAGS) $(CFLAGS_MX_CFN) mx_cfn.c

mx_interval_timer.$(OBJ): mx_interval_timer.c
	$(COMPILE) $(CFLAGS) mx_interval_timer.c

#---

mx_version.$(OBJ): Makefile.version mx_version.c mx_version.h

mx_version.h: mx_private_version.h

