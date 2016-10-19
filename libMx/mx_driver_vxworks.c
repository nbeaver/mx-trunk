/*
 * Name:    mx_driver_vxworks.c
 *
 * Purpose: The VxWorks systems I have are memory constrained, so this
 *          file only includes a small subset of the available MX drivers
 *          to reduce memory consumption by driver code.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2012, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_stdint.h"

#include "mx_driver.h"
#include "mx_interval_timer.h"

/* -- Include header files that define MX_XXX_FUNCTION_LIST structures. -- */

#include "mx_rs232.h"
#include "mx_vme.h"

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_relay.h"
#include "mx_pulse_generator.h"
#include "mx_mcs.h"

#include "mx_variable.h"
#include "mx_vinline.h"

#include "mx_bluice.h"

#include "mx_list_head.h"

#include "mx_dead_reckoning.h"

#include "mx_hrt_debug.h"

/* Include the header files for all of the interfaces and devices. */

#include "i_vxworks_rs232.h"
#include "i_vxworks_vme.h"

#include "i_vme58.h"
#include "i_vsc16.h"

#include "d_soft_ainput.h"
#include "d_soft_aoutput.h"
#include "d_soft_dinput.h"
#include "d_soft_doutput.h"

#include "d_bit.h"
#include "d_vme_dio.h"

#include "d_soft_motor.h"
#include "d_vme58.h"

#include "d_elapsed_time.h"

#include "d_soft_scaler.h"
#include "d_vsc16_scaler.h"
#include "d_vsc16_timer.h"
#include "d_soft_timer.h"
#include "d_interval_timer.h"

#include "d_soft_amplifier.h"

#include "d_generic_relay.h"
#include "d_blind_relay.h"

#include "d_sis3801.h"
#include "d_sis3820.h"
#include "d_mcs_timer.h"

#include "v_mathop.h"

  /********************** Record Types **********************/

MX_DRIVER mx_type_table[] = {

{"list_head",      MXT_LIST_HEAD,    MXL_LIST_HEAD,      MXR_LIST_HEAD,
				&mxr_list_head_record_function_list,
				NULL,
				NULL,
				&mxr_list_head_num_record_fields,
				&mxr_list_head_rfield_def_ptr},

  /* =================== Interface types =================== */

#if defined(OS_VXWORKS)
{"vxworks_rs232",  MXI_232_VXWORKS,  MXI_RS232,          MXR_INTERFACE,
				&mxi_vxworks_rs232_record_function_list,
				NULL,
				&mxi_vxworks_rs232_rs232_function_list,
				&mxi_vxworks_rs232_num_record_fields,
				&mxi_vxworks_rs232_rfield_def_ptr},
#endif /* OS_VXWORKS */

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

#if defined(OS_VXWORKS)
{"vxworks_vme",    MXI_VME_VXWORKS,    MXI_VME,        MXR_INTERFACE,
				&mxi_vxworks_vme_record_function_list,
				NULL,
				&mxi_vxworks_vme_vme_function_list,
				&mxi_vxworks_vme_num_record_fields,
				&mxi_vxworks_vme_rfield_def_ptr},
#endif /* OS_VXWORKS */

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

{"soft_motor",     MXT_MTR_SOFTWARE,  MXC_MOTOR,          MXR_DEVICE,
				&mxd_soft_motor_record_function_list,
				NULL,
				&mxd_soft_motor_motor_function_list,
				&mxd_soft_motor_num_record_fields,
				&mxd_soft_motor_rfield_def_ptr},

{"vme58_motor",    MXT_MTR_VME58,     MXC_MOTOR,          MXR_DEVICE,
				&mxd_vme58_record_function_list,
				NULL,
				&mxd_vme58_motor_function_list,
				&mxd_vme58_num_record_fields,
				&mxd_vme58_rfield_def_ptr},

{"elapsed_time",   MXT_MTR_ELAPSED_TIME, MXC_MOTOR,       MXR_DEVICE,
				&mxd_elapsed_time_record_function_list,
				NULL,
				&mxd_elapsed_time_motor_function_list,
				&mxd_elapsed_time_num_record_fields,
				&mxd_elapsed_time_rfield_def_ptr},

{"soft_scaler",    MXT_SCL_SOFTWARE,  MXC_SCALER,         MXR_DEVICE,
				&mxd_soft_scaler_record_function_list,
				NULL,
				&mxd_soft_scaler_scaler_function_list,
				&mxd_soft_scaler_num_record_fields,
				&mxd_soft_scaler_rfield_def_ptr},

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

{"soft_timer",     MXT_TIM_SOFTWARE,  MXC_TIMER,          MXR_DEVICE,
				&mxd_soft_timer_record_function_list,
				NULL,
				&mxd_soft_timer_timer_function_list,
				&mxd_soft_timer_num_record_fields,
				&mxd_soft_timer_rfield_def_ptr},

{"interval_timer", MXT_TIM_INTERVAL,      MXC_TIMER,          MXR_DEVICE,
				&mxd_interval_timer_record_function_list,
				NULL,
				&mxd_interval_timer_timer_function_list,
				&mxd_interval_timer_num_record_fields,
				&mxd_interval_timer_rfield_def_ptr},

{"soft_amplifier", MXT_AMP_SOFTWARE,  MXC_AMPLIFIER,      MXR_DEVICE,
				&mxd_soft_amplifier_record_function_list,
				NULL,
				&mxd_soft_amplifier_amplifier_function_list,
				&mxd_soft_amplifier_num_record_fields,
				&mxd_soft_amplifier_rfield_def_ptr},

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

{"sis3801",        MXT_MCS_SIS3801,   MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				&mxd_sis3801_record_function_list,
				NULL,
				&mxd_sis3801_mcs_function_list,
				&mxd_sis3801_num_record_fields,
				&mxd_sis3801_rfield_def_ptr},

{"sis3820",        MXT_MCS_SIS3820,   MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				&mxd_sis3820_record_function_list,
				NULL,
				&mxd_sis3820_mcs_function_list,
				&mxd_sis3820_num_record_fields,
				&mxd_sis3820_rfield_def_ptr},

{"mcs_timer",      MXT_TIM_MCS,       MXC_TIMER,          MXR_DEVICE,
				&mxd_mcs_timer_record_function_list,
				NULL,
				&mxd_mcs_timer_timer_function_list,
				&mxd_mcs_timer_num_record_fields,
				&mxd_mcs_timer_rfield_def_ptr},

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
{"bool",           MXV_INL_BOOL,      MXV_INLINE,         MXR_VARIABLE,
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

{"mathop",         MXV_CAL_MATHOP,    MXV_CALC,          MXR_VARIABLE,
				&mxv_mathop_record_function_list,
				&mxv_mathop_variable_function_list,
				NULL,
				&mxv_mathop_num_record_fields,
				&mxv_mathop_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

