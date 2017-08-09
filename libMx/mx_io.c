/*
 * Name:    mx_io.c
 *
 * Purpose: MX-specific file/pipe/etc I/O related functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2010-2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_LSOF	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"
#include "mx_socket.h"
#include "mx_select.h"
#include "mx_dynamic_library.h"
#include "mx_io.h"

#if defined(OS_WIN32) && !defined(__GNUC__)
#  define popen(x,p) _popen(x,p)
#  define pclose(x)  _pclose(x)
#endif

/*-------------------------------------------------------------------------*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_VMS) \
	|| defined(OS_DJGPP) || defined(OS_RTEMS) || defined(OS_VXWORKS) \
	|| defined(OS_ANDROID) || defined(OS_MINIX)

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if defined(OS_SOLARIS)
#  include <sys/filio.h>
#endif

#if defined(OS_CYGWIN)
#  include <sys/socket.h>
#endif

/*---*/

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_IRIX) || defined(OS_CYGWIN) || defined(OS_RTEMS) \
	|| defined(OS_ANDROID)

#  define USE_FIONREAD	TRUE
#else
#  define USE_FIONREAD	FALSE
#endif

/*---*/

MX_EXPORT mx_status_type
mx_fd_num_input_bytes_available( int fd, size_t *num_bytes_available )
{
	static const char fname[] = "mx_fd_num_input_bytes_available()";

	int os_status;

	if ( num_bytes_available == NULL )  {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available pointer passed was NULL." );
	}
	if ( fd < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"File descriptor %d passed is not a valid file descriptor.",
			fd );
	}

#if USE_FIONREAD
	{
		int saved_errno;
		int num_chars_available;

		os_status = ioctl( fd, FIONREAD, &num_chars_available );

		if ( os_status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"An error occurred while checking file descriptor %d "
			"to see if any input characters are available.  "
			"Errno = %d, error message = '%s'",
				fd, saved_errno, strerror(saved_errno) );
		}

		*num_bytes_available = num_chars_available;
	}
#else
	{
		/* Use select() instead.
		 *
		 * select() does not give us a way of knowing how many
		 * bytes are available, so we can only promise that there
		 * is at least 1 byte available.
		 */

		fd_set mask;
		struct timeval timeout;

		FD_ZERO( &mask );
		FD_SET( fd, &mask );

		timeout.tv_sec = 0;  timeout.tv_usec = 0;

		os_status = select( 1 + fd, &mask, NULL, NULL, &timeout );

		if ( os_status ) {
			*num_bytes_available = 1;
		} else {
			*num_bytes_available = 0;
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

MX_EXPORT mx_status_type
mx_fd_num_input_bytes_available( int fd, size_t *num_bytes_available )
{
	static const char fname[] = "mx_fd_num_input_bytes_available()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

/*-------------------------------------------------------------------------*/

#else
#error MX file descriptor I/O functions are not yet implemented for this platform.
#endif

/*=========================================================================*/

MX_EXPORT int
mx_get_max_file_descriptors( void )
{
	int result;

#if defined( OS_UNIX ) || defined( OS_CYGWIN ) || defined( OS_DJGPP ) \
	|| defined( OS_VMS ) || defined( OS_MINIX )

	result = getdtablesize();

#elif defined( OS_ANDROID )

	result = sysconf(_SC_OPEN_MAX);

#elif defined( OS_WIN32 )

#  if ( defined(_MSC_VER) && (_MSC_VER >= 1100) )
	result = _getmaxstdio();
#  else
	result = 512;
#  endif

#elif defined( OS_ECOS )

	result = CYGNUM_FILEIO_NFD;

#elif defined( OS_RTEMS ) || defined( OS_VXWORKS )

	result = 16;		/* A wild guess and probably wrong. */

#else

#error mx_get_max_file_descriptors() has not been implemented for this platform.

#endif

	return result;
}

/*=========================================================================*/

#if defined( OS_UNIX ) || defined( OS_CYGWIN ) || defined( OS_DJGPP ) \
	|| defined( OS_VMS ) || defined( OS_ECOS ) || defined( OS_RTEMS ) \
	|| defined( OS_VXWORKS ) || defined( OS_ANDROID ) || defined( OS_MINIX )

MX_EXPORT int
mx_get_number_of_open_file_descriptors( void )
{
	static const char fname[] = "mx_get_number_of_open_file_descriptors()";

	int i, max_descriptors, os_status, saved_errno, num_open;
	struct stat stat_buf;

	num_open = 0;
	max_descriptors = mx_get_max_file_descriptors();

	for ( i = 0; i < max_descriptors; i++ ) {
		os_status = fstat( i, &stat_buf );

		saved_errno = errno;

#if 0
		MX_DEBUG(-2,("open fds: i = %d, os_status = %d, errno = %d",
			i, os_status, saved_errno));
#endif

		if ( os_status == 0 ) {
			num_open++;
		} else {
			if ( saved_errno != EBADF ) {
				mx_warning(
		"%s: fstat(%d,...) returned errno = %d, error message = '%s'",
				fname, i, saved_errno, strerror(saved_errno) );
			}
		}
	}

	return num_open;
}

#elif defined( OS_WIN32 )

MX_EXPORT int
mx_get_number_of_open_file_descriptors( void )
{
	intptr_t handle;
	int i, max_fds, used_fds;

	max_fds = mx_get_max_file_descriptors();

	used_fds = 0;

	for ( i = 0 ; i < max_fds ; i++ ) {
		handle = _get_osfhandle( i );

		if ( handle != (-1) ) {
			used_fds++;
		}
	}

	return used_fds;
}

#else

#error mx_get_number_of_open_file_descriptors() has not been implemented for this platform.

#endif

/*=========================================================================*/

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_BSD) || defined(OS_QNX) || defined(OS_RTEMS) \
	|| defined(OS_CYGWIN) || defined(OS_UNIXWARE) || defined(OS_VMS) \
	|| defined(OS_HURD) || defined(OS_DJGPP) || defined(OS_ANDROID) \
	|| defined(OS_MINIX)

MX_EXPORT mx_bool_type
mx_fd_is_valid( int fd )
{
	int flags;

	flags = fcntl( fd, F_GETFD );

	if ( flags == -1 ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

#elif defined(OS_WIN32)

MX_EXPORT mx_bool_type
mx_fd_is_valid( int fd )
{
	intptr_t handle;

	handle = _get_osfhandle( fd );

	if ( handle == (-1) ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

#elif defined(OS_VXWORKS)

MX_EXPORT mx_bool_type
mx_fd_is_valid( int fd )
{
	return TRUE;
}

#else

#error mx_fd_is_valid() has not been implemented for this platform.

#endif

/*=========================================================================*/

#if defined(OS_WIN32)

MX_EXPORT int64_t
mx_get_file_size( char *filename )
{
	static const char fname[] = "mx_get_file_size()";

	int64_t file_size;

	HANDLE file_handle;
	DWORD low_doubleword, high_doubleword;
	long last_error_code;
	char error_message[1000];

	file_handle = CreateFile( filename,
				0,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL );

	if ( file_handle == INVALID_HANDLE_VALUE ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			error_message, sizeof(error_message) );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open file '%s' for read access failed.  "
		"Win32 error code = %ld, error message = '%s'",
			filename, last_error_code, error_message );

		return (-1);
	}

	low_doubleword = GetFileSize( file_handle, &high_doubleword );

	if ( low_doubleword == INVALID_FILE_SIZE ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			error_message, sizeof(error_message) );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to get the file size of file '%s' failed.  "
		"Win32 error code = %ld, error message = '%s'",
			filename, last_error_code, error_message );

		return (-1);
	}

	CloseHandle( file_handle );

	file_size = (int64_t) low_doubleword
			+ ( ( (int64_t) high_doubleword ) << 32 );

	return file_size;
}

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_BSD) \
	|| defined(OS_CYGWIN) || defined(OS_VMS) || defined(OS_HURD) \
	|| defined(OS_QNX) || defined(OS_RTEMS) || defined(OS_VXWORKS) \
	|| defined(OS_DJGPP) || defined(OS_ANDROID) || defined(OS_MINIX)

MX_EXPORT int64_t
mx_get_file_size( char *filename )
{
	int64_t file_size;

	struct stat stat_buffer;
	int os_status;

	os_status = stat( filename, &stat_buffer );

	if ( os_status < 0 ) {
		return (-1);
	}

	file_size = stat_buffer.st_size;

	return file_size;
}

#elif defined(OS_SOLARIS) || defined(OS_UNIXWARE)

MX_EXPORT int64_t
mx_get_file_size( char *filename )
{
	int64_t file_size;

	struct stat64 stat_buffer;
	int os_status;

	os_status = stat64( filename, &stat_buffer );

	if ( os_status < 0 ) {
		return (-1);
	}

	file_size = stat_buffer.st_size;

	return file_size;
}

#else

#error mx_get_file_size() has not yet been implemented for this platform.

#endif

/*=========================================================================*/

/* Windows with Visual C++ 2005 or newer. */

#if defined(OS_WIN32) && (_MSC_VER >= 1400)

MX_EXPORT int64_t
mx_get_file_offset( FILE *mx_file )
{
	int64_t offset;

	if ( mx_file == NULL ) {
		errno = EINVAL;

		return (-1);
	}

	offset = _ftelli64( mx_file );

	return offset;
}

MX_EXPORT int64_t
mx_set_file_offset( FILE *mx_file, int64_t offset, int origin )
{
	if ( mx_file == NULL ) {
		errno = EINVAL;

		return (-1);
	}

	offset = _fseeki64( mx_file, offset, origin );

	return offset;
}

/* Recent versions of Linux/MacOS X/Posix operating systems. */

#elif ( (_FILE_OFFSET_BITS == 64) \
	|| (_POSIX_C_SOURCE >= 200112L) \
	|| (_XOPEN_SOURCE >= 600) )

