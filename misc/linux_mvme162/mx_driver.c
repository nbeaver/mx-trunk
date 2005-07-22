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
 * Copyright 1999-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_osdef.h"
#include "mx_types.h"

#include "mx_driver.h"

/* -- Include header files that define MX_XXX_FUNCTION_LIST structures. -- */

#include "mx_rs232.h"
#include "mx_gpib.h"
#include "mx_camac.h"
#include "mx_generic.h"
#include "mx_portio.h"
#include "mx_vme.h"

#if HAVE_TCPIP

#include "mx_socket.h"

#endif /* HAVE_TCPIP */

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
#include "mx_ccd.h"

#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "mx_scan_list.h"
#include "mx_scan_xafs.h"
#include "mx_scan_quick.h"

#include "mx_variable.h"
#include "mx_vinline.h"
#include "mx_vnet.h"

#include "mx_net.h"

#include "mx_list_head.h"

#include "mx_dead_reckoning.h"

/* Include the header files for all of the interfaces and devices. */

#include "i_soft_rs232.h"
#include "i_network_rs232.h"
#include "i_tty.h"

#if HAVE_TCPIP
#include "i_tcp232.h"
#endif

#include "i_k500serial.h"
#include "i_lpt.h"
#include "i_pmac.h"
#include "i_compumotor.h"
#include "i_hsc1.h"
#include "i_vme58.h"
#include "i_vsc16.h"
#include "i_linux_portio.h"
#include "i_linux_iopl.h"
#include "i_mmap_vme.h"
#include "i_sis3807.h"
#include "i_aps_adcmod2.h"

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

#include "d_soft_motor.h"
#include "d_pmac.h"
#include "d_pmac_cs_axis.h"
#include "d_compumotor.h"
#include "d_network_motor.h"
#include "d_hsc1.h"
#include "d_stp100.h"
#include "d_vme58.h"
#include "d_dac_motor.h"
#include "d_disabled_motor.h"

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
#include "d_theta_2theta.h"
#include "d_qmotor.h"
#include "d_tangent_arm.h"

#include "d_soft_scaler.h"
#include "d_network_scaler.h"
#include "d_scaler_function.h"
#include "d_vsc16_scaler.h"
#include "d_vsc16_timer.h"
#include "d_soft_timer.h"
#include "d_network_timer.h"
#include "d_timer_fanout.h"
#include "d_network_relay.h"
#include "d_generic_relay.h"
#include "d_blind_relay.h"
#include "d_soft_amplifier.h"
#include "d_net_amplifier.h"
#include "d_sr570.h"
#include "d_udt_tramp.h"

#include "d_aps_adcmod2_amplifier.h"
#include "d_aps_adcmod2_ainput.h"

#include "d_soft_mca.h"
#include "d_network_mca.h"
#include "d_sis3801.h"
#include "d_mca_timer.h"

#include "d_network_pulser.h"
#include "d_sis3807.h"
#include "d_sis3801_pulser.h"

#include "d_soft_sca.h"
#include "d_network_sca.h"

#include "d_network_ccd.h"

#include "s_input.h"
#include "s_motor.h"
#include "s_theta_2theta.h"
#include "s_slit.h"
#include "s_pseudomotor.h"
#include "sl_file.h"
#include "sxafs_std.h"
#include "sq_mcs.h"

#include "d_soft_mcs.h"
#include "d_network_mcs.h"
#include "d_scaler_function_mcs.h"
#include "d_mcs_scaler.h"
#include "d_mcs_timer.h"
#include "d_mcs_mce.h"
#include "d_mcs_time_mce.h"

#include "d_network_mce.h"
#include "d_pmac_mce.h"

#include "v_mathop.h"

#include "v_position_select.h"

#include "v_pmac.h"

#if HAVE_TCPIP

#include "n_tcpip.h"

#endif /* HAVE_TCPIP */

/* -- Define the list of record types. -- */

MX_DRIVER *mx_list_of_types[] = {
	mx_superclass_list,
	mx_class_list,
	mx_type_list,
	NULL
};

/* -- Define lists that relate types to classes and to function lists. -- */

  /****************** Record Superclasses ********************/

