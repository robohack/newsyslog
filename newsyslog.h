#ident "@(#)newsyslog:$Name:  $:$Id: newsyslog.h,v 1.2 2007/07/24 18:25:43 woods Exp $"

/*
 * various portability related defintions
 *
 * Include this file after all other includes have been done.
 */

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN	255
#endif

#ifndef PATH_MAX
# ifdef MAXPATHLEN
#  define PATH_MAX	MAXPATHLEN
# else
#  define PATH_MAX	1024
# endif
#endif /* PATH_MAX */

#ifndef TRUE				/* XXX should be #undef ??? */
# define TRUE		1
#endif
#ifndef FALSE				/* XXX should be #undef ??? */
# define FALSE		0
#endif

/*
 * MIN_PID & MAX_PID are used to sanity-check the pid_file contents.
 */
#ifndef MIN_PID
# define MIN_PID	5		/* probably a sane number... */
#endif
#ifndef MAX_PID
# ifdef MAXPID
#  define MAX_PID	MAXPID
# else
#  ifdef PID_MAX
#   define MAX_PID	PID_MAX
#  else
#   ifdef __linux__
#    define MAX_PID	0x8000		/* probably true for those without linux/tasks.h? */
#   else
#    define MAX_PID	30000		/* good enough for real Unix... */
#   endif
#  endif
# endif
#endif

#if !defined(UINT_MAX) || !defined(ULONG_MAX)
# include "ERROR:  your system is too brain damaged to support!"
#endif

/*
 * The number of digits in a base-10 representation of MAXINT
 */
#ifndef MAXINT_B10_DIGITS
# if (__STDC__ - 0) > 0
#  if (UINT_MAX > 0xffffffffU)
#   define MAXINT_B10_DIGITS	(20)	/* for a 64-bit int: 9,223,372,036,854,775,808 */
#  else
#   define MAXINT_B10_DIGITS	(10)	/* for a 32-bit int 2,147,483,648 */
#  endif
# else
#  define MAXINT_B10_DIGITS	(10)	/* assume a 32-bit int */
# endif
#endif

#ifndef MAXLONG_B10_DIGITS
# if (__STDC__ - 0) > 0
#  if (ULONG_MAX > 0xffffffffUL)
#   define MAXLONG_B10_DIGITS	(20)	/* for a 64-bit long: 9,223,372,036,854,775,808 */
#  else
#   define MAXLONG_B10_DIGITS	(10)	/* for a 32-bit long 2,147,483,648 */
#  endif
# else
#  define MAXLONG_B10_DIGITS	(10)	/* assume a 32-bit long */
# endif
#endif

#if (MAXINT_B10_DIGITS == MAXLONG_B10_DIGITS && UINT_MAX != ULONG_MAX)
# include "ERROR:  assumptions about U*_MAX and MAX*_B10_DIGITS are wrong!"
#endif
#if (__STDC__ - 0) > 0
# if (MAXINT_B10_DIGITS <= 5 && UINT_MAX > 0xffffU)
#  include "ERROR:  assumptions about MAXINT_B10_DIGITS are wrong! (16bit int?)"
# endif
# if (MAXINT_B10_DIGITS <= 10 && UINT_MAX > 0xffffffffU)
#  include "ERROR:  assumptions about MAXINT_B10_DIGITS are wrong! (32bit int?)"
# endif
# if (MAXLONG_B10_DIGITS <= 10 && ULONG_MAX > 0xffffffffU)
#  include "ERROR:  assumptions about MAXLONG_B10_DIGITS are wrong! (64bit long?)"
# endif
#endif

#ifndef _PATH_DEVNULL
# define _PATH_DEVNULL	"/dev/null"
#endif

#ifndef SIG2STR_MAX
# define SIG2STR_MAX	32
#endif
/* Note "18" is (sizeof("(unknown signal %d)") - 1) */
#if (SIG2STR_MAX < (18 + MAXINT_B10_DIGITS))
# include "ERROR: SIG2STR_MAX has abnormally small value!"
#endif

/*
 * you can define bzero() in terms of memset(), but not the other way around...
 */
#ifndef HAVE_BZERO
# ifndef HAVE_MEMSET
#  include "ERROR: memset() not avaliable: this is nearly impossible!"
# endif
# define bzero(p, l)	memset((p), '0', (l));
#endif

#ifndef HAVE_STRCHR
# define strchr		index
# define strrchr	rindex	/* assume we don't have it either... */
#endif

#ifndef HAVE_RENAME
# include "ERROR: rename() not available!"
#endif

#ifndef HAVE_MKSTEMP
# include "ERROR: mkstemp() not available!"
#endif