MX_EXPORT int64_t
mx_get_file_offset( FILE *mx_file )
{
	int64_t offset;

	if ( mx_file == NULL ) {
		errno = EINVAL;

		return (-1);
	}

	offset = ftello( mx_file );

	return offset;
}

MX_EXPORT int64_t
mx_set_file_offset( FILE *mx_file, int64_t offset, int origin )
{
	if ( mx_file == NULL ) {
		errno = EINVAL;

		return (-1);
	}

	offset = fseeko( mx_file, offset, origin );

	return offset;
}

/* All other operating system versions use generic fseek() and ftell()
 * calls that only report 32-bit offsets.
 */

#else

MX_EXPORT int64_t
mx_get_file_offset( FILE *mx_file )
{
	int64_t offset;

	if ( mx_file == NULL ) {
		errno = EINVAL;

		return (-1);
	}

	offset = ftell( mx_file );

	return offset;
}

MX_EXPORT int64_t
mx_set_file_offset( FILE *mx_file, int64_t offset, int origin )
{
	if ( mx_file == NULL ) {
		errno = EINVAL;

		return (-1);
	}

	offset = fseek( mx_file, (long) offset, origin );

	return offset;
}

#endif

/*=========================================================================*/

/*
 * mx_get_disk_space() only reports on disk space available to 
 * the calling user.  If user quotas are in effect, these values
 * may be smaller than the values for the whole disk.
 */

#if defined(OS_WIN32)

typedef BOOL (*mxp_GetDiskFreeSpaceEx_type)( LPCTSTR, PULARGE_INTEGER,
					PULARGE_INTEGER, PULARGE_INTEGER );

MX_EXPORT mx_status_type
mx_get_disk_space( char *filename,
		uint64_t *user_total_bytes_in_partition,
		uint64_t *user_free_bytes_in_partition )
{
	static const char fname[] = "mx_get_disk_space()";

	static mx_bool_type get_disk_free_space_ex_tested_for = FALSE;
	static mx_bool_type get_disk_free_space_ex_available = FALSE;

	static mxp_GetDiskFreeSpaceEx_type pGetDiskFreeSpaceEx = NULL;

	HINSTANCE hinst_kernel32;
	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	char root_name[MXU_FILENAME_LENGTH+1];
	mx_status_type mx_status;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	/* The functions below require the use of a directory name, rather
	 * than a filename.  However, the directory name does not need to
	 * be the root directory of that partition.  Nevertheless, the
	 * already existing function mx_get_filesystem_root_name() will
	 * provide a name that can be used below.
	 */

	mx_status = mx_get_filesystem_root_name( filename,
					root_name, sizeof(root_name) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	MX_DEBUG(-2,("%s: filename = '%s', root_name = '%s'",
		fname, filename, root_name));
#endif

	/* Check to see if GetDiskFreeSpaceEx() is available.  It _should_ be
	 * available for Windows NT 4.0 and above, as well as Windows 95 OSR2
	 * or above, which covers most cases, but we check anyway.
	 */

	if ( get_disk_free_space_ex_tested_for == FALSE ) {
		get_disk_free_space_ex_tested_for = TRUE;

		hinst_kernel32 = GetModuleHandle(TEXT("kernel32.dll"));

		if ( hinst_kernel32 == NULL ) {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The operating system says that it cannot find "
			"'KERNEL32.DLL'.  If so, then it is likely that "
			"Microsoft Windows will crash soon.  In that case, "
			"attempting to send you this error message is probably "
			"an exercise in futility, but we try anyway." );
		}

		pGetDiskFreeSpaceEx = (mxp_GetDiskFreeSpaceEx_type)
			GetProcAddress( hinst_kernel32,
				TEXT("GetDiskFreeSpaceEx") );

		if ( pGetDiskFreeSpaceEx != NULL ) {
			get_disk_free_space_ex_available = TRUE;
		}
	}

	if ( get_disk_free_space_ex_available ) {
		/* Use the modern function. */

		ULARGE_INTEGER free_bytes_available;
		ULARGE_INTEGER total_number_of_bytes;
		ULARGE_INTEGER total_number_of_free_bytes;

		os_status = pGetDiskFreeSpaceEx( root_name,
						&free_bytes_available,
						&total_number_of_bytes,
						&total_number_of_free_bytes );

		if ( os_status == 0 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The attempt to get the disk usage for the disk "
			"partition containing the file '%s' failed.  "
			"Win32 error code = %ld, error message = '%s'.",
				filename, last_error_code, message_buffer );
		}

		if ( user_total_bytes_in_partition != NULL ) {
			*user_total_bytes_in_partition
			    = (uint64_t) total_number_of_bytes.QuadPart;
		}
		if ( user_free_bytes_in_partition != NULL ) {
			*user_free_bytes_in_partition
			    = (uint64_t) free_bytes_available.QuadPart;
		}
	} else {
		/* Use the crufty old function. */

		DWORD sectors_per_cluster;
		DWORD bytes_per_sector;
		DWORD number_of_free_clusters;
		DWORD total_number_of_clusters;
		uint64_t bytes_per_cluster;

		os_status = GetDiskFreeSpace( root_name,
						&sectors_per_cluster,
						&bytes_per_sector,
						&number_of_free_clusters,
						&total_number_of_clusters );

		if ( os_status == 0 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The attempt to get the disk usage via "
			"GetDiskFreeSpace() for the disk "
			"partition containing the file '%s' failed.  "
			"Win32 error code = %ld, error message = '%s'.",
				filename, last_error_code, message_buffer );
		}

		bytes_per_cluster = ( (uint64_t) bytes_per_sector )
				* ( (uint64_t) sectors_per_cluster );

		/* GetDiskFreeSpace() only provides a way to find out the
		 * number of bytes available in the user's quota, so we
		 * copied the quota-ed value to the total values.  In any
		 * case, this code path should only be called by versions
		 * of Windows from 1995 or before.
		 */

		if ( user_total_bytes_in_partition != NULL ) {
			*user_total_bytes_in_partition = bytes_per_cluster
					* (uint64_t) total_number_of_clusters;
		}
		if ( user_free_bytes_in_partition != NULL ) {
			*user_free_bytes_in_partition = bytes_per_cluster
					* (uint64_t) number_of_free_clusters;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_BSD) || defined(OS_QNX) || defined(OS_ANDROID) \
	|| defined(OS_CYGWIN) || defined(OS_HURD) || defined(OS_MINIX)

/* statvfs() version */

#include <sys/statvfs.h>

MX_EXPORT mx_status_type
mx_get_disk_space( char *filename,
		uint64_t *user_total_bytes_in_partition,
		uint64_t *user_free_bytes_in_partition )
{
	static const char fname[] = "mx_get_disk_space()";

	struct statvfs fs_stats;
	int os_status, saved_errno;
	uint64_t fragment_size;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	os_status = statvfs( filename, &fs_stats );

	if ( os_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The call to statvfs( '%s', &fs_stats ) returned with "
		"errno = %d, error message = '%s'.",
			filename, saved_errno, strerror(saved_errno) );
	}

	fragment_size = fs_stats.f_frsize;

	if ( user_total_bytes_in_partition != NULL ) {
		*user_total_bytes_in_partition = fragment_size
					* (uint64_t) fs_stats.f_blocks;
	}
	if ( user_free_bytes_in_partition != NULL ) {
		*user_free_bytes_in_partition = fragment_size
					* (uint64_t) fs_stats.f_bavail;
	}

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_VXWORKS)

/* statfs() version */

#include <sys/stat.h>

MX_EXPORT mx_status_type
mx_get_disk_space( char *filename,
		uint64_t *user_total_bytes_in_partition,
		uint64_t *user_free_bytes_in_partition )
{
	static const char fname[] = "mx_get_disk_space()";

	struct statfs fs_stats;
	uint64_t block_size;
	int os_status, saved_errno;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	os_status = statfs( filename, &fs_stats );

	if ( os_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The call to statfs( '%s', &fs_stats ) returned with "
		"errno = %d, error message = '%s'.",
			filename, saved_errno, strerror(saved_errno) );
	}

	block_size = (uint64_t) fs_stats.f_bsize;

	if ( user_total_bytes_in_partition != NULL ) {
		*user_total_bytes_in_partition = block_size
					* (uint64_t) fs_stats.f_blocks;
	}
	if ( user_free_bytes_in_partition != NULL ) {
		*user_free_bytes_in_partition = block_size
					* (uint64_t) fs_stats.f_bavail;
	}

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_VMS)

#include <errno.h>
#include <ssdef.h>
#include <starlet.h>
#include <descrip.h>
#include <dvidef.h>

typedef struct {
	uint16_t buffer_length;
	uint16_t item_code;
	uint32_t *buffer_address;
	uint32_t *return_length_address;
} ILE;