MX_DRIVER mx_superclass_list[] = {
{"list_head_sclass", 0, 0, MXR_LIST_HEAD,     NULL, NULL, NULL, NULL, NULL},
{"interface",        0, 0, MXR_INTERFACE,     NULL, NULL, NULL, NULL, NULL},
{"device",           0, 0, MXR_DEVICE,        NULL, NULL, NULL, NULL, NULL},
{"scan",             0, 0, MXR_SCAN,          NULL, NULL, NULL, NULL, NULL},
{"variable",         0, 0, MXR_VARIABLE,      NULL, NULL, NULL, NULL, NULL},
{"server",           0, 0, MXR_SERVER,        NULL, NULL, NULL, NULL, NULL},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

  /********************* Record Classes **********************/

MX_DRIVER mx_class_list[] = {

{"list_head_class", 0, MXL_LIST_HEAD,      MXR_LIST_HEAD,
				NULL, NULL, NULL, NULL, NULL},

  /* ================== Interface classes ================== */

{"rs232",          0, MXI_RS232,          MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"gpib",           0, MXI_GPIB,           MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"camac",          0, MXI_CAMAC,          MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"generic",        0, MXI_GENERIC,        MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"portio",         0, MXI_PORTIO,         MXR_INTERFACE,
				NULL, NULL, NULL, NULL, NULL},
{"vme",            0, MXI_VME,            MXR_INTERFACE,
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
{"ccd",            0, MXC_CCD,            MXR_DEVICE,
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

  /* ================== Server classes ================== */

{"network",        0, MXN_NETWORK_SERVER, MXR_SERVER,
				NULL, NULL, NULL, NULL, NULL},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

  /********************** Record Types **********************/

MX_DRIVER mx_type_list[] = {

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

#if defined(OS_UNIX) || defined(OS_CYGWIN)
{"tty",            MXI_232_TTY,      MXI_RS232,          MXR_INTERFACE,
				&mxi_tty_record_function_list,
				NULL,
				&mxi_tty_rs232_function_list,
				&mxi_tty_num_record_fields,
				&mxi_tty_rfield_def_ptr},
#endif /* OS_UNIX */

#if HAVE_TCPIP

{"tcp232",         MXI_232_TCP232,   MXI_RS232,          MXR_INTERFACE,
				&mxi_tcp232_record_function_list,
				NULL,
				&mxi_tcp232_rs232_function_list,
				&mxi_tcp232_num_record_fields,
				&mxi_tcp232_rfield_def_ptr},
#endif /* HAVE_TCPIP */

#if HAVE_NI488
{"ni488",          MXI_GPIB_NI488,   MXI_GPIB,           MXR_INTERFACE,
				&mxi_ni488_record_function_list,
				NULL,
				&mxi_ni488_gpib_function_list,
				&mxi_ni488_num_record_fields,
				&mxi_ni488_rfield_def_ptr},
#endif /* HAVE_NI488 */

#if HAVE_LLP_GPIB
{"llp_gpib",       MXI_GPIB_LLP,     MXI_GPIB,           MXR_INTERFACE,
				&mxi_ni488_record_function_list,
				NULL,
				&mxi_ni488_gpib_function_list,
				&mxi_ni488_num_record_fields,
				&mxi_ni488_rfield_def_ptr},
#endif /* HAVE_LLP_GPIB */

{"k500serial",     MXI_GPIB_K500SERIAL, MXI_GPIB,         MXR_INTERFACE,
				&mxi_k500serial_record_function_list,
				NULL,
				&mxi_k500serial_gpib_function_list,
				&mxi_k500serial_num_record_fields,
				&mxi_k500serial_rfield_def_ptr},

  /* === */

{"pmac",           MXI_GEN_PMAC,      MXI_GENERIC,        MXR_INTERFACE,
                                &mxi_pmac_record_function_list,
                                NULL,
                                NULL,
				&mxi_pmac_num_record_fields,
				&mxi_pmac_rfield_def_ptr},

{"compumotor_int", MXI_GEN_COMPUMOTOR, MXI_GENERIC,       MXR_INTERFACE,
				&mxi_compumotor_record_function_list,
				NULL,
				NULL,
				&mxi_compumotor_num_record_fields,
				&mxi_compumotor_rfield_def_ptr},

{"lpt",            MXI_GEN_LPT,       MXI_GENERIC,       MXR_INTERFACE,
				&mxi_lpt_record_function_list,
				NULL,
				NULL,
				&mxi_lpt_num_record_fields,
				&mxi_lpt_rfield_def_ptr},

{"hsc1",            MXI_GEN_HSC1,         MXI_GENERIC,       MXR_INTERFACE,
				&mxi_hsc1_record_function_list,
				NULL,
				&mxi_hsc1_generic_function_list,
				&mxi_hsc1_num_record_fields,
				&mxi_hsc1_rfield_def_ptr},

{"vme58",           MXI_GEN_VME58,        MXI_GENERIC,       MXR_INTERFACE,
				&mxi_vme58_record_function_list,
				NULL,
				NULL,
				&mxi_vme58_num_record_fields,
				&mxi_vme58_rfield_def_ptr},

#if HAVE_VME58_ESRF

{"vme58_esrf",      MXI_GEN_VME58_ESRF,   MXI_GENERIC,       MXR_INTERFACE,
				&mxi_vme58_record_function_list,
				NULL,
				NULL,
				&mxi_vme58_esrf_num_record_fields,
				&mxi_vme58_esrf_rfield_def_ptr},

#endif /* HAVE_VME58_ESRF */

{"vsc16",           MXI_GEN_VSC16,        MXI_GENERIC,       MXR_INTERFACE,
				&mxi_vsc16_record_function_list,
				NULL,
				NULL,
				&mxi_vsc16_num_record_fields,
				&mxi_vsc16_rfield_def_ptr},

{"sis3807",        MXI_GEN_SIS3807,    MXI_GENERIC,       MXR_INTERFACE,
				&mxi_sis3807_record_function_list,
				NULL,
				NULL,
				&mxi_sis3807_num_record_fields,
				&mxi_sis3807_rfield_def_ptr},

{"aps_adcmod2",    MXI_GEN_APS_ADCMOD2,    MXI_GENERIC,       MXR_INTERFACE,
				&mxi_aps_adcmod2_record_function_list,
				NULL,
				NULL,
				&mxi_aps_adcmod2_num_record_fields,
				&mxi_aps_adcmod2_rfield_def_ptr},

#ifdef OS_LINUX
#if HAVE_LINUX_PORTIO

{"linux_portio",   MXI_PIO_PORTIO,    MXI_PORTIO,         MXR_INTERFACE,
				&mxi_linux_portio_record_function_list,
				NULL,
				&mxi_linux_portio_portio_function_list,
				&mxi_linux_portio_num_record_fields,
				&mxi_linux_portio_rfield_def_ptr},
#endif /* HAVE_LINUX_PORTIO */

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

#if defined(OS_LINUX)

{"mmap_vme",       MXI_VME_MMAP,      MXI_VME,        MXR_INTERFACE,
                                &mxi_mmap_vme_record_function_list,
                                NULL,
                                &mxi_mmap_vme_vme_function_list,
                                &mxi_mmap_vme_num_record_fields,
                                &mxi_mmap_vme_rfield_def_ptr},

#endif /* OS_LINUX */

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

{"soft_motor",     MXT_MTR_SOFTWARE,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_soft_motor_record_function_list,
				NULL,
				&mxd_soft_motor_motor_function_list,
				&mxd_soft_motor_num_record_fields,
				&mxd_soft_motor_rfield_def_ptr},
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

{"dac_motor",      MXT_MTR_DAC_MOTOR, MXC_MOTOR,          MXR_DEVICE,
				&mxd_dac_motor_record_function_list,
				NULL,
				&mxd_dac_motor_motor_function_list,
				&mxd_dac_motor_num_record_fields,
				&mxd_dac_motor_rfield_def_ptr},

{"disabled_motor", MXT_MTR_DISABLED,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_disabled_motor_record_function_list,
				NULL,
				&mxd_disabled_motor_motor_function_list,
				&mxd_disabled_motor_num_record_fields,
				&mxd_disabled_motor_rfield_def_ptr},

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

{"scaler_function", MXT_SCL_SCALER_FUNCTION, MXC_SCALER,       MXR_DEVICE,
				&mxd_scaler_function_record_function_list,
				NULL,
				&mxd_scaler_function_scaler_function_list,
				&mxd_scaler_function_num_record_fields,
				&mxd_scaler_function_rfield_def_ptr},

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

{"mcs_scaler",     MXT_SCL_MCS,       MXC_SCALER,         MXR_DEVICE,
				&mxd_mcs_scaler_record_function_list,
				NULL,
				&mxd_mcs_scaler_scaler_function_list,
				&mxd_mcs_scaler_num_record_fields,
				&mxd_mcs_scaler_rfield_def_ptr},

{"mcs_encoder",    MXT_MCE_MCS,       MXC_MULTICHANNEL_ENCODER, MXR_DEVICE,
				&mxd_mcs_encoder_record_function_list,
				NULL,
				&mxd_mcs_encoder_mce_function_list,
				&mxd_mcs_encoder_num_record_fields,
				&mxd_mcs_encoder_rfield_def_ptr},

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

{"soft_amplifier", MXT_AMP_SOFTWARE,  MXC_AMPLIFIER,      MXR_DEVICE,
				&mxd_soft_amplifier_record_function_list,
				NULL,
				&mxd_soft_amplifier_amplifier_function_list,
				&mxd_soft_amplifier_num_record_fields,
				&mxd_soft_amplifier_rfield_def_ptr},

{"net_amplifier",  MXT_AMP_NETWORK,   MXC_AMPLIFIER,      MXR_DEVICE,
				&mxd_network_amplifier_record_function_list,
				NULL,
				&mxd_network_amplifier_amplifier_function_list,
				&mxd_network_amplifier_num_record_fields,
				&mxd_network_amplifier_rfield_def_ptr},

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

{"soft_mca",       MXT_MCA_SOFTWARE,  MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_soft_mca_record_function_list,
				NULL,
				&mxd_soft_mca_mca_function_list,
				&mxd_soft_mca_num_record_fields,
				&mxd_soft_mca_rfield_def_ptr},

{"network_mca",    MXT_MCA_NETWORK,   MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_network_mca_record_function_list,
				NULL,
				&mxd_network_mca_mca_function_list,
				&mxd_network_mca_num_record_fields,
				&mxd_network_mca_rfield_def_ptr},

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

{"network_ccd",    MXT_CCD_NETWORK,   MXC_CCD,            MXR_DEVICE,
				&mxd_network_ccd_record_function_list,
				NULL,
				&mxd_network_ccd_ccd_function_list,
				&mxd_network_ccd_num_record_fields,
				&mxd_network_ccd_rfield_def_ptr},

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
{"int",            MXV_INL_INT,       MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_int_variable_num_record_fields,
				&mxv_inline_int_variable_def_ptr},
{"uint",           MXV_INL_UINT,      MXV_INLINE,         MXR_VARIABLE,
				&mxv_inline_variable_record_function_list,
				NULL, NULL,
				&mxv_inline_uint_variable_num_record_fields,
				&mxv_inline_uint_variable_def_ptr},
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
{"net_int",        MXV_NET_INT,       MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_int_variable_num_record_fields,
				&mxv_network_int_variable_dptr},
{"net_uint",       MXV_NET_UINT,      MXV_NETWORK,         MXR_VARIABLE,
				&mxv_network_variable_record_function_list,
				&mxv_network_variable_variable_function_list,
				NULL,
				&mxv_network_uint_variable_num_record_fields,
				&mxv_network_uint_variable_dptr},
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

  /* =================== Server types ================== */

#if HAVE_TCPIP
{"tcpip_server",   MXN_NET_TCPIP,     MXN_NETWORK_SERVER, MXR_SERVER,
				&mxn_tcpip_server_record_function_list,
				NULL, NULL,
				&mxn_tcpip_server_num_record_fields,
				&mxn_tcpip_server_rfield_def_ptr},
#endif

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

