/*
 * This implementation is somewhat tied to newsyslog.c
 */

#include <sys/cdefs.h>

#ifndef lint
static const char rcsid[] =
	"@(#)newsyslog:$Name:  $:$Id: str2sig.c,v 1.3 2002/01/04 18:09:24 woods Exp $";
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#include <ctype.h>
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

#if !HAVE_DECL_STR2SIG
int                     str2sig __P((const char *, int *));
#endif

static int              isnumber __P((const char *));

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

/*
 * Check if string is actually a number, i.e. *all* digits
 */
static int
isnumber(p)
	register const char *p;
{
	if (!p)
		return (0);
	while (*p && isdigit(*p))
		p++;
	return (*p ? 0 : 1);
}