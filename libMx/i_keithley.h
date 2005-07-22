/*
 * Name:    i_keithley.h
 *
 * Purpose: Header file for common MX functions used by Keithley 2000, 2400,
 *          and 2700 series controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_KEITHLEY_H__
#define __I_KEITHLEY_H__

/* Status byte register bits for the *STB? command. */

#define MXF_KEITHLEY_STB_MSB	0x1	/* Measurement Summary Bit */
#define MXF_KEITHLEY_STB_EAV	0x4	/* Error Available */
#define MXF_KEITHLEY_STB_QSB	0x8	/* Questionable Summary Bit */
#define MXF_KEITHLEY_STB_MAV	0x10	/* Message Available */
#define MXF_KEITHLEY_STB_ESB	0x20	/* Event Summary Bit */
#define MXF_KEITHLEY_STB_RQS	0x40	/* Request Service */
#define MXF_KEITHLEY_STB_MSS	0x40	/* Master Summary Status */
#define MXF_KEITHLEY_STB_OSB	0x80	/* Operation Summary */

MX_API mx_status_type mxi_keithley_command( MX_RECORD *keithley_record,
				MX_INTERFACE *port_interface,
				char *command,
				char *response, int response_buffer_length,
				int debug_flag );

MX_API mx_status_type mxi_keithley_putline( MX_RECORD *keithley_record,
				MX_INTERFACE *port_interface,
				char *command,
				int debug_flag );

MX_API mx_status_type mxi_keithley_getline( MX_RECORD *keithley_record,
				MX_INTERFACE *port_interface,
				char *response, int response_buffer_length,
				int debug_flag );

#endif /* __I_KEITHLEY_H__ */
