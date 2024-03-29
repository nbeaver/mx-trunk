/*  $Revision: 1.3 $
**
**  Internal header file for editline library.
*/
#include <stdio.h>
#if	defined(HAVE_STDLIB)
#include <stdlib.h>
#include <string.h>
#endif	/* defined(HAVE_STDLIB) */
#if	defined(SYS_UNIX)
#include "unix.h"
#endif	/* defined(SYS_UNIX) */
#if	defined(SYS_OS9)
#include "os9.h"
#endif	/* defined(SYS_OS9) */

#if	!defined(SIZE_T)
#define SIZE_T	unsigned int
#endif	/* !defined(SIZE_T) */

typedef unsigned char	CHAR;

#if	defined(HIDE)
#define STATIC	static
#else
#define STATIC	/* NULL */
#endif	/* !defined(HIDE) */

#if	!defined(CONST)
#if	defined(__STDC__)
#define CONST	const
#else
#define CONST
#endif	/* defined(__STDC__) */
#endif	/* !defined(CONST) */


#define MEM_INC		64
#define SCREEN_INC	256

#define DISPOSE(p)	free((char *)(p))
#define NEW(T, c)	\
	((T *)malloc((unsigned int)(sizeof (T) * (c))))
#define RENEW(p, T, c)	\
	(p = (T *)realloc((char *)(p), (unsigned int)(sizeof (T) * (c))))
#define COPYFROMTO(new, p, len)	\
	(void)memcpy((char *)(new), (char *)(p), (int)(len))

#if !defined(__APPLE__)
/* strlcpy() and strlcat() definitions added by William Lavender (2015-07-28) */

extern size_t strlcpy( char *dest, const char *src, size_t maxlen );
extern size_t strlcat( char *dest, const char *src, size_t maxlen );

#endif


/*
**  Variables and routines internal to this package.
*/
extern int	rl_eof;
extern int	rl_erase;
extern int	rl_intr;
extern int	rl_kill;
extern int	rl_quit;
extern char	*rl_complete( char *, int * );
extern int	rl_list_possib( char *, char *** );
extern void	rl_ttyset( int );
extern void	rl_add_slash( char *, char * );

#if	!defined(HAVE_STDLIB)
extern char	*getenv();
extern char	*malloc();
extern char	*realloc();
extern char	*memcpy();
extern char	*strcat();
extern char	*strchr();
extern char	*strrchr();
extern char	*strcpy();
extern char	*strdup();
extern int	strcmp();
extern int	strlen();
extern int	strncmp();
#endif	/* !defined(HAVE_STDLIB) */
