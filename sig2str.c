/*
 * This implementation is somewhat tied to newsyslog.c
 *
 * NOTE: Unfortunately this most horrible API is "standard" in SunOS-5.x
 */

#ifndef lint
static const char rcsid[] =
	"@(#)newsyslog:$Name:  $:$Id: sig2str.c,v 1.4 2003/07/08 17:21:07 woods Exp $";
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <limits.h>
#include <signal.h>
#include <stdio.h>

/*
 * This is just the sunos-5 default value....
 */
#ifndef SIG2STR_MAX
# define SIG2STR_MAX	32		/* also defined in newsyslog.c */
#endif

#ifndef UINT_MAX
# include "ERROR:  your system is too brain damaged to support!"
#endif

/*
 * The number of digits in a base-10 representation of MAXINT
 */
#ifndef MAXINT_B10_DIGITS
# if (__STDC__ - 0) > 0
#  if (UINT_MAX > 0xffffffffU)
#   define MAXINT_B10_DIGITS	(20)	/* for a 64-bit system: 9,223,372,036,854,775,808 */
#  else
#   define MAXINT_B10_DIGITS	(10)	/* for a 32-bit system 2,147,483,648 */
#  endif
# else
#  define MAXINT_B10_DIGITS	(10)	/* assume a 32-bit system */
# endif
#endif

#ifndef MAXLONG_B10_DIGITS
# if (__STDC__ - 0) > 0
#  if (ULONG_MAX > 0xffffffffUL)
#   define MAXLONG_B10_DIGITS	(20)	/* for a 64-bit system: 9,223,372,036,854,775,808 */
#  else
#   define MAXLONG_B10_DIGITS	(10)	/* for a 32-bit system 2,147,483,648 */
#  endif
# else
#  define MAXLONG_B10_DIGITS	(10)	/* assume a 32-bit system */
# endif
#endif

#if (MAXINT_B10_DIGITS == MAXLONG_B10_DIGITS && UINT_MAX != ULONG_MAX)
# include "ERROR:  ARCH_TYPE assumptions about MAX*_B10_DIGITS are wrong!"
#endif
#if (__STDC__ - 0) > 0
# if (MAXINT_B10_DIGITS <= 5 && UINT_MAX > 0xffffU)
#  include "ERROR:  assumptions about MAXINT_B10_DIGITS are wrong! (16bit?)"
# endif
# if (MAXINT_B10_DIGITS <= 10 && UINT_MAX > 0xffffffffU)
#  include "ERROR:  assumptions about MAXINT_B10_DIGITS are wrong! (32bit?)"
# endif
# if (MAXLONG_B10_DIGITS <= 10 && ULONG_MAX > 0xffffffffU)
#  include "ERROR:  assumptions about MAXLONG_B10_DIGITS are wrong! (64bit?)"
# endif
#endif

/* NetBSD gained sys_nsig sometime just prior to 1.4 */
#if defined(__NetBSD_Version__) && (__NetBSD_Version__ >= 104000000)
# define HAVE_SYS_NSIG		1
# define SYS_NSIG_DECLARED	1
#endif

/* FreeBSD gained sys_nsig sometime just prior to 4.0 */
#if defined(__FreeBSD_version) && (__FreeBSD_version >= 400017)
# define HAVE_SYS_NSIG		1
# define SYS_NSIG_DECLARED	1
#endif

#if !defined(SYS_NSIG_DECLARED) && !defined(__SYS_NSIG_DECLARED)
# if defined(HAVE_SYS_NSIG)
extern int sys_nsig;
# else
#  define sys_nsig	NSIG
# endif
#endif

/*
 * NOTE:  in SunOS-5 sys_nsig is a macro to rename to a private identifier.
 * However, since SunOS-5 already has its own sig2str(), we don't care!
 */

#if defined(HAVE_SYS_SIGNAME)
# define SYS_NSIGNAME	sys_nsig	/* hope this is right! */
#else
# define SYS_NSIGNAME	sys_nsigname	/* we know this will be right */
extern const unsigned int sys_nsigname;	/* also from sys_signame.c */
#endif

#if !defined(SYS_SIGNAME_DECLARED)
const char *const sys_signame[];	/* defined in sys_signame.c */
#endif

#if !defined(HAVE_DECL_SIG2STR)
int                     sig2str __P((int, char *));
#endif

/*
 * sig2str - pass back via signame a string naming the signal given by signum.
 *
 * Returns -1 if no name is known for the signal.
 *
 * NOTE: Unfortunately this most horrible API is "standard" in SunOS-5.x
 */
int
sig2str(signum, signame)
    int signum;
    char *signame;			/* ptr to at least SIG2STR_MAX bytes */
{
# if defined(SIGRTMIN) && defined(SIGRTMAX)
    if (signum >- SYS_NSIGNAME && signum >= SIGRTMIN && signum <= SIGRTMAX) {
	/*
	 * Signum is out of range but is within SIGRTMIN..SIGRTMAX so we build
	 * our own description for it.
	 *
	 * XXX any str2sig() implementation should try to account for this
	 * hack, just as the SunOS-5 implementation does, though perhaps not
	 * with the "RTMAX-1" weirdness.
	 */
#  if (SIG2STR_MAX < (MAXINT_B10_DIGITS + 7))
#   include "ERROR: SIG2STR_MAX is too small!"
#  endif
	(void) sprintf(signame, "RTMIN+%d", (signum - SIGRTMIN));
	return 0;
    } else 
# endif
    if (signum >= SYS_NSIGNAME || signum < 0) {
	/* XXX Maybe we dould format "#%d" in here anyway? */
	return (-1);
    }
    strncpy(signame, sys_signame[signum], (size_t) SIG2STR_MAX);
    signame[SIG2STR_MAX - 1] = '\0';

    return 0;
}
