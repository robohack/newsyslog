/*
 * This implementation is somewhat tied to newsyslog.c
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif

#include <signal.h>

/*
 * This is just the sunos-5 default value....
 */
#ifndef SIG2STR_MAX
# define SIG2STR_MAX	32		/* also defined in newsyslog.c */
#endif

#if !defined(SYS_SIGNAME_DECLARED)
const char *const sys_signame[];	/* defined in signame.c */
#endif

#if !HAVE_DECL_SIG2STR
int                     sig2str __P((int, char *));
#endif

int
sig2str(signum, signame)
	int signum;			/* signal number to translage */
	char *signame;			/* ptr to SIG2STR_MAX bytes storage */
{
	if (signum < 0 || signum >= NSIG)
		return (-1);
	strncpy(signame, sys_signame[signum], SIG2STR_MAX);
	signame[SIG2STR_MAX - 1] = '\0';

	return 0;
}
