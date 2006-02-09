/*
 * Name:    i_mmap_vme.c
 *
 * Purpose: MX driver for VME bus access via mmap().  This driver is intended
 *          for use with a VME crate where the VME address space may be
 *          accessed indirectly via mmap().
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#if defined(OS_LINUX)

#define MX_MMAP_VME_DEBUG	FALSE

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_vme.h"
#include "i_mmap_vme.h"

#define TWO_TO_THE_8TH_POWER	256UL
#define TWO_TO_THE_16TH_POWER	65536UL
#define TWO_TO_THE_24TH_POWER	16777216UL

#if __WORDSIZE > 32
#define TWO_TO_THE_32ND_POWER	4294967296UL
#endif

MX_RECORD_FUNCTION_LIST mxi_mmap_vme_record_function_list = {
	NULL,
	mxi_mmap_vme_create_record_structures,
	mxi_mmap_vme_finish_record_initialization,
	NULL,
	mxi_mmap_vme_print_structure,
	NULL,
	NULL,
	mxi_mmap_vme_open,
	mxi_mmap_vme_close,
	NULL,
	mxi_mmap_vme_resynchronize
};

MX_VME_FUNCTION_LIST mxi_mmap_vme_vme_function_list = {
	mxi_mmap_vme_input,
	mxi_mmap_vme_output,
	mxi_mmap_vme_multi_input,
	mxi_mmap_vme_multi_output,
	mxi_mmap_vme_get_parameter,
	mxi_mmap_vme_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxi_mmap_vme_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VME_STANDARD_FIELDS,
	MXI_MMAP_VME_STANDARD_FIELDS
};

mx_length_type mxi_mmap_vme_num_record_fields
		= sizeof( mxi_mmap_vme_record_field_defaults )
			/ sizeof( mxi_mmap_vme_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxi_mmap_vme_rfield_def_ptr
		= &mxi_mmap_vme_record_field_defaults[0];

/*--*/

#if MX_MMAP_VME_DEBUG

#  define MX_MMAP_VME_DEBUG_PRINT \
	do { \
	    unsigned long value = 0; \
                                     \
	    switch( vme->data_size ) { \
	    case MXF_VME_D8:  \
		value = (unsigned long) *(uint8_t *) vme->data_pointer; \
		break; \
	    case MXF_VME_D16: \
		value = (unsigned long) *(uint16_t *) vme->data_pointer; \
		break; \
	    case MXF_VME_D32: \
		value = (unsigned long) *(uint32_t *) vme->data_pointer; \
		break; \
	    } \
	    MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value )); \
	} while(0)

#  define MX_MMAP_VME_MULTI_DEBUG_PRINT \
	do { \
	    unsigned long value = 0; \
                                     \
	    switch( vme->data_size ) { \
	    case MXF_VME_D8:  \
		value = (unsigned long) *(uint8_t *) vme->data_pointer; \
		break; \
	    case MXF_VME_D16: \
		value = (unsigned long) *(uint16_t *) vme->data_pointer; \
		break; \
	    case MXF_VME_D32: \
		value = (unsigned long) *(uint32_t *) vme->data_pointer; \
		break; \
	    } \
	MX_DEBUG(-2,("%s: A%lu, D%lu, addr = %#lx, value = %#lx (%lu values)", \
			fname, \
			(unsigned long) vme->address_mode, \
			(unsigned long) vme->data_size, \
			(unsigned long) vme->address, \
			(unsigned long) value, \
			vme->num_values )); \
	} while(0)
#else

#  define MX_MMAP_VME_DEBUG_PRINT

#  define MX_MMAP_VME_MULTI_DEBUG_PRINT

#endif

/*==========================*/