MX_EXPORT mx_status_type
mx_get_disk_space( char *filename,
		uint64_t *user_total_bytes_in_partition,
		uint64_t *user_free_bytes_in_partition )
{
	static const char fname[] = "mx_get_disk_space()";

	unsigned int max_blocks, free_blocks, block_size;
	int vms_status;
	char *filename_copy = NULL;
	char *ptr = NULL;

	short io_status_block[4];

	ILE item_list[] =
	      { {4, DVI$_FREEBLOCKS, &free_blocks, NULL},
		{4, DVI$_MAXBLOCK, &max_blocks, NULL},
		{0, 0, NULL, NULL} };

	struct dsc$descriptor device_descriptor;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	device_descriptor.dsc$b_dtype = DSC$K_DTYPE_T;
	device_descriptor.dsc$b_class = DSC$K_CLASS_S;

	filename_copy = strdup( filename );

	if ( filename_copy == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to make a copy of the filename '%s'.",
			filename );
	}

	ptr = strchr( filename_copy, ':' );

	if ( ptr == NULL ) {
		/* FIXME: Add a way to get the default disk device. */

		device_descriptor.dsc$a_pointer = "SYS$SYSDEVICE";
	} else {
		*ptr = '\0';

		device_descriptor.dsc$a_pointer = filename_copy;
	}

	device_descriptor.dsc$w_length =
			strlen( device_descriptor.dsc$a_pointer ) + 1;

	block_size = 512;

	vms_status = sys$getdviw( 0, 0, &device_descriptor, &item_list,
				&io_status_block, NULL, NULL, 0 );

	if ( vms_status != SS$_NORMAL ) {
		mx_free( filename_copy );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the total and free disk space "
		"on the drive containing '%s' Failed.  "
		"VMS error number %d, error message = '%s'",
			filename, vms_status,
			strerror( EVMSERR, vms_status ) );
	}

	if ( user_total_bytes_in_partition != NULL ) {
		*user_total_bytes_in_partition = block_size
					* (uint64_t) max_blocks;
	}

	if ( user_free_bytes_in_partition != NULL ) {
		*user_free_bytes_in_partition = block_size
					* (uint64_t) free_blocks;
	}

	mx_free( filename_copy );

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_DJGPP)

/* This version is for DJGPP using DPMI calls directly.  It is designed
 * for FAT16 partitions, so it only reports sizes up to 2 gigabytes.
 */

#include <dpmi.h>

MX_EXPORT mx_status_type
mx_get_disk_space( char *filename,
		uint64_t *user_total_bytes_in_partition,
		uint64_t *user_free_bytes_in_partition )
{
	static const char fname[] = "mx_get_disk_space()";

	__dpmi_regs regs;
	uint64_t ax_cx;;
	char drive_number;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	if ( filename[1] != ':' ) {
		drive_number = 0;	/* The default drive. */
	} else {
		if ( islower( filename[0] ) ) {
			drive_number = filename[0] - 'a' + 1;
		} else {
			drive_number = filename[0] - 'A' + 1;
		}
	}

	regs.x.ax = 0x3600;
	regs.x.dx = drive_number;

	__dpmi_int( 0x21, &regs );	/* call DOS */

	if ( regs.x.ax == 0xffff ) {
		if ( drive_number == 0 ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The current default drive is not a valid drive." );
		} else {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The requested drive '%c:' is not a valid drive.",
				'A' + drive_number - 1 );
		}
	}

	ax_cx = (uint64_t) regs.x.ax * (uint64_t) regs.x.cx;

	if ( user_total_bytes_in_partition != NULL ) {
		*user_total_bytes_in_partition = ax_cx * (uint64_t) regs.x.dx;
	}
	if ( user_free_bytes_in_partition != NULL ) {
		*user_free_bytes_in_partition = ax_cx * (uint64_t) regs.x.bx;
	}

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_RTEMS)

/* This is a stub that always reports that 2 gigabytes of disk space is free.
 *
 * This stub is only intended for systems that do not provide a way of finding
 * out how much disk space is free, so do not use it otherwise.
 */

MX_EXPORT mx_status_type
mx_get_disk_space( char *filename,
		uint64_t *user_total_bytes_in_partition,
		uint64_t *user_free_bytes_in_partition )
{
	static const char fname[] = "mx_get_disk_space()";

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	if ( user_total_bytes_in_partition != NULL ) {
		*user_total_bytes_in_partition = 2147483647L;
	}
	if ( user_free_bytes_in_partition != NULL ) {
		*user_free_bytes_in_partition = 2147483647L;
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

#error mx_get_disk_space() has not yet been implemented for this platform.

#endif

/*=========================================================================*/

#if defined(OS_MACOSX) || defined(OS_BSD) || defined(OS_UNIXWARE)

#define MXP_LSOF_FILE	1
#define MXP_LSOF_PIPE	2
#define MXP_LSOF_SOCKET	3

static void
mxp_lsof_get_pipe_peer( unsigned long process_id,
			int inode,
			unsigned long *peer_pid,
			char *peer_command_name,
			size_t peer_command_name_length )
{
	static const char fname[] = "mxp_lsof_get_pipe_peer()";

	FILE *file;
	char command[200];
	char response[MXU_FILENAME_LENGTH+20];
	int saved_errno;
	unsigned long current_pid, current_inode;
	char current_command_name[MXU_FILENAME_LENGTH+1];
	int max_fds;
	mx_bool_type is_fifo;

#if DEBUG_LSOF
	fprintf( stderr, "Looking for pipe inode %d\n", inode );
#endif

	max_fds = mx_get_max_file_descriptors();

	snprintf( command, sizeof(command), "lsof -d0-%d -FfatDin", max_fds );

	file = popen( command, "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to run the command '%s'.  "
		"Errno = %d, error message = '%s'",
			command, saved_errno, strerror(saved_errno) );

		return;
	}

	mx_fgets( response, sizeof(response), file );

	if ( feof(file) || ferror(file) ) {
		/* Did not get any output from lsof. */

		pclose(file);
		return;
	}

	is_fifo = FALSE;
	current_inode = 0;
	current_command_name[0] = '\0';
	current_pid = 0;

	*peer_pid = 0;
	peer_command_name[0] = '\0';

	while (1) {

#if DEBUG_LSOF
		fprintf( stderr, "response = '%s'\n", response );
#endif
		switch( response[0] ) {
		case 'c':
			strlcpy( current_command_name, response+1,
				sizeof(current_command_name) );
			break;
		case 'f':
			is_fifo = FALSE;
			current_inode = 0;
			current_command_name[0] = '\0';
			break;
		case 'i':
			current_inode = mx_string_to_unsigned_long(response+1);
			break;
		case 'p':
			current_pid = mx_string_to_unsigned_long(response+1);
			break;
		case 't':
			if ( strcmp( response, "tFIFO" ) == 0 ) {
				is_fifo = TRUE;
			} else {
				is_fifo = FALSE;
			}
			break;
		}

		if ( is_fifo
		  && ( current_inode == inode )
		  && ( current_pid != process_id )
		) {
			/* We have found the correct combination. */

			break;
		}

		mx_fgets( response, sizeof(response), file );

		if ( feof(file) || ferror(file) ) {
			/* End of lsof output, so give up. */

			pclose(file);
			return;
		}
	}

	*peer_pid = current_pid;

	strlcpy( peer_command_name, current_command_name,
		peer_command_name_length );

	pclose(file);
	return;
}

static char *
mxp_parse_lsof_output( FILE *file,
			unsigned long process_id, int fd,
			char *buffer, size_t buffer_size )
{
	static const char fname[] = "mxp_parse_lsof_output()";

	mx_bool_type is_self;
	char response[MXU_FILENAME_LENGTH + 20];

	char mode[4] = "";
	char filename[MXU_FILENAME_LENGTH + 1] = "";
	char socket_protocol[20] = "";
	char socket_state[40] = "";
	unsigned long inode = 0;
	mx_bool_type inode_found;
	int fd_type = 0;

	int c, i, response_fd;
	size_t length;
	unsigned long peer_pid = 0;
	char peer_command_name[MXU_FILENAME_LENGTH+1];

	if ( process_id == mx_process_id() ) {
		is_self = TRUE;
	} else {
		is_self = FALSE;
	}

	MXW_UNUSED( is_self );

	c = fgetc( file );

	if ( feof(file) || ferror(file) ) {
		return NULL;
	}

	ungetc( c, file );

	if ( c != 'f' ) {
		(void) mx_error( MXE_PROTOCOL_ERROR, fname,
		"The first character from lsof for fd %d is not 'f'.  "
		"Instead, it is '%c' %x.", fd, c, c );

		return NULL;
	}

	fd_type = -1;
	filename[0] = '\0';
	inode_found = FALSE;
	mode[0] = '\0';
	response_fd = -1;
	socket_protocol[0] = '\0';

	while (1) {
		mx_fgets( response, sizeof(response), file );

		if ( feof(file) || ferror(file) ) {
			break;
		}

#if DEBUG_LSOF
		fprintf(stderr, "response = '%s'\n", response);
#endif

		switch( response[0] ) {
		case 'f':
			response_fd = mx_string_to_long( response+1 );

			if ( fd < 0 ) {
				fd = response_fd;
			} else
			if ( response_fd != fd ) {
				(void) mx_error( MXE_PROTOCOL_ERROR, fname,
				"The file descriptor in the response line "
				"did not have the expected value of %d.  "
				"Instead, it had a value of %d.",
					fd, response_fd );
				return NULL;
			}
			break;
			
		case 'a':
			if ( response[1] == 'r' ) {
				strlcpy( mode, "r", sizeof(mode) );
			} else
			if ( response[1] == 'w' ) {
				strlcpy( mode, "w", sizeof(mode) );
			} else
			if ( response[1] == 'u' ) {
				strlcpy( mode, "rw", sizeof(mode) );
			} else {
				strlcpy( mode, "?", sizeof(mode) );
			}
			break;
		case 'i':
			inode = mx_string_to_long( response+1 );
			inode_found = TRUE;
			break;
		case 'n':
			strlcpy( filename, response+1, sizeof(filename) );
			break;
		case 't':
			if ( strcmp( response+1, "REG" ) == 0 ) {
				fd_type = MXP_LSOF_FILE;
			} else
			if ( strcmp( response+1, "CHR" ) == 0 ) {
				fd_type = MXP_LSOF_FILE;
			} else
			if ( strcmp( response+1, "PIPE" ) == 0 ) {
				fd_type = MXP_LSOF_PIPE;
			} else
			if ( strcmp( response+1, "FIFO" ) == 0 ) {
				fd_type = MXP_LSOF_PIPE;
			} else
			if ( strcmp( response+1, "IPv4" ) == 0 ) {
				fd_type = MXP_LSOF_SOCKET;
			} else {
				fd_type = 0;
			}
			break;
		case 'P':
			length = sizeof(socket_protocol);

			for ( i = 0; i < length; i++ ) {
				c = response[i+1];

				if ( isupper(c) ) {
					c = tolower(c);
				}

				socket_protocol[i] = c;

				if ( c == '\0' )
					break;
			}
			break;
		case 'T':
			if ( strncmp( response, "TST=", 4 ) == 0 ) {
				snprintf( socket_state, sizeof(socket_state),
					"(%s)", response+4 );
			}
			break;
		default:
			break;
		}

		/* Check to see if the next character is 'f'.  If it is,
		 * then we have received all of the lines that we are
		 * going to receive for this file descriptor and it is
		 * time to break out of the loop.
		 */

		c = fgetc( file );

		if ( feof(file) || ferror(file) ) {
			/* There is no more data to receive, so we
			 * break out of the loop.
			 */
			break;
		}

		ungetc( c, file );

		if ( c == 'f' ) {
			break;
		}

	}

	switch( fd_type ) {
	case MXP_LSOF_FILE:
		strlcpy( buffer, filename, buffer_size );
		break;
	case MXP_LSOF_PIPE:
		if ( inode_found == FALSE ) {
			strlcpy( buffer, "pipe:", buffer_size );
		} else {
			mxp_lsof_get_pipe_peer( process_id, inode, &peer_pid,
				peer_command_name, sizeof(peer_command_name) );

			snprintf( buffer, buffer_size,
				"pipe: connected to PID %lu '%s'",
				peer_pid, peer_command_name );
		}
		break;
	case MXP_LSOF_SOCKET:
		snprintf( buffer, buffer_size,
			"%s: %s %s",
			socket_protocol, filename, socket_state );
		break;
	default:
		strlcpy( buffer, "???:", buffer_size );
		break;
	}

	return buffer;
}

#endif

/*=========================================================================*/

#if defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_CYGWIN) \
	|| defined(OS_ANDROID)

/* On Linux, Cygwin, and Android, use readlink() on the /proc/NNN/fd/NNN files.
 * On Solaris, use readlink() on  the /proc/NNN/path/NNN files.
 */

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
			char *buffer, size_t buffer_size )
{
	static char fname[] = "mx_get_fd_name()";

	char fd_pathname[MXU_FILENAME_LENGTH+1];
	int bytes_read, saved_errno;

	if ( fd < 0 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal file descriptor %d passed.", fd );

		return NULL;
	}
	if ( buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL buffer pointer passed." );

		return NULL;
	}
	if ( buffer_size < 1 ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified buffer length of %ld is too short "
		"for the file descriptor name to fit.",
			(long) buffer_size );

		return NULL;
	}

