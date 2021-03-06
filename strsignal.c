#ident	"@(#)newsyslog:strsignal.c:$Format:%D:%ci:%cN:%h$"

/*
 * a portable implementation of strsignal(3)
 */

#include <sys/cdefs.h>

#ifndef lint
static const char rcsid[] =
	"@(#)newsyslog:strsignal.c:$Format:%D:%ci:%cN:%h$";
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

#include "newsyslog.h"		/* generic portability definitions */

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

#if !defined(HAVE_DECL_SIG2STR)
extern int              sig2str __P((int, char *));
#endif

# if !defined(SYS_SIGLIST_DECLARED) && !defined(__SYS_SIGLIST_DECLARED)
#  if 0		/* 4.4BSD, GNU/Linux, SCO5, SunOS-5, if they needed it  */
    extern const char *const sys_siglist[];
#  else
    extern char *sys_siglist[];
#  endif
# endif

/*
 * NOTE:  in SunOS-5 sys_nsig is a macro to rename to a private identifier.
 * However, since SunOS-5 already has its own strsignal(), we don't care!
 */

#if !defined(HAVE_DECL_STRSIGNAL)
char                   *strsignal __P((int *));
#endif

/*
 * strsignal -  return a pointer to a string describing the signal given in signum.
 */
char *
strsignal(signum)
    int signum;
{
#if defined(SIGRTMIN) && defined(SIGRTMAX)
    if (signum >= sys_nsig && signum >= SIGRTMIN && signum <= SIGRTMAX) {
	static char rt_descr[sizeof("Real time signal: ") + MAXINT_B10_DIGITS + 1];

	/*
	 * Signum is out of range but is within SIGRTMIN..SIGRTMAX so we build
	 * our own description for it.
	 */
	(void) sprintf(rt_descr, "Real time signal: %d", signum);
	return rt_descr;
    } else
#endif
    if (signum >= sys_nsig || signum < 0) {
	char signame[SIG2STR_MAX];
	static char misc_err[sizeof("Unknown signal: SIG []") + MAXINT_B10_DIGITS + SIG2STR_MAX + 1];

	/*
	 * Signum is out of range so there's no entry for it in sys_siglist, so
	 * we build our own description.
	 *
	 * Even though the SunOS-5 manual says "The strsignal() function
	 * returns NULL if sig is not a valid signal number.", it's easy enough
	 * to assume signum is valid and instead that sys_siglist is
	 * incomplete.  The behaviour of this implementation matches that of
	 * 4.4BSD.
	 */
	if (sig2str(signum, signame) == -1) {
	    (void) sprintf(signame, "#%d", signum);
	}
	(void) sprintf(misc_err, "Unknown signal: SIG%s [%d]", signame, signum);
	return misc_err;
    }

    /* there is an entry in sys_siglist, use it */
    /* NOTE: this discards the const qualifiers, but that's what must be done here */
    return (char *) sys_siglist[signum];
}
