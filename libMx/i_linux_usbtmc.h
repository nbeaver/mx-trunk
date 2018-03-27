/*
 * Name:    i_linux_usbtmc.h
 *
 * Purpose: Header for a Linux MX GPIB driver for USBTMC devices controlled
 *          via /dev/usbtmc0 and friends.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LINUX_USBTMC_H__
#define __I_LINUX_USBTMC_H__

/* Values for the 'linux_usbtmc_flags' field. */

#define MXF_LINUX_USBTMC_ASSERT_INTERFACE_CLEAR	0x1

typedef struct {
	MX_RECORD *record;

	char filename[MXU_FILENAME_LENGTH+1];
	unsigned long linux_usbtmc_flags;

	int usbtmc_fd;
} MX_LINUX_USBTMC;

/* Define all of the interface functions. */

MX_API mx_status_type mxi_linux_usbtmc_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_linux_usbtmc_open( MX_RECORD *record );

MX_API mx_status_type mxi_linux_usbtmc_open_device(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_linux_usbtmc_close_device(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_linux_usbtmc_read( MX_GPIB *gpib,
						long address,
						char *buffer,
						size_t max_bytes_to_read,
						size_t *bytes_read,
						unsigned long flags);
MX_API mx_status_type mxi_linux_usbtmc_write( MX_GPIB *gpib,
						long address,
						char *buffer,
						size_t bytes_to_write,
						size_t *bytes_written,
						unsigned long flags);
MX_API mx_status_type mxi_linux_usbtmc_interface_clear(MX_GPIB *gpib);
MX_API mx_status_type mxi_linux_usbtmc_device_clear(MX_GPIB *gpib);
MX_API mx_status_type mxi_linux_usbtmc_selective_device_clear(MX_GPIB *gpib,
						long address);
MX_API mx_status_type mxi_linux_usbtmc_local_lockout(MX_GPIB *gpib);
MX_API mx_status_type mxi_linux_usbtmc_remote_enable(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_linux_usbtmc_go_to_local(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_linux_usbtmc_trigger(MX_GPIB *gpib, long address);
MX_API mx_status_type mxi_linux_usbtmc_wait_for_service_request( MX_GPIB *gpib,
						double timeout );
MX_API mx_status_type mxi_linux_usbtmc_serial_poll( MX_GPIB *gpib, long address,
					unsigned char *serial_poll_byte );
MX_API mx_status_type mxi_linux_usbtmc_serial_poll_disable(MX_GPIB *gpib);

extern MX_RECORD_FUNCTION_LIST mxi_linux_usbtmc_record_function_list;
extern MX_GPIB_FUNCTION_LIST mxi_linux_usbtmc_gpib_function_list;

extern long mxi_linux_usbtmc_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_linux_usbtmc_rfield_def_ptr;

#define MXI_LINUX_USBTMC_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_USBTMC, filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "linux_usbtmc_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_USBTMC, linux_usbtmc_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __I_LINUX_USBTMC_H__ */