static mx_status_type
mxi_mmap_vme_get_pointers( MX_VME *vme,
			MX_MMAP_VME **mmap_vme,
			const char calling_fname[] )
{
	static const char fname[] = "mxi_mmap_vme_get_pointers()";

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( mmap_vme == (MX_MMAP_VME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MMAP_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( vme->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_VME pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*mmap_vme = (MX_MMAP_VME *) vme->record->record_type_struct;

	if ( *mmap_vme == (MX_MMAP_VME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MMAP_VME pointer for record '%s' is NULL.",
				vme->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_mmap_vme_bus_to_local_address( MX_MMAP_VME *mmap_vme,
					int address_mode,
					unsigned long bus_address,
					char **local_address )
{
	static const char fname[] = "mxi_mmap_vme_bus_to_local_address()";

	unsigned long page_number, address_offset;
	int saved_errno;
	size_t page_size;
	off_t page_start_offset;
	void **page_table;

	switch( address_mode ) {
	case MXF_VME_A16:
		if ( bus_address >= TWO_TO_THE_16TH_POWER ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Requested VME address %lu for record '%s' "
				"is too large to be an A16 address.",
					bus_address, mmap_vme->record->name );
		}

		page_table = mmap_vme->a16_page_table;
		page_size  = mmap_vme->a16_page_size;
		break;
	case MXF_VME_A24:
		if ( bus_address >= TWO_TO_THE_24TH_POWER ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Requested VME address %lu for record '%s' "
				"is too large to be an A24 address.",
					bus_address, mmap_vme->record->name );
		}

		page_table = mmap_vme->a24_page_table;
		page_size  = mmap_vme->a24_page_size;
		break;
	case MXF_VME_A32:

#if __WORDSIZE > 32
		if ( bus_address >= TWO_TO_THE_32ND_POWER ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Requested VME address %lu for record '%s' "
				"is too large to be an A32 address.",
					bus_address, mmap_vme->record->name );
		}
#endif

		page_table = mmap_vme->a32_page_table;
		page_size  = mmap_vme->a32_page_size;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported VME address mode A%d requested for record '%s'.",
			address_mode, mmap_vme->record->name );
		break;
	}

	page_number = bus_address / page_size;

	address_offset = bus_address % page_size;

	page_start_offset = page_number * page_size;

	if ( page_table[ page_number ] == NULL ) {

		/* Convert A16 and A24 page start addresses to A32 addresses.
		 *
		 * FIXME: I am not completely sure that the following
		 * is the way one is supposed to handle A16 and A24
		 * addresses. However, it seems to work with Linux
		 * running on an MVME162.
		 */

		switch( address_mode ) {
		case MXF_VME_A16:
			page_start_offset |= 0xffff0000;
			break;
		case MXF_VME_A24:
			page_start_offset |= 0xff000000;
			break;
		}

		/* Need to mmap() a page corresponding to this address. */

		page_table[ page_number ] = mmap( NULL, page_size,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			mmap_vme->file_descriptor, page_start_offset );

		if ( page_table[ page_number ] == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to mmap() an A%d page starting at "
			"address %#lx.  errno = %d, error message = '%s'.",
				address_mode, (unsigned long) page_start_offset,
				saved_errno, strerror( saved_errno ) );
		}

#if MX_MMAP_VME_DEBUG
		MX_DEBUG(-2,
		("%s: Added A%d page table entry for page %lu",
		 	fname, address_mode, page_number));
		MX_DEBUG(-2,
		("%s: page_start_offset = %#lx, page_table[%lu] = %p",
		 	fname, page_start_offset, page_number,
			page_table[ page_number ] ));
#endif
	}

	*local_address = ( char * ) ( page_table[ page_number ] )
							+ address_offset;

#if 0
	MX_DEBUG(-2,("%s: A%d bus address = %#lx, local address = %p",
		fname, address_mode, bus_address, *local_address ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_mmap_vme_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_mmap_vme_create_record_structures()";

	MX_VME *vme;
	MX_MMAP_VME *mmap_vme;

	/* Allocate memory for the necessary structures. */

	vme = (MX_VME *) malloc( sizeof(MX_VME) );

	if ( vme == (MX_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VME structure." );
	}

	mmap_vme = (MX_MMAP_VME *) malloc( sizeof(MX_MMAP_VME) );

	if ( mmap_vme == (MX_MMAP_VME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MMAP_VME structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = vme;
	record->record_type_struct = mmap_vme;
	record->class_specific_function_list
			= &mxi_mmap_vme_vme_function_list;

	vme->record = record;
	mmap_vme->record = record;

	mmap_vme->file_descriptor = -1;

	mmap_vme->a16_page_table = NULL;
	mmap_vme->a24_page_table = NULL;
	mmap_vme->a32_page_table = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_vme_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxi_mmap_vme_print_structure()";

	MX_VME *vme;
	MX_MMAP_VME *mmap_vme;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf( file, "VME parameters for interface '%s'\n\n", record->name );

	fprintf( file, "  mmap_vme_flags = '%#lx'\n\n",
				mmap_vme->mmap_vme_flags );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_mmap_vme_open()";

	MX_VME *vme;
	MX_MMAP_VME *mmap_vme;
	size_t system_page_size;
	int saved_errno;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the mmap() page size was specified as zero, take a guess as
	 * to what the page size should really be.
	 *
	 * FIXME: There probably should be a more profound algorithm for
	 * this depending on available memory size rather than just hard
	 * coding a value.
	 */

	if ( mmap_vme->page_size == 0 ) {
		mmap_vme->page_size = TWO_TO_THE_16TH_POWER;
	}

	/* Check to see if the requested mmap() page size is acceptable. */

	system_page_size = getpagesize();

	if ( ( mmap_vme->page_size % system_page_size ) != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested page size %lu for record '%s' is not "
			"a multiple of the system page size %lu.",
			mmap_vme->page_size, record->name,
			(unsigned long) system_page_size );
	}

	mmap_vme->file_descriptor = open( "/dev/mem", O_RDWR | O_SYNC );

	if ( mmap_vme->file_descriptor < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to open /dev/mem with read/write access "
		"for VME record '%s'.  errno = %d, error message = '%s'.",
			record->name, saved_errno, strerror( saved_errno ) );
	}

	if ( mmap_vme->page_size > TWO_TO_THE_16TH_POWER ) {
		mmap_vme->a16_page_size = (size_t) TWO_TO_THE_16TH_POWER;
		mmap_vme->num_a16_pages = 1;
	} else {
		mmap_vme->a16_page_size = (size_t) mmap_vme->page_size;
		mmap_vme->num_a16_pages =
			TWO_TO_THE_16TH_POWER / mmap_vme->a16_page_size;
	}

	if ( mmap_vme->page_size > TWO_TO_THE_24TH_POWER ) {
		mmap_vme->a24_page_size = (size_t) TWO_TO_THE_24TH_POWER;
		mmap_vme->num_a24_pages = 1;
	} else {
		mmap_vme->a24_page_size = (size_t) mmap_vme->page_size;
		mmap_vme->num_a24_pages =
			TWO_TO_THE_24TH_POWER / mmap_vme->a24_page_size;
	}

#if __WORDSIZE > 32

	if ( mmap_vme->page_size > TWO_TO_THE_32ND_POWER ) {
		mmap_vme->a32_page_size = (size_t) TWO_TO_THE_32ND_POWER;
		mmap_vme->num_a32_pages = 1;
	} else {
		mmap_vme->a32_page_size = (size_t) mmap_vme->page_size;
		mmap_vme->num_a32_pages =
			TWO_TO_THE_32ND_POWER / mmap_vme->a32_page_size;
	}

#else  /* __WORDSIZE <= 32 */

	mmap_vme->a32_page_size = (size_t) mmap_vme->page_size;
	mmap_vme->num_a32_pages = TWO_TO_THE_8TH_POWER
			* ( TWO_TO_THE_24TH_POWER / mmap_vme->a32_page_size );

#endif

	mmap_vme->a16_page_table = (void **)
		calloc( mmap_vme->num_a16_pages, sizeof(void *) );

	if ( mmap_vme->a16_page_table == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of A16 page table pointers for VME record '%s'.",
			mmap_vme->num_a16_pages, record->name );
	}

	mmap_vme->a24_page_table = (void **)
		calloc( mmap_vme->num_a24_pages, sizeof(void *) );

	if ( mmap_vme->a24_page_table == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of A24 page table pointers for VME record '%s'.",
			mmap_vme->num_a24_pages, record->name );
	}

	mmap_vme->a32_page_table = (void **)
		calloc( mmap_vme->num_a32_pages, sizeof(void *) );

	if ( mmap_vme->a32_page_table == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of A32 page table pointers for VME record '%s'.",
			mmap_vme->num_a32_pages, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_mmap_vme_close()";

	MX_VME *vme;
	MX_MMAP_VME *mmap_vme;
	int status, saved_errno;
	unsigned long i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	vme = (MX_VME *) (record->record_class_struct);

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mmap_vme->file_descriptor >= 0 ) {
		(void) close( mmap_vme->file_descriptor );
	}

	/* Deallocate the A16 page table. */

	if ( mmap_vme->a16_page_table != NULL ) {
		status = 0;

		for ( i = 0; i < mmap_vme->num_a16_pages; i++ ) {

			if ( mmap_vme->a16_page_table[i] != NULL ) {

				status = munmap( mmap_vme->a16_page_table[i],
						mmap_vme->a16_page_size );

				if ( status != 0 )
					break;   /* Exit the for() loop. */
			}
		}

		if ( status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to unmap A16 page table entry %lu for "
			"VME record '%s'.  errno = %d, error message = '%s'.",
			i, record->name, saved_errno, strerror( saved_errno ) );
		}

		mx_free( mmap_vme->a16_page_table );

		mmap_vme->num_a16_pages = 0;
	}

	/* Deallocate the A24 page table. */

	if ( mmap_vme->a24_page_table != NULL ) {
		status = 0;

		for ( i = 0; i < mmap_vme->num_a24_pages; i++ ) {

			if ( mmap_vme->a24_page_table[i] != NULL ) {

				status = munmap( mmap_vme->a24_page_table[i],
						mmap_vme->a24_page_size );

				if ( status != 0 )
					break;   /* Exit the for() loop. */
			}
		}

		if ( status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to unmap A24 page table entry %lu for "
			"VME record '%s'.  errno = %d, error message = '%s'.",
			i, record->name, saved_errno, strerror( saved_errno ) );
		}

		mx_free( mmap_vme->a24_page_table );

		mmap_vme->num_a24_pages = 0;
	}

	/* Deallocate the A32 page table. */

	if ( mmap_vme->a32_page_table != NULL ) {
		status = 0;

		for ( i = 0; i < mmap_vme->num_a32_pages; i++ ) {

			if ( mmap_vme->a32_page_table[i] != NULL ) {

				status = munmap( mmap_vme->a32_page_table[i],
						mmap_vme->a32_page_size );

				if ( status != 0 )
					break;   /* Exit the for() loop. */
			}
		}

		if ( status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Unable to unmap A32 page table entry %lu for "
			"VME record '%s'.  errno = %d, error message = '%s'.",
			i, record->name, saved_errno, strerror( saved_errno ) );
		}

		mx_free( mmap_vme->a32_page_table );

		mmap_vme->num_a32_pages = 0;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_resynchronize( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxi_mmap_vme_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_mmap_vme_open( record );

	return mx_status;
}

/*===============*/

MX_EXPORT mx_status_type
mxi_mmap_vme_input( MX_VME *vme )
{
	static const char fname[] = "mxi_mmap_vme_input()";

	MX_MMAP_VME *mmap_vme;
	char *vme_local_address;
	uint8_t *uint8_input_ptr;
	uint16_t *uint16_input_ptr;
	uint32_t *uint32_input_ptr;
	mx_status_type mx_status;

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mmap_vme->file_descriptor < 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Access to /dev/mem for VME record '%s' is unavailable.",
			vme->record->name );
	}

	mx_status = mxi_mmap_vme_bus_to_local_address( mmap_vme,
						vme->address_mode,
						vme->address,
						&vme_local_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_input_ptr = (uint8_t *) vme->data_pointer;

		*uint8_input_ptr = *( (uint8_t *) vme_local_address );
		break;
	case MXF_VME_D16:
		uint16_input_ptr = (uint16_t *) vme->data_pointer;

		*uint16_input_ptr = *( (uint16_t *) vme_local_address );
		break;
	case MXF_VME_D32:
		uint32_input_ptr = (uint32_t *) vme->data_pointer;

		*uint32_input_ptr = *( (uint32_t *) vme_local_address );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.",
			(unsigned long) vme->data_size );
		break;
	}

	MX_MMAP_VME_DEBUG_PRINT;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_output( MX_VME *vme )
{
	static const char fname[] = "mxi_mmap_vme_output()";

	MX_MMAP_VME *mmap_vme;
	char *vme_local_address;
	uint8_t *uint8_output_ptr;
	uint16_t *uint16_output_ptr;
	uint32_t *uint32_output_ptr;
	mx_status_type mx_status;

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mmap_vme->file_descriptor < 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Access to /dev/mem for VME record '%s' is unavailable.",
			vme->record->name );
	}

	mx_status = mxi_mmap_vme_bus_to_local_address( mmap_vme,
						vme->address_mode,
						vme->address,
						&vme_local_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_output_ptr = (uint8_t *) vme_local_address;

		*uint8_output_ptr = *( (uint8_t *) vme->data_pointer );
		break;
	case MXF_VME_D16:
		uint16_output_ptr = (uint16_t *) vme_local_address;

		*uint16_output_ptr = *( (uint16_t *) vme->data_pointer );
		break;
	case MXF_VME_D32:
		uint32_output_ptr = (uint32_t *) vme_local_address;

		*uint32_output_ptr = *( (uint32_t *) vme->data_pointer );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.",
			(unsigned long) vme->data_size );
		break;
	}

	MX_MMAP_VME_DEBUG_PRINT;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_multi_input( MX_VME *vme )
{
	static const char fname[] = "mxi_mmap_vme_multi_input()";

	MX_MMAP_VME *mmap_vme;
	unsigned long i;
	char *vme_local_address;
	uint8_t *uint8_ptr;
	uint16_t *uint16_ptr;
	uint32_t *uint32_ptr;
	mx_status_type mx_status;

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mmap_vme->file_descriptor < 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Access to /dev/mem for VME record '%s' is unavailable.",
			vme->record->name );
	}

	mx_status = mxi_mmap_vme_bus_to_local_address( mmap_vme,
						vme->address_mode,
						vme->address,
						&vme_local_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = (uint8_t *) vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			*uint8_ptr = *((uint8_t *) vme_local_address);

			uint8_ptr++;

			vme_local_address += vme->read_address_increment;
		}
		break;
	case MXF_VME_D16:
		uint16_ptr = vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			*uint16_ptr = *((uint16_t *) vme_local_address);

			uint16_ptr++;

			vme_local_address += vme->read_address_increment;
		}
		break;
	case MXF_VME_D32:
		uint32_ptr = vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			*uint32_ptr = *((uint32_t *) vme_local_address);

			uint32_ptr++;

			vme_local_address += vme->read_address_increment;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.",
			(unsigned long) vme->data_size );
		break;
	}

	MX_MMAP_VME_MULTI_DEBUG_PRINT;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_multi_output( MX_VME *vme )
{
	static const char fname[] = "mxi_mmap_vme_multi_output()";

	MX_MMAP_VME *mmap_vme;
	unsigned long i;
	char *vme_local_address;
	uint8_t *uint8_ptr;
	uint16_t *uint16_ptr;
	uint32_t *uint32_ptr;
	mx_status_type mx_status;

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mmap_vme->file_descriptor < 0 ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Access to /dev/mem for VME record '%s' is unavailable.",
			vme->record->name );
	}

	mx_status = mxi_mmap_vme_bus_to_local_address( mmap_vme,
						vme->address_mode,
						vme->address,
						&vme_local_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( vme->data_size ) {
	case MXF_VME_D8:
		uint8_ptr = (uint8_t *) vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			*((uint8_t *) vme_local_address) = *uint8_ptr;

			uint8_ptr++;

			vme_local_address += vme->write_address_increment;
		}
		break;
	case MXF_VME_D16:
		uint16_ptr = (uint16_t *) vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			*((uint16_t *) vme_local_address) = *uint16_ptr;

			uint16_ptr++;

			vme_local_address += vme->write_address_increment;
		}
		break;
	case MXF_VME_D32:
		uint32_ptr = (uint32_t *) vme->data_pointer;

		for ( i = 0; i < vme->num_values; i++ ) {
			*((uint32_t *) vme_local_address) = *uint32_ptr;

			uint32_ptr++;

			vme_local_address += vme->write_address_increment;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported VME data size %lu.",
			(unsigned long) vme->data_size );
		break;
	}

	MX_MMAP_VME_MULTI_DEBUG_PRINT;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_get_parameter( MX_VME *vme )
{
	static const char fname[] = "mxi_mmap_vme_get_parameter()";

	MX_MMAP_VME *mmap_vme;
	mx_status_type mx_status;

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for VME interface '%s' for parameter type '%s' (%d).",
		fname, vme->record->name,
		mx_get_field_label_string( vme->record, vme->parameter_type ),
		vme->parameter_type));

	switch( vme->parameter_type ) {
	case MXLV_VME_READ_ADDRESS_INCREMENT:
	case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		/* The controller itself does not have a place to store
		 * this information, so we just return the value we have
		 * stored locally.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the MX driver "
		"for VME interface '%s'.",
			mx_get_field_label_string(
				vme->record, vme->parameter_type ),
			vme->parameter_type, vme->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_mmap_vme_set_parameter( MX_VME *vme )
{
	static const char fname[] = "mxi_mmap_vme_set_parameter()";

	MX_MMAP_VME *mmap_vme;
	mx_status_type mx_status;

	mx_status = mxi_mmap_vme_get_pointers( vme, &mmap_vme, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for VME interface '%s' for parameter type '%s' (%d).",
		fname, vme->record->name,
		mx_get_field_label_string( vme->record, vme->parameter_type ),
		vme->parameter_type));

	switch( vme->parameter_type ) {
	case MXLV_VME_READ_ADDRESS_INCREMENT:
	case MXLV_VME_WRITE_ADDRESS_INCREMENT:
		/* The controller itself does not have a place to store
		 * this information, so we just store the information
		 * locally.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the MX driver "
		"for VME interface '%s'.",
			mx_get_field_label_string(
				vme->record, vme->parameter_type ),
			vme->parameter_type, vme->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX */
