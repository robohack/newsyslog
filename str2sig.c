/*
 * This implementation is somewhat tied to newsyslog.c
 */

#include <sys/cdefs.h>

#ifndef lint
static const char rcsid[] =
	"@(#)newsyslog:$Name:  $:$Id: str2sig.c,v 1.5 2003/07/08 18:02:13 woods Exp $";
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#include <ctype.h>
#include <limits.h>	/* for strtol(), but not really needed in here... */
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <signal.h>

#include "newsyslog.h"		/* generic portability definitions */

#if !defined(SYS_SIGNAME_DECLARED)
const char *const sys_signame[];		/* defined in signame.c */
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
 * However, since SunOS-5 already has its own str2sig(), we don't care!
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

#if !defined(HAVE_DECL_STR2SIG)
int                     str2sig __P((const char *, int *));
#endif

/*
 * The stupid FreeBSD and SunOS-5 have an isnumber() function declared in their
 * <ctype.h>.  In FreeBSD it is defined identicaly to isdigit(), but in SunOS-5
 * it is defined as testing whether its 'wint_t' parameter is a "wide character
 * digit representing [0-9] from the supplementary code set" but of course all
 * the wide character ctype macros should have been called isw*().
 */
static int              isNumber __P((const char *));

int
str2sig(signame, signum)
	const char     *signame;
	int            *signum;
{
	long           n;

	if (isNumber(signame)) {
		n = strtol(signame, (char **) NULL, 0);
		if (n <= 0 || (unsigned) n >= NSIG)
			return (-1);
		*signum = (int) n;
		return 0;
	} /* else it might be a signal name */
#if defined(SIGRTMIN) && defined(SIGRTMAX)
	/* XXX TODO: handle RTMIN+n */
#endif
	for (n = 1; n < SYS_NSIGNAME; n++) {
		if (strcasecmp(sys_signame[n], signame) == 0) {
			*signum = (int) n;
			return 0;
		}
	}
	return (-1);
}

/*
 * Check if string is actually a number, i.e. *all* digits
 */
static int
isNumber(p)
	register const char *p;
{
	if (!p)
		return (0);
	while (*p && isdigit(*p))
		p++;
	return (*p ? 0 : 1);
}