#  if defined(OS_LINUX) || defined(OS_CYGWIN) || defined(OS_ANDROID)
	snprintf( fd_pathname, sizeof(fd_pathname),
		"/proc/%lu/fd/%d",
		process_id, fd );

#  elif defined(OS_SOLARIS)
	snprintf( fd_pathname, sizeof(fd_pathname),
		"/proc/%lu/path/%d",
		process_id, fd );

#  else
#     error readlink() method is not used for this build target.
#  endif

	bytes_read = readlink( fd_pathname, buffer, buffer_size );

	if ( bytes_read >= 0 ) {
		buffer[bytes_read] = '\0';
	} else {
		saved_errno = errno;

		switch( saved_errno ) {
		case ENOENT:
			return NULL;

		default:
			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Could not read the filename corresponding to "
			"file descriptor %d from /proc directory entry '%s'.  "
			"Errno = %d, error message = '%s'",
				fd, fd_pathname,
				saved_errno, strerror(saved_errno) );
			return NULL;
		}
	}

#  if defined(OS_LINUX)

	if ( strncmp( buffer, "socket:", 7 ) == 0 ) {
		(void) mx_get_socket_name_by_fd( fd, buffer, buffer_size );
	}

#  endif /* OS_LINUX */

	return buffer;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

/* The method we use depends on which version of Windows we are running. */

static mx_bool_type tested_for_mx_get_fd_name = FALSE;

/*--- For Windows Vista and newer, we can use GetFinalPathNameByHandle(). ---*/

typedef BOOL (*GetFinalPathNameByHandle_ptr)( HANDLE, LPTSTR, DWORD, DWORD );

static GetFinalPathNameByHandle_ptr
	ptrGetFinalPathNameByHandle = NULL;

#define GFPN_BY_HANDLE_BUFFER_SIZE	(MXU_FILENAME_LENGTH + 3)

/*--- For Windows XP and older, we use GetMappedFileName() from PSAPI.DLL. ---*/

typedef DWORD (*GetMappedFileName_ptr)( HANDLE, LPVOID, LPTSTR, DWORD );

static GetMappedFileName_ptr
	ptrGetMappedFileName = NULL;

/*------*/

static mx_status_type
mxp_get_fd_name_gfpn_by_handle( HANDLE fd_handle,
			char *buffer, size_t buffer_size )
{
	static const char fname[] = "mxp_get_fd_name_gfpn_by_handle()";

	DWORD filename_length, last_error_code;
	TCHAR message_buffer[100];
	TCHAR tchar_filename_buffer[GFPN_BY_HANDLE_BUFFER_SIZE];

#if defined(_UNICODE)
	size_t num_converted;
	int errno_status;
#endif

	filename_length = ptrGetFinalPathNameByHandle( fd_handle,
						tchar_filename_buffer,
						GFPN_BY_HANDLE_BUFFER_SIZE,
						0 );

	if ( filename_length >= GFPN_BY_HANDLE_BUFFER_SIZE ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The filename corresponding to Win32 handle %p "
		"is longer than the supplied buffer size %ld in TCHARs.  "
		"Increase GFPN_BY_HANDLE_BUFFER_SIZE to %ld and recompile MX.",
			fd_handle, (long) GFPN_BY_HANDLE_BUFFER_SIZE,
			filename_length );
	} else
	if ( filename_length == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get filename information for Win32 handle %p.  "
		"Win32 error code = %ld, error message = '%s'.",
			fd_handle, last_error_code, message_buffer );
	}

	/* Convert the wide character filename into single-byte characters. */

#if defined(_UNICODE)
	errno_status = wcstombs_s( &num_converted,
				buffer,
				buffer_size,
				tchar_filename_buffer,
				_TRUNCATE );

	if ( errno_status != 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to convert wide character filename for "
		"file descriptor %d.  "
		"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}
#else
	strlcpy( buffer, tchar_filename_buffer, buffer_size );
#endif

	return MX_SUCCESSFUL_RESULT;
#if 1
	/* FIXME: This function dies with an access violation upon returning
	 * unless this "extra" return is here.  This should not be necessary.
	 */
	return MX_SUCCESSFUL_RESULT;
#endif
}

/*--- For Windows XP and before, we use file mapping objects. ---*/

/* FIXME: For some reason, this code fails with permission errors. */

static mx_status_type
mxp_get_fd_name_via_mapping( HANDLE fd_handle,
			char *buffer, size_t buffer_size )
{
	static const char fname[] = "mxp_get_fd_name_via_mapping()";

	HANDLE file_mapping_handle;
	DWORD file_size_high, file_size_low;
	void *memory_ptr;

	DWORD last_error_code;
	TCHAR message_buffer[100];

	DWORD os_status;
	TCHAR tchar_filename[MXU_FILENAME_LENGTH+1];
	TCHAR mapping_name[100];

	SECURITY_ATTRIBUTES security_attributes;

#if defined(_UNICODE)
	int errno_status;
#endif

#if 0
	SECURITY_INFORMATION security_information;
	SECURITY_DESCRIPTOR *security_descriptor;
#endif

	file_size_high = 0;

	file_size_low  = GetFileSize( fd_handle, &file_size_high );

	if ( (file_size_high == 0) && (file_size_low == 0) ) {
		(void) mx_error( MXE_UNSUPPORTED, fname,
		"Handle %p has file size 0, so we cannot get its name.",
			fd_handle );

		*buffer = '\0';

		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the security descriptor for this handle. */

#if 0
	security_information =
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION
		| DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION
		| LABEL_SECURITY_INFORMATION;

	os_status = GetSecurityInfo( fd_handle, SE_FILE_OBJECT,
					security_information,
					NULL, NULL, NULL, NULL,
					&security_descriptor );

	if ( os_status != ERROR_SUCCESS ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot get the security descriptor for handle %#lx.  "
		"Win32 error code = %ld, error message = '%s'.",
			fd_handle, last_error_code, message_buffer );
		
		strlcpy( buffer, ">>> Access Denied <<<", buffer_size );

		return MX_SUCCESSFUL_RESULT;
	}

	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.lpSecurityDescriptor = security_descriptor;
	security_attributes.bInheritHandle = FALSE;
#endif

#if defined(_UNICODE)
	_snwprintf_s( mapping_name, sizeof(mapping_name)/sizeof(TCHAR),
		"Local\\mx_handle_%#lx", fd_handle );
#else
	snprintf( mapping_name, sizeof(mapping_name),
		"Local\\mx_handle_%p", fd_handle );
#endif

	file_mapping_handle = CreateFileMapping( fd_handle,
#if 0
					NULL,
#else
					&security_attributes,
#endif
					PAGE_READWRITE,
					0,
					1,
					mapping_name );

	if ( file_mapping_handle == NULL ) {
		last_error_code = GetLastError();

		if ( last_error_code == ERROR_ACCESS_DENIED ) {
			strlcpy( buffer, ">>> Access Denied <<<", buffer_size );

			return MX_SUCCESSFUL_RESULT;
		}

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot create file mapping object for handle %p.  "
		"Win32 error code = %ld, error message = '%s'.",
			fd_handle, last_error_code, message_buffer );
		
		*buffer = '\0';

		return MX_SUCCESSFUL_RESULT;
	}

	memory_ptr = MapViewOfFile( file_mapping_handle,
					FILE_MAP_READ, 0, 0, 1 );

	if ( memory_ptr == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		CloseHandle( file_mapping_handle );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot map the file corresponding to handle %p "
		"into our address space.  "
		"Win32 error code = %ld, error message = '%s'.",
			fd_handle, last_error_code, message_buffer );
		
		*buffer = '\0';

		return MX_SUCCESSFUL_RESULT;
	}

	os_status = ptrGetMappedFileName( GetCurrentProcess(),
					memory_ptr,
					tchar_filename,
					MXU_FILENAME_LENGTH );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		UnmapViewOfFile( memory_ptr );
		CloseHandle( file_mapping_handle );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot get the filename for the mapped file "
		"corresponding to handle %p.  "
		"Win32 error code = %ld, error message = '%s'.",
			fd_handle, last_error_code, message_buffer );
		
		*buffer = '\0';

		return MX_SUCCESSFUL_RESULT;
	}

