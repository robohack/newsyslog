/*
 * This implementation is somewhat tied to newsyslog.c
 */

#include <sys/cdefs.h>

#ifndef lint
static const char rcsid[] =
	"@(#)newsyslog:$Name:  $:$Id: str2sig.c,v 1.1 2002/01/04 00:49:13 woods Exp $";
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#include <limits.h>	/* for strtol(), but not really needed in here... */

#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif

#include <signal.h>

#if !defined(SYS_SIGNAME_DECLARED)
const char *const sys_signame[];		/* defined in signame.c */
#endif

extern int              isnumber __P((const char *));	/* defined in newsyslog.c */

#if !HAVE_DECL_STR2SIG
int                     str2sig __P((const char *, int *));
#endif

int
str2sig(signame, signum)
	const char     *signame;
	int            *signum;
{
	long           n;

	if (isnumber(signame)) {
		n = strtol(signame, (char **) NULL, 0);
		if (n <= 0 || (unsigned) n >= NSIG)
			return (-1);
		*signum = (int) n;
		return 0;
	} /* else it might be a signal name */
	for (n = 1; n < NSIG; n++) {
		if (strcasecmp(sys_signame[n], signame) == 0) {
			*signum = (int) n;
			return 0;
		}
	}
	return (-1);
}
