/*
 * Name:    pr_vme.c
 *
 * Purpose: Functions used to process MX VME record events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_vme.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_vme_process_functions( MX_RECORD *record )
{
	static const char fname[] =
		"mx_setup_vme_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_VME_READ_ADDRESS_INCREMENT:
		case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		case MXLV_VME_DATA:
			record_field->process_function
					= mx_vme_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_vme_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_vme_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_VME *vme;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	vme = (MX_VME *) (record->record_class_struct);

	MXW_UNUSED( vme );

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_VME_READ_ADDRESS_INCREMENT:
			mx_status = mx_vme_get_read_address_increment(
					record, vme->crate,
					&(vme->read_address_increment) );
			break;
		case MXLV_VME_WRITE_ADDRESS_INCREMENT:
			mx_status = mx_vme_get_write_address_increment(
					record, vme->crate,
					&(vme->write_address_increment) );
			break;
		case MXLV_VME_DATA:
			switch( vme->data_size ) {
			case 8:
				mx_status = mx_vme_in8( record,
							vme->crate,
							vme->address_mode,
							vme->address,
						(uint8_t *) vme->data_pointer );
				break;
			case 16:
				mx_status = mx_vme_in16( record,
							vme->crate,
							vme->address_mode,
							vme->address,
						(uint16_t *) vme->data_pointer);
				break;
			case 32:
				mx_status = mx_vme_in32( record,
							vme->crate,
							vme->address_mode,
							vme->address,
						(uint32_t *) vme->data_pointer);
				break;
			default:
				return mx_error( MXE_UNSUPPORTED, fname,
				"Unsupported data size %lu requested "
				"for VME interface '%s'.",
					vme->data_size, record->name );
				break;
			}
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_VME_READ_ADDRESS_INCREMENT:
			mx_status = mx_vme_set_read_address_increment(
					record, vme->crate,
					vme->read_address_increment );
			break;
		case MXLV_VME_WRITE_ADDRESS_INCREMENT:
			mx_status = mx_vme_set_write_address_increment(
					record, vme->crate,
					vme->write_address_increment );
			break;
		case MXLV_VME_DATA:
			switch( vme->data_size ) {
			case 8:
				mx_status = mx_vme_out8( record,
							vme->crate,
							vme->address_mode,
							vme->address,
					*( (uint8_t *) vme->data_pointer ) );
				break;
			case 16:
				mx_status = mx_vme_out16( record,
							vme->crate,
							vme->address_mode,
							vme->address,
					*( (uint16_t *) vme->data_pointer ) );
				break;
			case 32:
				mx_status = mx_vme_out32( record,
							vme->crate,
							vme->address_mode,
							vme->address,
					*( (uint32_t *) vme->data_pointer ) );
				break;
			default:
				return mx_error( MXE_UNSUPPORTED, fname,
				"Unsupported data size %lu requested "
				"for VME interface '%s'.",
					vme->data_size, record->name );
				break;
			}
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