#if defined(_UNICODE)
	errno_status = wcstombs_s( &num_converted,
				buffer,
				buffer_size,
				tchar_filename,
				_TRUNCATE );

	if ( errno_status != 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		UnmapViewOfFile( memory_ptr );
		CloseHandle( file_mapping_handle );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to convert wide character filename for "
		"file descriptor %d.  "
		"Win32 error code = %ld, error message = '%s'.",
			last_error_code , message_buffer );
	}
#else
	strlcpy( buffer, tchar_filename, buffer_size );
#endif

	UnmapViewOfFile( memory_ptr );
	CloseHandle( file_mapping_handle );

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
			char *buffer, size_t buffer_size )
{
	static char fname[] = "mx_get_fd_name()";

	HANDLE fd_handle;
	mx_status_type mx_status;

	if ( fd < 0 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal file descriptor %d passed.", fd );

		return NULL;
	}
	if ( buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL buffer pointer passed." );

		return NULL;
	}
	if ( buffer_size < 1 ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified buffer length of %ld is too short "
		"for the file descriptor name to fit.",
			(long) buffer_size );

		return NULL;
	}

	fd_handle = (HANDLE) _get_osfhandle( fd );

	if ( fd_handle == INVALID_HANDLE_VALUE ) {
		return NULL;
	}
	if ( fd_handle == NULL ) {
		return NULL;
	}

	/* Take care of standard I/O handles. */

	if ( fd_handle == GetStdHandle( STD_INPUT_HANDLE ) ) {
		strlcpy( buffer, "<standard input>", buffer_size );
		return buffer;
	}
	if ( fd_handle == GetStdHandle( STD_OUTPUT_HANDLE ) ) {
		strlcpy( buffer, "<standard output>", buffer_size );
		return buffer;
	}
	if ( fd_handle == GetStdHandle( STD_ERROR_HANDLE ) ) {
		strlcpy( buffer, "<standard error>", buffer_size );
		return buffer;
	}

	/* Test for the existence of file information functions. */

	if ( tested_for_mx_get_fd_name == FALSE ) {

		/* First, we look for GetFinalPathNameByHandle(). */

		HMODULE hinst_kernel32;

		tested_for_mx_get_fd_name = TRUE;

		hinst_kernel32 = GetModuleHandle(TEXT("kernel32.dll"));

		if ( hinst_kernel32 == NULL ) {
			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to get a module handle for KERNEL32.DLL failed.  "
		"This should NEVER, EVER happen.  In fact, you should not be "
		"reading this message, since the computer should have "
		"crashed by now.  If you DO see this message, let "
		"William Lavender know." );
			return NULL;
		}

#if defined(_UNICODE)
		ptrGetFinalPathNameByHandle =
			(GetFinalPathNameByHandle_ptr)
			GetProcAddress( hinst_kernel32,
				TEXT("GetFinalPathNameByHandleW") );
#else
		ptrGetFinalPathNameByHandle =
			(GetFinalPathNameByHandle_ptr)
			GetProcAddress( hinst_kernel32,
				TEXT("GetFinalPathNameByHandleA") );
#endif

		if ( ptrGetFinalPathNameByHandle == NULL ) {
			/* Next, we try GetMappedFileName(). */

#if defined(_UNICODE)
			mx_status = mx_dynamic_library_get_library_and_symbol(
				"psapi.dll", "GetMappedFileNameW",
				NULL, (void **) &ptrGetMappedFileName, 0 );
#else
			mx_status = mx_dynamic_library_get_library_and_symbol(
				"psapi.dll", "GetMappedFileNameA",
				NULL, (void **) &ptrGetMappedFileName, 0 );
#endif
			if ( mx_status.code != MXE_SUCCESS )
				return NULL;
		}
	}

	/* Now we branch out to the various version specific methods. */

	if ( ptrGetFinalPathNameByHandle != NULL ) {

		/* Windows Vista and above. */

		mx_status = mxp_get_fd_name_gfpn_by_handle( fd_handle,
						buffer, buffer_size );
	} else
	if ( ptrGetMappedFileName != NULL ) {
		/* Windows XP and below. */

		mx_status = mxp_get_fd_name_via_mapping( fd_handle,
						buffer, buffer_size );
	} else {
		snprintf( buffer, buffer_size, "FD %d", fd );

		mx_status = MX_SUCCESSFUL_RESULT;
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		return NULL;
	} else {
		return buffer;
	}
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void
mx_win32_show_socket_names( void )
{
	MX_SOCKET_FD i, max_sockets, num_open_sockets, increment;
	char buffer[MXU_FILENAME_LENGTH+20];
	int os_major, os_minor, os_update;
	mx_status_type mx_status;

	/*---*/

	mx_status = mx_get_os_version( &os_major, &os_minor, &os_update );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	/* For most versions of Win32, socket descriptors are multiples
	 * of 4, like 384 or 400.  However, if a socket descriptor has
	 * (for example) a value like 384, then descriptor values like 385,
	 * 386, and 387 will also appear to be active sockets, but with
	 * the same parameters as socket 384.  To avoid these duplicates,
	 * we only look at socket descriptors that are a multiple of 4,
	 * except for platforms like Windows 95, where sockets can have
	 * any value.
	 */

	increment = 4;

	switch( os_major ) {
	case 3:
		increment = 1;
		break;
	case 4:
		switch( os_minor ) {
		case 0:
		case 1:
		case 3:
			if ( os_update != VER_PLATFORM_WIN32_NT ) {
				increment = 1;      /* Windows 95 */
			}
			break;
		}
		break;
	}

	/*---*/

	max_sockets = 65536;	/* FIXME: May require more investigation. */

	num_open_sockets = 0;

	for ( i = 0; i < max_sockets; i += increment ) {
		mx_status = mx_get_socket_name_by_fd(
					i, buffer, sizeof(buffer) );

		if ( mx_status.code == MXE_SUCCESS ) {
			num_open_sockets++;

			mx_info( "%d - %s", (int) i, buffer );
		}
	}

	mx_info( "num_open_sockets = %d", (int) num_open_sockets );

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_MACOSX) || defined(OS_BSD) || defined(OS_UNIXWARE)

/* Use the external 'lsof' program to get the fd name. */

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
		char *buffer, size_t buffer_size )
{
	static const char fname[] = "mxp_get_fd_name_from_lsof()";

	FILE *file;
	int saved_errno;
	char command[ MXU_FILENAME_LENGTH + 20 ];
	char response[80];
	char *ptr;

	snprintf( command, sizeof(command),
		"lsof -a -p%lu -d%d -n -P -FfatDinPT",
		process_id, fd );

	file = popen( command, "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to run the command '%s'.  "
		"Errno = %d, error message = '%s'",
			command, saved_errno, strerror(saved_errno) );

		return NULL;
	}

	mx_fgets( response, sizeof(response), file );

	if ( feof(file) || ferror(file) ) {
		/* Did not get any output from lsof. */

		pclose(file);
		return NULL;
	}

	if ( response[0] != 'p' ) {
		/* This file descriptor is not open. */

		pclose(file);
		return NULL;
	}

	ptr = mxp_parse_lsof_output( file, process_id, fd,
					buffer, buffer_size );

	pclose(file);

	return ptr;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_VMS)

#include <unixio.h>

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
		char *buffer, size_t buffer_size )
{
	static char fname[] = "mx_get_fd_name()";

	char *ptr;
	char local_buffer[MXU_FILENAME_LENGTH+1];

	if ( fd < 0 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal file descriptor %d passed.", fd );

		return NULL;
	}
	if ( buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL buffer pointer passed." );

		return NULL;
	}

	ptr = getname( fd, local_buffer );

	if ( ptr == NULL ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
	    "An error occurred when getting the name of file descriptor %d.",
			fd );

		return NULL;
	}

	strlcpy( buffer, local_buffer, buffer_size );

	return buffer;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_QNX) || defined(OS_VXWORKS) || defined(OS_RTEMS) \
	|| defined(OS_DJGPP) || defined(OS_HURD) || defined(OS_MINIX)

MX_EXPORT char *
mx_get_fd_name( unsigned long process_id, int fd,
		char *buffer, size_t buffer_size )
{
	static char fname[] = "mx_get_fd_name()";

	if ( fd < 0 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal file descriptor %d passed.", fd );

		return NULL;
	}
	if ( buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL buffer pointer passed." );

		return NULL;
	}
	if ( buffer_size < 1 ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The specified buffer length of %ld is too short "
		"for the file descriptor name to fit.",
			(long) buffer_size );

		return NULL;
	}

	snprintf( buffer, buffer_size, "FD %d", fd );

	return buffer;
}

#else

#error mx_get_fd_name() has not yet been written for this platform.

#endif

/*=========================================================================*/

#if 0

MX_EXPORT void
mx_show_fd_names( unsigned long process_id )
{
	static const char fname[] = "mx_show_fd_names()";

	FILE *file;
	char command[MXU_FILENAME_LENGTH+20];
	char response[80];
	char buffer[MXU_FILENAME_LENGTH+20];
	char *ptr;
	int max_fds, num_open_fds, saved_errno;

	max_fds = mx_get_max_file_descriptors();

	snprintf( command, sizeof(command),
		"lsof -a -p%lu -d0-%d -n -P -FfatDinPT",
		process_id, max_fds );

	file = popen( command, "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to run the command '%s'.  "
		"Errno = %d, error message = '%s'",
			command, saved_errno, strerror(saved_errno) );

		return;
	}

	mx_fgets( response, sizeof(response), file );

	if ( feof(file) || ferror(file) ) {
		/* Did not get any output from lsof. */

		pclose(file);
		return;
	}

	if ( response[0] != 'p' ) {
		/* This file descriptor is not open. */

		pclose(file);
		return;
	}

	while (1) {
		if ( feof(file) || ferror(file) ) {
			break;
		}

		ptr = mxp_parse_lsof_output( file, process_id, -1,
					buffer, sizeof(buffer) );

		if ( ptr != NULL ) {
			mx_info( "%s", ptr );
		}
	}

	num_open_fds = mx_get_number_of_open_file_descriptors();

	pclose(file);

	mx_info( "max_fds = %d, num_open_fds = %d", max_fds, num_open_fds );

	return;
}

#else

MX_EXPORT void
mx_show_fd_names( unsigned long process_id )
{
	int i, max_fds, num_open_fds;
	char *ptr;
	char buffer[MXU_FILENAME_LENGTH+20];

	max_fds = mx_get_max_file_descriptors();

	num_open_fds = 0;

	for ( i = 0; i < max_fds; i++ ) {
		ptr = mx_get_fd_name( process_id, i, buffer, sizeof(buffer) );

		if ( ptr != NULL ) {
			num_open_fds++;

			mx_info( "%d - %s", i, ptr );
		}
	}

	mx_info( "max_fds = %d, num_open_fds = %d", max_fds, num_open_fds );

	return;
}

#endif

/*=========================================================================*/

#if defined(OS_ANDROID) || ( defined(OS_LINUX) \
	&& ( ( MX_GLIBC_VERSION > 2003006L ) || defined( MX_MUSL_VERSION ) ) )

	/*** Use Linux inotify via Glibc > 2.3.6 or musl ***/

#include <sys/inotify.h>
#include <stddef.h>

#define MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH \
			( 1024 * (sizeof(struct inotify_event) + 16) )

typedef struct {
	int inotify_file_descriptor;
	int inotify_watch_descriptor;
	char event_buffer[MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH];

} MXP_LINUX_INOTIFY_MONITOR;

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			const char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_LINUX_INOTIFY_MONITOR *linux_monitor;
	unsigned long flags, quiet_flag;
	int saved_errno;
	mx_status_type mx_status;

	if ( monitor_ptr == (MX_FILE_MONITOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	*monitor_ptr = (MX_FILE_MONITOR *) malloc( sizeof(MX_FILE_MONITOR) );

	if ( (*monitor_ptr) == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_FILE_MONITOR structure." );
	}

	linux_monitor = (MXP_LINUX_INOTIFY_MONITOR *)
				malloc( sizeof(MXP_LINUX_INOTIFY_MONITOR) );

	if ( linux_monitor == (MXP_LINUX_INOTIFY_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_LINUX_INOTIFY_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = linux_monitor;

	strlcpy( (*monitor_ptr)->filename, filename, MXU_FILENAME_LENGTH );

	/*----*/

	linux_monitor->inotify_file_descriptor = inotify_init();

	if ( linux_monitor->inotify_file_descriptor < 0 ) {
		saved_errno = errno;

		mx_free( linux_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while invoking inotify_init().  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	/* FIXME: For the moment we ignore the value of 'access_type'. */

	flags = IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF;

	linux_monitor->inotify_watch_descriptor = inotify_add_watch(
					linux_monitor->inotify_file_descriptor,
					filename, flags );

	if ( linux_monitor->inotify_watch_descriptor < 0 ) {
		saved_errno = errno;

		mx_free( linux_monitor );
		mx_free( *monitor_ptr );
		*monitor_ptr = NULL;

		if ( access_type & MXF_FILE_MONITOR_QUIET ) {
			quiet_flag = MXE_QUIET;
		} else {
			quiet_flag = 0;
		}

		switch( saved_errno ) {
		case ENOENT:
			mx_status = mx_error( MXE_NOT_FOUND | quiet_flag, fname,
				"File '%s' not found.  No monitor created.",
				filename );
			break;

		default:
			mx_status = mx_error(
					MXE_FILE_IO_ERROR | quiet_flag, fname,
			"An error occurred while invoking inotify_add_watch() "
			"for file '%s'.  Errno = %d, error message = '%s'",
				filename, saved_errno, strerror(saved_errno) );
			break;
		}

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_LINUX_INOTIFY_MONITOR *linux_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	linux_monitor = monitor->private_ptr;

	if ( linux_monitor == (MXP_LINUX_INOTIFY_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_LINUX_INOTIFY_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	(void) close( linux_monitor->inotify_file_descriptor );

	mx_free( linux_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

/* The following was inspired by
 *
http://unix.derkeiler.com/Newsgroups/comp.unix.programmer/2011-03/msg00059.html
 *
 * but is structured differently.  Much of the complexity is to deal with
 * pointer alignment issues on some platforms like ARM.
 */

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_LINUX_INOTIFY_MONITOR *linux_monitor;
	size_t num_bytes_available;
	ssize_t read_length;
	int saved_errno;
	char *buffer_ptr, *end_of_buffer_ptr;
	union {
		struct inotify_event event;
		char padding[ roundup(
				offsetof(struct inotify_event,name)
				+ NAME_MAX + 1,
					__alignof__(struct inotify_event) ) ];
	} u;
	uint32_t event_length;
	char *len_ptr;
	mx_status_type mx_status;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	linux_monitor = monitor->private_ptr;

	if ( linux_monitor == (MXP_LINUX_INOTIFY_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_LINUX_INOTIFY_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	/* How many bytes have been sent to the inotify file descriptor? */

	mx_status = mx_fd_num_input_bytes_available(
				linux_monitor->inotify_file_descriptor,
				&num_bytes_available );

	if ( mx_status.code != MXE_SUCCESS ) {
		return FALSE;
	}

	if ( num_bytes_available == 0 ) {
		return FALSE;
	}

	if ( num_bytes_available > MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH ) {
		num_bytes_available = MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH;
	}

	/* We only want to read complete event structures here, so we
	 * leave any not yet completed event structures in the fd buffer.
	 */

	read_length = read( linux_monitor->inotify_file_descriptor,
			linux_monitor->event_buffer,
			num_bytes_available );

	if ( read_length < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while reading from inotify fd %d "
		"for filename '%s'.  Errno = %d, error message = '%s'.",
			linux_monitor->inotify_file_descriptor,
			monitor->filename,
			saved_errno, strerror(saved_errno) );
	} else
	if ( read_length < num_bytes_available ) {
		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Short read of %lu bytes seen when %lu bytes were expected "
		"from inotify fd %d for file '%s'",
			(unsigned long) read_length,
			(unsigned long) num_bytes_available,
			linux_monitor->inotify_file_descriptor,
			monitor->filename );
	}

	end_of_buffer_ptr = linux_monitor->event_buffer
				+ MXP_LINUX_INOTIFY_EVENT_BUFFER_LENGTH;

	buffer_ptr = linux_monitor->event_buffer;

	while ( buffer_ptr < end_of_buffer_ptr ) { 
	    len_ptr = buffer_ptr + offsetof(struct inotify_event, len);

	    memcpy( &event_length, len_ptr, sizeof(event_length) );

	    event_length += offsetof( struct inotify_event, name );

	    memcpy( &(u.event), buffer_ptr, event_length );

	    if ( linux_monitor->inotify_watch_descriptor == u.event.wd ){
		return TRUE;
	    }

	    buffer_ptr += event_length;
	}

	return FALSE;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

	/*** Use Win32 FindFirstChangeNotification() ***/

#include <windows.h>

/* Note: We use a Unix-style 'struct _stat' structure here instead of a
 * WIN32_FILE_ATTRIBUTE_DATA structure, because the _stat() function
 * exists in all versions of Win32, but GetFileAttributesEx() only exists
 * in Windows XP and later.
 */

typedef struct {
	HANDLE change_handle;
	struct _stat file_status;
} MXP_WIN32_FILE_MONITOR;

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			const char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_WIN32_FILE_MONITOR *win32_monitor;
	char *directory_name, *separator_ptr;
	int os_status, saved_errno;
	unsigned long flags, quiet_flag;
	mx_status_type mx_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( monitor_ptr == (MX_FILE_MONITOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	*monitor_ptr = (MX_FILE_MONITOR *) malloc( sizeof(MX_FILE_MONITOR) );

	if ( (*monitor_ptr) == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_FILE_MONITOR structure." );
	}

	win32_monitor = (MXP_WIN32_FILE_MONITOR *)
				malloc( sizeof(MXP_WIN32_FILE_MONITOR) );

	if ( win32_monitor == (MXP_WIN32_FILE_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_WIN32_FILE_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = win32_monitor;

	strlcpy( (*monitor_ptr)->filename, filename, MXU_FILENAME_LENGTH );

	/*----*/

	/* Create a directory name from the specified filename.  If the
	 * filename contains a path separator, then we truncate it at
	 * the last separator and call that the directory name.  If the
	 * filename does _NOT_ contain a path separator, then we use
	 * '.' as the directory name.
	 */ 

	directory_name = strdup( filename );

	if ( directory_name == NULL ) {
		mx_free( win32_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a copy of the "
		"filename '%s'.", filename );
	}

	separator_ptr = strrchr( directory_name, '\\' );

	if ( separator_ptr != NULL ) {
		*separator_ptr = '\0';
	} else {
		separator_ptr = strrchr( directory_name, '/' );

		if ( separator_ptr != NULL ) {
			*separator_ptr = '\0';
		} else {
			mx_free( directory_name );

			directory_name = strdup( "." );

			if ( directory_name == NULL ) {
				mx_free( win32_monitor );
				mx_free( *monitor_ptr );

				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to allocate "
				"memory for the string '.' to use as a "
				"directory name." );
			}
		}
	}

	/* Save a copy of the initial file status for this filename. */

	os_status = _stat( filename, &(win32_monitor->file_status) );

	if ( os_status != 0 ) {
		saved_errno = errno;

		mx_free( win32_monitor );
		mx_free( *monitor_ptr );
		*monitor_ptr = NULL;

		if ( access_type & MXF_FILE_MONITOR_QUIET ) {
			quiet_flag = MXE_QUIET;
		} else {
			quiet_flag = 0;
		}

		switch( saved_errno ) {
		case ENOENT:
			mx_status = mx_error( MXE_NOT_FOUND | quiet_flag, fname,
				"File '%s' not found.  No monitor created.",
				filename );
			break;

		default:
			mx_status =  mx_error(
					MXE_FILE_IO_ERROR | quiet_flag, fname,
			"An attempt to get the current file status of file "
			"'%s' failed with errno = %d, error message = '%s'.",
				filename, saved_errno, strerror(saved_errno) );
			break;
		}

		return mx_status;
	}

	/* FIXME: For the moment we ignore the value of 'access_type'. */

	flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME
		| FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE
		| FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY;

	win32_monitor->change_handle = FindFirstChangeNotification(
					directory_name, FALSE, flags );

	if ( win32_monitor->change_handle == INVALID_HANDLE_VALUE ) {
		last_error_code = GetLastError();

		mx_free( win32_monitor );
		mx_free( *monitor_ptr );

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while invoking "
		"FindFirstChangeNotifcation() for directory '%s'.  "
		"Win32 error code = %ld, error message = '%s'",
			directory_name, last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_WIN32_FILE_MONITOR *win32_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	win32_monitor = monitor->private_ptr;

	if ( win32_monitor == (MXP_WIN32_FILE_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_WIN32_FILE_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	(void) FindCloseChangeNotification( win32_monitor->change_handle );

	mx_free( win32_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_WIN32_FILE_MONITOR *win32_monitor;
	struct _stat current_file_status;
	int os_status, saved_errno;
	BOOL find_status;
	DWORD wait_status, last_error_code;
	TCHAR message_buffer[100];
	mx_bool_type file_has_changed;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	win32_monitor = monitor->private_ptr;

	if ( win32_monitor == (MXP_WIN32_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_WIN32_FILE_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	/* Is a change notification available? */

	wait_status = WaitForSingleObject( win32_monitor->change_handle, 0 );

	switch( wait_status ) {
	case WAIT_OBJECT_0:
		/* If we get here, then _some_ file changed in the directory
		 * that contains our file, but it may not be our file.  Thus,
		 * we need to get the file status, so that we can compare it
		 * with our previously saved version of the status.
		 */

		os_status = _stat( monitor->filename, &current_file_status );

		if ( os_status != 0 ) {
			saved_errno = errno;

			if ( saved_errno == ENOENT ) {
				file_has_changed = TRUE;
			} else {
				file_has_changed = FALSE;

				(void) mx_error( MXE_FILE_IO_ERROR, fname,
				"An attempt to get the current file status "
				"of file '%s' failed with error = %d, "
				"error message = '%s'.",
					monitor->filename,
					saved_errno, strerror(saved_errno) );
			}
		} else {

			if ( current_file_status.st_mtime
				!= (win32_monitor->file_status.st_mtime) )
			{
				file_has_changed = TRUE;
			} else {
				file_has_changed = FALSE;
			}

			/* Save a copy of the current file status. */

			memcpy( &(win32_monitor->file_status),
				&current_file_status,
				sizeof(struct _stat) );
		}
		break;

	case WAIT_TIMEOUT:
		file_has_changed = FALSE;
		break;

	default:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"WaitForSingleObject( %p, 0 ) for file '%s' failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			win32_monitor->change_handle,
			monitor->filename,
			last_error_code, message_buffer );

		file_has_changed = FALSE;
		break;
	}

	/* Restart the notification. */

	find_status =
	    FindNextChangeNotification( win32_monitor->change_handle );

	if( find_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while invoking "
			"FindNextChangeNotifcation() for the directory "
			"containing file '%s'.  "
			"Win32 error code = %ld, error message = '%s'",
				monitor->filename,
				last_error_code, message_buffer );
	}

	/* Return the file change status. */

	return file_has_changed;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_MACOSX) || defined(OS_BSD)

#include <sys/event.h>
#include <sys/time.h>

typedef struct {
	int kqueue_fd;
	int file_fd;
} MXP_KQUEUE_VNODE_MONITOR;

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			const char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_KQUEUE_VNODE_MONITOR *kqueue_monitor;
	int num_events, saved_errno;
	long quiet_flag;
	struct kevent event;
	mx_status_type mx_status;

	if ( monitor_ptr == (MX_FILE_MONITOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	*monitor_ptr = (MX_FILE_MONITOR *) malloc( sizeof(MX_FILE_MONITOR) );

	if ( (*monitor_ptr) == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_FILE_MONITOR structure." );
	}

	kqueue_monitor = (MXP_KQUEUE_VNODE_MONITOR *)
				malloc( sizeof(MXP_KQUEUE_VNODE_MONITOR) );

	if ( kqueue_monitor == (MXP_KQUEUE_VNODE_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_KQUEUE_VNODE_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = kqueue_monitor;

	strlcpy( (*monitor_ptr)->filename, filename, MXU_FILENAME_LENGTH );

	/*----*/

	kqueue_monitor->kqueue_fd = kqueue();

	if ( kqueue_monitor->kqueue_fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Cannot create a new BSD kqueue event object.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	}

	kqueue_monitor->file_fd = open( filename, O_RDONLY );

	if ( kqueue_monitor->file_fd < 0 ) {
		saved_errno = errno;

		close( kqueue_monitor->kqueue_fd );
		mx_free( kqueue_monitor );
		mx_free( *monitor_ptr );
		*monitor_ptr = NULL;

		if ( access_type & MXF_FILE_MONITOR_QUIET ) {
			quiet_flag = MXE_QUIET;
		} else {
			quiet_flag = 0;
		}

		switch( saved_errno ) {
		case ENOENT:
			mx_status = mx_error( MXE_NOT_FOUND | quiet_flag, fname,
				"File '%s' not found.  No monitor created.",
				filename );
			break;

		default:
			mx_status = mx_error(
					MXE_FILE_IO_ERROR | quiet_flag, fname,
				"Cannot open file '%s'.  "
				"Errno = %d, error message = '%s'",
				filename, saved_errno, strerror(saved_errno) );
			break;
		}

		return mx_status;
	}

	/* FIXME: For the moment we ignore the value of 'access_type'. */

	/* Add this event to the kqueue. */

	event.ident = kqueue_monitor->file_fd;
	event.filter = EVFILT_VNODE;
	event.flags = EV_ADD | EV_ENABLE;
	event.fflags = NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB;
	event.data = 0;
	event.udata = 0;

	num_events = kevent( kqueue_monitor->kqueue_fd,
				&event, 1, NULL, 0, NULL );

	if ( num_events < 0 ) {
		saved_errno = errno;

		close( kqueue_monitor->kqueue_fd );
		mx_free( kqueue_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Cannot monitor changes to the file '%s'.  "
			"Errno = %d, error message = '%s'.",
				filename, saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_KQUEUE_VNODE_MONITOR *kqueue_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	kqueue_monitor = monitor->private_ptr;

	if ( kqueue_monitor == (MXP_KQUEUE_VNODE_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_KQUEUE_VNODE_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	close( kqueue_monitor->kqueue_fd );
	close( kqueue_monitor->file_fd );

	mx_free( kqueue_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_KQUEUE_VNODE_MONITOR *kqueue_monitor;
	struct kevent event;
	int num_events;
	struct timespec timeout;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	kqueue_monitor = monitor->private_ptr;

	if ( kqueue_monitor == (MXP_KQUEUE_VNODE_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_KQUEUE_VNODE_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	timeout.tv_sec = 0;
	timeout.tv_nsec = 0;

	num_events = kevent( kqueue_monitor->kqueue_fd,
				NULL, 0,
				&event, 1,
				&timeout );

#if 0
	if ( num_events < 0 ) {
		int saved_errno;

		saved_errno = errno;
	} else
#endif

	if ( num_events > 0 ) {
		return TRUE;
	}

	return FALSE;
}

/*-------------------------------------------------------------------------*/

#elif ( defined(OS_SOLARIS) \
        && (MX_SOLARIS_VERSION >= 5011000L) \
        && (!defined(__GNUC__)) )

/*
 * https://blogs.oracle.com/darren/entry/file_notification_in_opensolaris_and
 * contains a basic description of how to use port_associate() and friends.
 */

#include <port.h>

typedef struct {
	int port;
	file_obj_t file_object;
	int events_to_monitor;
} MXP_SOLARIS_PORT_MONITOR;

static mx_status_type
mxp_solaris_port_associate( MX_FILE_MONITOR *file_monitor,
				unsigned long quiet_flag )
{
	static const char fname[] = "mxp_solaris_port_associate()";

	MXP_SOLARIS_PORT_MONITOR *port_monitor;
	file_obj_t *file_object;
	int os_status, saved_errno;
	mx_status_type mx_status;

	port_monitor = file_monitor->private_ptr;

	file_object = &(port_monitor->file_object);

	file_object->fo_name = file_monitor->filename;

	os_status = port_associate( port_monitor->port,
				PORT_SOURCE_FILE,
				(uintptr_t) file_object,
				port_monitor->events_to_monitor,
				NULL );

	if ( os_status == (-1) ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case ENOENT:
			mx_status = mx_error( MXE_NOT_FOUND | quiet_flag, fname,
				"File '%s' not found.  No monitor created.",
				file_monitor->filename );
			break;

		default:
			mx_status = mx_error( 
					MXE_FILE_IO_ERROR | quiet_flag, fname,
			"A call to port_associate() for file '%s' failed.  "
			"Errno = %d, error message = '%s'.",
				file_monitor->filename,
				saved_errno, strerror(saved_errno) );
			break;
		}

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			const char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_SOLARIS_PORT_MONITOR *port_monitor;
	int saved_errno;
	mx_status_type mx_status;

	if ( monitor_ptr == (MX_FILE_MONITOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	*monitor_ptr = (MX_FILE_MONITOR *) malloc( sizeof(MX_FILE_MONITOR) );

	if ( (*monitor_ptr) == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_FILE_MONITOR structure." );
	}

	port_monitor = (MXP_SOLARIS_PORT_MONITOR *)
				malloc( sizeof(MXP_SOLARIS_PORT_MONITOR) );

	if ( port_monitor == (MXP_SOLARIS_PORT_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_SOLARIS_PORT_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = port_monitor;

	strlcpy( (*monitor_ptr)->filename, filename, MXU_FILENAME_LENGTH );

	/*----*/

	port_monitor->port = port_create();

	if ( port_monitor->port == (-1) ) {
		saved_errno = errno;

		mx_free( port_monitor );
		mx_free( *monitor_ptr );

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while invoking port_create().  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	port_monitor->events_to_monitor = FILE_MODIFIED;

	memset( &(port_monitor->file_object), 0, sizeof(file_obj_t) );

	mx_status = mxp_solaris_port_associate( *monitor_ptr, FALSE );

	if ( mx_status.code != MXE_SUCCESS ) {
		close( port_monitor->port );

		mx_free( port_monitor );
		mx_free( *monitor_ptr );
		*monitor_ptr = NULL;

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_SOLARIS_PORT_MONITOR *port_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	port_monitor = monitor->private_ptr;

	if ( port_monitor == (MXP_SOLARIS_PORT_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_SOLARIS_PORT_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	(void) close( port_monitor->port );

	mx_free( port_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_SOLARIS_PORT_MONITOR *port_monitor;
	port_event_t port_event;
	timespec_t timeout;
	int os_status, saved_errno;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	port_monitor = monitor->private_ptr;

	if ( port_monitor == (MXP_SOLARIS_PORT_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_SOLARIS_PORT_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	timeout.tv_sec = 0;
	timeout.tv_nsec = 0;

	os_status = port_get( port_monitor->port, &port_event, &timeout );

	if ( os_status == 0 ) {
		return TRUE;
	} else
	if ( os_status == (-1) ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case ETIME:
		case EINTR:
			return FALSE;
			break;
		default:
			(void) mx_error( MXE_FILE_IO_ERROR, fname,
		    "Checking for file status changes for file '%s' failed.  "
		    "Errno = %d, error message = '%s'.",
				monitor->filename,
				saved_errno, strerror(saved_errno) );

			return FALSE;
			break;
		}
	}

	return FALSE;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_HURD) \
	|| defined(OS_UNIXWARE) || defined(OS_CYGWIN) || defined(OS_VXWORKS) \
	|| defined(OS_QNX) || defined(OS_VMS) || defined(OS_DJGPP) \
	|| defined(OS_RTEMS) || defined(OS_MINIX)

/*
 * This is a generic stat()-based implementation that requires polling.
 * It is used for the following platforms:
 *
 *   DJGPP
 *   Gnu Hurd.
 *   Linux with Glibc 2.3.5 and before.
 *   OpenVMS.
 *   QNX
 *   RTEMS
 *   Solaris 9 and before.
 *   VxWorks.
 *   Other Unix-like platforms.
 */

typedef struct {
	struct stat stat_buf;
} MXP_STAT_MONITOR;

MX_EXPORT mx_status_type
mx_create_file_monitor( MX_FILE_MONITOR **monitor_ptr,
			unsigned long access_type,
			const char *filename )
{
	static const char fname[] = "mx_create_file_monitor()";

	MXP_STAT_MONITOR *stat_monitor;
	unsigned long quiet_flag;
	int os_status, saved_errno;
	mx_status_type mx_status;

	if ( monitor_ptr == (MX_FILE_MONITOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	*monitor_ptr = (MX_FILE_MONITOR *) malloc( sizeof(MX_FILE_MONITOR) );

	if ( (*monitor_ptr) == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_FILE_MONITOR structure." );
	}

	stat_monitor = (MXP_STAT_MONITOR *)
				malloc( sizeof(MXP_STAT_MONITOR) );

	if ( stat_monitor == (MXP_STAT_MONITOR *) NULL ) {
		mx_free( *monitor_ptr );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MXP_STAT_MONITOR structure." );
	}

	(*monitor_ptr)->private_ptr = stat_monitor;

	strlcpy( (*monitor_ptr)->filename, filename, MXU_FILENAME_LENGTH );

	/*----*/

	/* Save a copy of the initial status of the file. */

	os_status = stat( (*monitor_ptr)->filename, &(stat_monitor->stat_buf) );

	if ( os_status != 0 ) {
		saved_errno = errno;

		mx_free( stat_monitor );
		mx_free( *monitor_ptr );
		*monitor_ptr = NULL;

		if ( access_type & MXF_FILE_MONITOR_QUIET ) {
			quiet_flag = MXE_QUIET;
		} else {
			quiet_flag = 0;
		}

		switch( saved_errno ) {
		case ENOENT:
			mx_status = mx_error( MXE_NOT_FOUND | quiet_flag, fname,
				"File '%s' not found.  No monitor created.",
				filename );
			break;

		default:
			mx_status = mx_error(
					MXE_FILE_IO_ERROR | quiet_flag, fname,
			"An error occurred while invoking stat() for "
			"file '%s'.  Errno = %d, error message = '%s'",
				filename, saved_errno, strerror(saved_errno) );
			break;
		}

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_delete_file_monitor( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_delete_file_monitor()";

	MXP_STAT_MONITOR *stat_monitor;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );
	}

	stat_monitor = monitor->private_ptr;

	if ( stat_monitor == (MXP_STAT_MONITOR *) NULL ) {
		mx_free( monitor );

		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_STAT_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );
	}

	mx_free( stat_monitor );
	mx_free( monitor );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_file_has_changed( MX_FILE_MONITOR *monitor )
{
	static const char fname[] = "mx_file_has_changed()";

	MXP_STAT_MONITOR *stat_monitor;
	int os_status, saved_errno;
	mx_bool_type result;
	struct stat current_stat_buf;

	if ( monitor == (MX_FILE_MONITOR *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_FILE_MONITOR pointer passed was NULL." );

		return FALSE;
	}

	stat_monitor = monitor->private_ptr;

	if ( stat_monitor == (MXP_STAT_MONITOR *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MXP_STAT_MONITOR pointer "
			"for MX_FILE_MONITOR %p is NULL.", monitor );

		return FALSE;
	}

	/* Get the current status of the file. */

	os_status = stat( monitor->filename, &current_stat_buf );

	if ( os_status != 0 ) {
		saved_errno = errno;

		if ( saved_errno == ENOENT )
			return TRUE;

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while invoking stat() for file '%s'.  "
		"Errno = %d, error message = '%s'",
			monitor->filename,
			saved_errno, strerror( saved_errno ) );

		return FALSE;
	}

	/* Has the file been modified since the last time that we looked? */

	if ( current_stat_buf.st_mtime != stat_monitor->stat_buf.st_mtime ) {
		result = TRUE;
	} else {
		result = FALSE;
	}

	/* Save the new status. */

	memcpy( &(stat_monitor->stat_buf), &current_stat_buf,
			sizeof(struct stat) );

	return result;
}

/*-------------------------------------------------------------------------*/

#else

#error MX file monitors not yet implemented for this platform.

#endif

