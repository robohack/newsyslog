/*
 *	newsyslog - roll over selected logs at the appropriate time,
 *		keeping the a specified number of backup files around.
 */

/*
 * This file contains changes from Greg A. Woods; Planix, Inc.
 *
 * Copyright for these changes is hereby assigned to MIT under the copyright
 * license given below.
 */

/*
 * This file contains changes from the Open Software Foundation.
 *
 * Presumably their changes were assigned implicitly to MIT too.
 */

/*
 * This file contains changes from FreeBSD.
 *
 * Presumably their changes were assigned implicitly to MIT too.
 */

/*
 *
 * Copyright 1988, 1989 by the Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is
 * hereby granted, provided that the above copyright notice
 * appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation,
 * and that the names of M.I.T. and the M.I.T. S.I.P.B. not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * M.I.T. and the M.I.T. S.I.P.B. make no representations about
 * the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 */

#include <sys/cdefs.h>

#ifndef lint
static const char orig_rcsid[] =
	"FreeBSD: newsyslog.c,v 1.14 1997/10/06 07:46:08 charnier Exp";
static const char rcsid[] =
	"@(#)newsyslog:$Name:  $:$Id: newsyslog.c,v 1.63 2009/07/16 02:07:41 woods Exp $";
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#if defined(HAVE_FLOCK) && defined(HAVE_SYS_FILE_H) && !defined(LOCK_EX)
# include <sys/file.h>			/* LOCK_* usually in <fcntl.h> */
#endif
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>			/* for MAXHOSTNAMELEN */
#endif
#if !defined(PID_MAX) && defined(HAVE_SYS_PROC_H)
# if defined(HAVE_SYS_USER_H)
#  include <sys/user.h>
# endif
# include <sys/proc.h>
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_STRING_H
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#else
# ifndef errno
extern int errno;
# endif /* !errno */
#endif /* HAVE_ERRNO_H */

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif
#ifdef HAVE_PATHNAMES_H
# include "pathnames.h"			/* a "local" file in *BSD */
#endif

#ifdef HAVE_LINUX_THREADS_H
# include <linux/threads.h>		/* PID_MAX? */
#endif
#ifdef HAVE_LINUX_TASKS_H
# include <linux/tasks.h>		/* PID_MAX? */
#endif

#include "newsyslog.h"			/* portability defines */

#ifndef PATH_COMPRESS
# if defined(_PATH_COMPRESS)
#  define PATH_COMPRESS		_PATH_COMPRESS
#  define COMPRESS_SUFFIX	".Z"
# elif defined(_PATH_GZIP)
#  define PATH_COMPRESS		_PATH_GZIP
#  define COMPRESS_SUFFIX	".gz"
# endif
#endif

#ifndef PATH_CONFIG
# ifdef _PATH_NEWSYSLOGCONF
#  define PATH_CONFIG		_PATH_NEWSYSLOGCONF
# endif
#endif

#ifndef PATH_SYSLOGD_PIDFILE
# ifdef _PATH_SYSLOGDPID
#  define PATH_SYSLOGD_PIDFILE	_PATH_SYSLOGDPID
# endif
#endif

#ifndef MAX_FORK_RETRIES
# define MAX_FORK_RETRIES	5	/* a reasonable effort.... */
#endif

#define CE_COMPACT	(1<<0)	/* Compact the achived log files */
#define CE_BINARY	(1<<1)	/* Logfile is binary, don't add status mesg */
#define CE_PLAIN0	(1<<2)	/* Keep .0 file uncompressed */
#define CE_NOSIGNAL	(1<<3)	/* Don't send a signal when file is trimmed */
#define CE_NOCREATE	(1<<4)	/* Don't create the new log file */
#define CE_TRIMAT	(1<<5)	/* we have a valid trim time specification */
#define CE_SUBDIR	(1<<6)	/* move archived logs in "${logfile}.d" */

#define NO_ID		((uid_t) -1)	/* no UID/GID specified -- preserve */

struct conf_entry {
	char           *log;		/* Name of the log */
	char           *pid_file;	/* PID file */
	char           *user;		/* Owner name of log */
	uid_t           uid;		/* Owner ID of log */
	char           *group;		/* Group name of log */
	uid_t           gid;		/* Group ID of log */
	unsigned int    numlogs;	/* Number of logs to keep */
	unsigned long   size;		/* maximum log size in KB */
	int             hours;		/* maximum hours between log trimming */
	time_t          trim_at;	/* time to trim log at */
	unsigned int    permissions;	/* File permissions on the log */
	unsigned int    flags;		/* Flags (CE_*)  */
	int             signum;		/* Signal to send to daemon (SIG*) */
	struct conf_entry *next;	/* Linked list pointer */
};

const char     *argv0 = PACKAGE_NAME;
const char      package[] = PACKAGE_NAME;	/* the package name */
const char      tarname[] = PACKAGE;		/* the original dist name (not PACKAGE_TARNAME, not yet) */
const char      version[] = VERSION;		/* the package version (not PACKAGE_VERSION, not yet) */
const char      bugreport[] = PACKAGE_BUGREPORT;

int             verbose = FALSE;/* Print out what's going on */
int             quiet = FALSE;	/* Don't print certain useless errors (use with '-F') */
int             write_metalog = FALSE;/* Magic for NetBSD unprivileged builds (use with '-F') */
int             needroot = TRUE;/* Root privs are necessary for default conf */
int             show_script = FALSE;/* Show sh-script on stdout instead of doing the job */
int             debug = FALSE;	/* Don't do anything, don't show script (use with -v) */
int             domidnight = -1;/* ignore(-1) do(1) don't(0) do midnight run */
int             run_interval = -1;/* interval in minutes at which we are run by cron */
int             force = FALSE;	/* force all files to be trimmed */
int             send_signals = TRUE;	/* normally we send signals as configured */
const char     *config_file = PATH_CONFIG;/* Configuration file to use */
const char     *syslogd_pidfile = PATH_SYSLOGD_PIDFILE;/* syslogd's pid file */
time_t          timenow;
pid_t           syslogd_pid;	/* read in from /etc/syslog.pid */
char            hostname[MAXHOSTNAMELEN + 1];	/* hostname */
char           *localdomain = NULL;		/* not currently used.... */
char           *my_uname;
char           *my_gname;
char           *daytime;		/* timenow in human readable form */

int                     main __P((int, char **)); /* XXX HACK for WARNS=1 */

static struct conf_entry *parse_file __P((char **));
static char            *strsob __P((char *));
static char            *strson __P((char *));
static char            *missing_field __P((char *, int, char *));
static void             do_entry __P((struct conf_entry *));
static void             parse_options __P((int, char **));
static void             usage __P((void));
static void             help __P((void));
static void             do_trim __P((struct conf_entry *));
static int              note_trim __P((char *));
static void             compress_log __P((char *));
static off_t            check_file_size __P((char *));
static int              check_old_log_age __P((struct conf_entry *));
static time_t           read_first_timestamp __P((char *));
static time_t           parse_tmstamp __P((char *, char *));
static pid_t            get_pid_file __P((const char *));
static int              getsig __P((char *));
static int              parse_dwm __P((char *, time_t *));

#if ! HAVE_DECL_OPTERR
extern int              opterr;
#endif
#if ! HAVE_DECL_OPTIND
extern int              optind;
#endif
#if ! HAVE_DECL_OPTOPT
extern int              optopt;
#endif
#if ! HAVE_DECL_OPTARG
extern char            *optarg;
#endif

#if ! HAVE_DECL_EXIT
extern void             exit __P((int));
#endif
#if ! HAVE_DECL_GETOPT
extern int              getopt __P((int, char * const [], const char *));
#endif
#if ! HAVE_DECL_SETGROUPENT
extern int              setgroupent __P((int));
#endif
#if ! HAVE_DECL_SETPASSENT
extern int              setpassent __P((int));
#endif
#if ! HAVE_DECL_SIG2STR
extern int              sig2str __P((int, char *));
#endif
#if ! HAVE_DECL_STR2SIG
extern int              str2sig __P((const char *, int *));
#endif
#if ! HAVE_DECL_STRCHR
extern char            *strchr __P((const char *, int));
#endif
#if ! HAVE_DECL_STRDUP
extern char            *strdup __P((const char *));
#endif
#if ! HAVE_DECL_STRPTIME		/* STUPID GNU/Linux/glibc */
extern char            *strptime __P((const char *, const char *, struct tm *));
#endif
#if ! HAVE_DECL_STRRCHR
extern char            *strrchr __P((const char *, int));
#endif
#if ! HAVE_DECL_STRTOK
extern char            *strtok __P((char *, const char *));
#endif
#if ! HAVE_DECL_STRTOL
extern long            *strtol __P((const char *, char **, int));
#endif
#if ! HAVE_DECL_STRTOL
extern unsigned long   *strtoul __P((const char *, char **, int));
#endif

int
main(argc, argv)
	int             argc;
	char           *argv[];
{
	char           *s;
	int             cstatus;
	int             cpid;
	struct conf_entry *p, *q;
	struct passwd  *pass;
	struct group   *grp;

	timenow = time((time_t *) 0);

	/* Let's get our hostname */
	(void) gethostname(hostname, sizeof(hostname));

	/* and truncate any domain part off */
	if ((s = strchr(hostname, '.'))) {
		*s++ = '\0';
		localdomain = strdup(s);
	} else {
		localdomain = strdup("");
	}

	parse_options(argc, argv);

	/* timenow may have been changed in parse_options() by -T */
	daytime = ctime(&timenow) + 4;		/* trim the day name off */
	daytime[15] = '\0';			/* trim the year off too */

	if (needroot && getuid()) {		/* using geteuid() would allow making program setuid */
		fprintf(stderr, "%s: you do not have root privileges\n", argv0);
		exit(1);
	}

	if (setgroupent(1) != 1) {
		fprintf(stderr,
			"%s: warning: setgroupent(1) failed: %s\n",
			argv0,
			strerror(errno));
		endgrent();		/* ??? */
	}
	if (setpassent(1) != 1) {
		fprintf(stderr,
			"%s: warning: setpassent(1) failed: %s\n",
			argv0,
			strerror(errno));
		endpwent();		/* ??? */
	}
	if ((pass = getpwuid(geteuid())) == NULL) {
		fprintf(stderr, "%s: you do not have a valid user name!\n", argv0);
		exit(1);
	}
	my_uname = strdup(pass->pw_name);

	if ((grp = getgrgid(getegid())) == NULL) {
		fprintf(stderr, "%s: you do not have a valid group name\n", argv0);
		exit(1);
	}
	my_gname = strdup(grp->gr_name);

	p = q = parse_file(argv + optind);

	syslogd_pid = get_pid_file(syslogd_pidfile);

	while (p) {
		do_entry(p);
		p = p->next;
		if (q->log)
			free(q->log);
		if (q->pid_file)
			free(q->pid_file);
		free((void *) q);
		q = p;
	}

	/* Wait for any children (log compressors) to finish.... */
	errno = 0;
	while (((cpid = wait(&cstatus)) != -1) || errno == EINTR) {
		if (cstatus == -1) {
			fprintf(stderr, "%s: failed to reap child process %d: %s.",
				argv0, cpid, strerror(errno));
		} else if (WIFEXITED(cstatus)) {
			if (verbose || (WEXITSTATUS(cstatus) != 0)) {
				fprintf(verbose ? stdout : stderr,
					"%s: child process[%d] exited status %d\n",
					argv0, cpid, WEXITSTATUS(cstatus));
			}
		} else if (WIFSIGNALED(cstatus)) {
			char signm[SIG2STR_MAX];

			if (sig2str(WTERMSIG(cstatus), signm) == -1) {
				sprintf(signm, "#%d", WTERMSIG(cstatus));
			}
			fprintf(stderr, "%s: child process[%d] killed by signal SIG%s %s: %s\n",
				argv0, cpid, signm,
				WCOREDUMP(cstatus) ? "and dumped core" : "(no core)",
				strsignal(WTERMSIG(cstatus)));
		} else if (WIFSTOPPED(cstatus)) {
			char signm[SIG2STR_MAX];

			if (sig2str(WSTOPSIG(cstatus), signm) == -1) {
				sprintf(signm, "#%d", WSTOPSIG(cstatus));
			}
			fprintf(stderr, "%s: child process[%d] stopped with signal SIG%s (%s), continuing...\n",
				argv0, cpid, signm, strsignal(WSTOPSIG(cstatus)));
			kill(cpid, SIGCONT);
		} else {
			fprintf(stderr, "%s: child process[%d] died with bad status \\0%03o\n",
				argv0, cpid, cstatus);
		}
		errno = 0;
	}
	if (errno != ECHILD) {
		fprintf(stderr, "%s: wait() failed: %s\n", argv0, strerror(errno));
		exit(1);
	}

	free(localdomain);

	exit (0);
	/* NOTREACHED */
}

static void
do_entry(ent)
	struct conf_entry *ent;
{
	int             we_trim_it = FALSE;
	off_t           size;			/* in kbytes */
	int             modtime = 0;		/* in hours */

	assert(domidnight == -1 || domidnight == 1 || domidnight == 0);
	if (verbose) {
		printf("## %s [%s(%lu):%s(%lu)] 0%03o <#%u,%s%s%s%s%s>: ",
		       ent->log,
		       ent->user ? ent->user : "",
		       (unsigned long) (ent->uid == NO_ID) ? (write_metalog ? 0 : geteuid()) : ent->uid,
		       ent->group ? ent->group : "",
		       (unsigned long) (ent->gid == NO_ID) ? (write_metalog ? 0 : getegid()) : ent->gid,
		       ent->permissions,
		       ent->numlogs,
		       (ent->flags & CE_SUBDIR) ? "subdir," : "",
		       (ent->flags & CE_NOCREATE) ? "nocreate," : "",
		       (ent->flags & CE_COMPACT) ? "compact," : "",
		       (ent->flags & CE_BINARY) ? "binary," : "",
		       (ent->flags & CE_NOSIGNAL) ? "nosig," : "",
		       (ent->flags & CE_PLAIN0) ? "plain.0" : "");
		if (ent->flags & CE_TRIMAT) {
			struct tm tms;
			char trimtime[20];

			tms = *localtime(&(ent->trim_at));
			strftime(trimtime, sizeof(trimtime), "%Y/%m/%d-%T", &tms);
			printf("(T:%s) ", trimtime);
		}
	}
	size = check_file_size(ent->log);
	if (size < 0) {
		if (verbose)
			printf("does not exist ");
		if (!(ent->flags & CE_NOCREATE)) {
			if (verbose)
				printf("(will create) ");
			we_trim_it = TRUE;
		}
	} else if (size == 0 && verbose)
		printf("is empty ");
	if (size > 0) {				/* ignore empty/missing ones */
		if (verbose && (ent->size > 0))
			printf("size (Kb): %ld [allow %ld] ", (long) size, ent->size);
		if ((ent->size > 0) && (size >= ent->size))
			we_trim_it = TRUE;
		if (ent->hours > 0) {
			modtime = check_old_log_age(ent);
			if (verbose)
				printf("age (hr): %d [allow %d] ", modtime, ent->hours);
			/* always trim if timestamp FUBAR */
			if (modtime >= ent->hours || modtime < 0)
				we_trim_it = TRUE;
		}
		if (ent->flags & CE_TRIMAT && domidnight == -1) {
			/*
			 * if there was no interval, but just a trim time spec
			 * then check to see if it's time to trim it now...
			 */
			if ((timenow >= ent->trim_at) &&
			    (difftime(timenow, ent->trim_at) < 60 * run_interval)) {
				if (verbose)
					printf("(time to trim) ");
				we_trim_it = TRUE;
			} else if (!we_trim_it) { /* don't not trim if we should! */
				if (verbose)
					printf("(but not time to trim) ");
				we_trim_it = FALSE;
			}
		}
	}
	if (domidnight == -1 ) {
		if (verbose && !(ent->flags & CE_TRIMAT))
			printf("(no -m|-M|trimtime) ");
	} else if (domidnight == 1 && (ent->hours % 24) == 0) {
		/*
		 * we've already set we_trim_it above if the file is as old or
		 * older than ent->hours
		 */
		if (verbose && we_trim_it)
			printf("(daily with -m) ");
	} else if (domidnight == 0 && (ent->hours % 24) == 0) {
		if (verbose && !force)
			printf("(not the daily trim, -M set) ");
		we_trim_it = FALSE;
	}
	if (force & !we_trim_it) {
		if (verbose)
			printf("(forced )");
		we_trim_it = TRUE;
	}
	if (we_trim_it) {
		if (verbose)
			puts(": creating and/or trimming log...");
		do_trim(ent);
	} else {
		if (verbose)
			putchar('\n');
	}

	return;
}

static void
parse_options(argc, argv)
	int             argc;
	char          **argv;
{
	int             ch;
	long            ltmp;
	char           *p;
	struct tm       tms;
	int             tms_hour;
	int             tms_min;

#ifdef notyet
/* #ifdef HAVE_GETPROGNAME */
	argv0 = getprogname();
/* #else */
#endif
	argv0 = (argv0 = strrchr(argv[0], '/')) ? argv0 + 1 : argv[0];

	optind = 1;		/* Start options parsing */
	opterr = 0;
	while ((ch = getopt(argc, argv, ":FMT:UVdf:hi:mnp:qrst:v")) != -1) {
		switch (ch) {
		case 'F':
			force = TRUE;
			break;
		case 'M':		/* NOT the midnight run */
			domidnight = 0;
			break;
		case 'T':		/* reset current time, mostly for testing */
			/* assert(optarg != NULL); */
			p = strptime(optarg, "%H:%M", &tms);
			tms_hour = tms.tm_hour;
			tms_min = tms.tm_min;
			tms = *localtime(&timenow);
			tms.tm_hour = tms_hour;
			tms.tm_min = tms_min;
			if (!p || (p && *p)) {	/* should point to '\0' */
				fprintf(stderr,
					"%s: time of '%s' is not parsable (use HH:MM)\n",
					argv0,
					optarg);
				exit(2);
			}
			/*
			 * XXX mktime() has a broken API....
			 */
			if ((timenow = mktime(&tms)) == (time_t) -1) {
				fprintf(stderr,
					"%s: time of '%s' cannot be converted\n",
					argv0,
					optarg);
				exit(2);
			}
			if (timenow < time((time_t *) NULL))
				timenow += 24*60*60; /* go to tomorrow */
			if (verbose)
				printf("%s: have adjusted timenow to: %s", argv0, ctime(&timenow));
			break;
		case 'U':
			write_metalog = TRUE;
			needroot = FALSE;	/* don't need root */
			break;
		case 'V':
			printf("%s: version %s-%s.\n", argv0, package, version);
			exit(0);
			/* NOTREACHED */
		case 'd':
			debug++;
			needroot = FALSE;	/* don't need root */
			break;
		case 'f':
			/* assert(optarg != NULL); */
			config_file = optarg;
			break;
		case 'h':
			help();
			/* NOTREACHED */
		case 'i':		/* run interval in minutes */
			/* assert(optarg != NULL); */
			if (optarg[0] == '\0') {
				fprintf(stderr,
					"%s: an empty run interval value is not permitted\n",
					argv0);
				usage();
				/* NOTREACHED */
			}
			errno = 0;
			ltmp = strtol(optarg, &p, 0);
			if (p == optarg) {
				fprintf(stderr,
					"%s: run interval of '%s' is not a valid number\n",
					argv0,
					optarg);
				exit(2);
			}
			if (*p) {
				fprintf(stderr,
					"%s: run interval of '%s' has unsupported trailing unit specification characters\n",
					argv0,
					optarg);
				exit(2);
			}
			if (errno != 0) {
				fprintf(stderr,
					"%s: run interval of '%s' is not convertible: %s\n",
					argv0,
					optarg,
					strerror(errno));
				exit(2);
			}
			if (ltmp > INT_MAX) {
				fprintf(stderr,
					"%s: run interval of %ld is too large\n",
					argv0,
					ltmp);
				exit(2);
			}
			if (ltmp < 1) {
				fprintf(stderr,
					"%s: run interval of %ld is too small\n",
					argv0,
					ltmp);
				exit(2);
			}
			run_interval = (int) ltmp;
			break;
		case 'm':
			domidnight = 1;	/* the midnight run */
			break;
		case 'n':
			show_script = TRUE;
			needroot = FALSE;	/* don't need root */
			break;
		case 'p':
			/* assert(optarg != NULL); */
			syslogd_pidfile = optarg;
			break;
		case 'q':
			quiet++;
			break;
		case 'r':
			needroot = FALSE;
			break;
		case 's':
			send_signals = FALSE;
			break;
		case 'v':
			verbose++;
			break;
		case '?':
			fprintf(stderr, "%s: invalid option: '-%c'\n", argv0, optopt);
			usage();
			/* NOTREACHED */
		/*
		 * "If optstring has a leading ':' then a missing option
		 * argument causes ':' to be returned instead of '?'." ... "in
		 * addition to suppressing any error messages" ... "the variable
		 * optopt is set to the character that caused the error."
		 */
		case ':':
			fprintf(stderr, "%s: missing parameter for -%c\n", argv0, optopt);
			usage();
			/* NOTREACHED */
		default:
			fprintf(stderr, "%s: internal error: -%c not handled\n", argv0, ch);
			abort();
			/* NOTREACHED */
		}
	}

	return;
}

#define USAGE_FMT	"Usage: %s [-V] [-T hh:mm] [-M|-m|-i interval] [-Fdnrsv] [-f config-file] [-p syslogd-pidfile] [file ...]\n"

static void
usage()
{
	fprintf(stderr, USAGE_FMT, argv0);
	exit(2);
	/* NOTREACHED */
}

static void
help()
{
	printf(USAGE_FMT, argv0);
	printf("\n");
	printf("%s, distributed as %s-%s.  Usage details:\n", package, tarname, version);
	printf("\n");
	printf("	-F		force immediate trimming\n");
	printf("	-M		select normal periodic processing [opposite of -m]\n");
	printf("	-T hh:mm	adjust current time\n");
	printf("	-U		magic 'unprivileged' feature for use in NetBSD builds\n");
	printf("	-V		display version and exit\n");
	printf("	-d		don't do anything, just debug (repeat for in-depth)\n");
	printf("	-f config_fn	configuration file [default: %s]\n", config_file);
	printf("	-h		print this help and exit\n");
	printf("	-i minutes	specify how often %s is invoked\n", argv0);
	printf("	-m		select 'midnight' processing [opposite of -M]\n");
	printf("	-n		show sh-script on stdout instead of doing work\n");
	printf("	-p syslogd-pid	syslogd PID file [default: %s]\n", syslogd_pidfile);
	printf("	-r		remove restriction to superuser\n");
	printf("	-s		do not signal daemon processes\n");
	printf("	-v		show verbose explanitory messages\n");
	printf("\n");
	printf("	file		only trim specified file(s)\n");
	printf("\n");
	printf("Original Copyright (c) 1987, Massachusetts Institute of Technology\n");
	printf("Package Copyright (c) Planix, Inc.\n");
	printf("See 'COPYING' file in the source distribution for details.\n");
	printf("Please send bug reports to <%s>\n", bugreport);
	exit(0);
	/* NOTREACHED */
}

/* Parse a configuration file and return a linked list of all the logs
 * to process
 */
static struct conf_entry *
parse_file(files )
	char          **files;
{
	FILE           *fp = NULL;
	char            line[BUFSIZ];
	char           *parse;
	char           *q;
	char           *errline;
	char           *group;
	struct conf_entry *first = NULL;
	struct conf_entry *working = NULL;
	struct passwd  *pass;
	struct group   *grp;
	int             eol = FALSE;
	int             lnum = 0;
	char            prev = '\0';

	/*
	 * NOTE:  we don't ever close the config file until exit() does it for
	 * us.  If we're reading stdin then we don't need to close it, and if
	 * we're reading any other file we need to hold the lock until we're
	 * done to prevent collisions with new instances.
	 */
	if (strcmp(config_file, "-") == 0) {
		fp = stdin;
		config_file = "STDIN";
	} else {
		int             fd;

#ifdef HAVE_FLOCK
		/*
		 * flock(2) is preferred because it works with vi
		 */
		if ((fd = open(config_file, O_RDONLY, (mode_t) 0)) != -1) {
			if (flock(fd, LOCK_EX | LOCK_NB) != -1)
				fp = fdopen(fd, "r");
		}
#else
		/*
		 * fcntl locking is almost standard so assume it is
		 * there if flock() is not available.
		 */
		/*
		 * NOTE:  because we want an exclusive lock, we have to ask for
		 * a "write" lock, and on some systems (e.g. some/all 4.4BSD
		 * and derivatives) this means we need to open the file for
		 * read and write.  This is really stupid, but that's the way
		 * of POSIX!  :-/
		 */
		if ((fd = open(config_file, O_RDWR, (mode_t) 0)) != -1) {
			struct flock	lock;

			lock.l_type = F_WRLCK;	/* exclusive */
			lock.l_start = 0;
			lock.l_whence = SEEK_SET;
			lock.l_len = 0;
			if (fcntl(fd, F_SETLK, &lock) != -1)
				fp = fdopen(fd, "r");
		}
#endif
	}
	if (!fp) {
		fprintf(stderr,
			"%s: could not open config file %s: %s.\n",
			argv0,
			config_file,
			strerror(errno));
		exit(1);
	}
	while (fgets(line, BUFSIZ, fp)) {
		struct conf_entry *tmpentry;
		char *ep;

		lnum++;

		if ((line[0] == '\n') || (line[0] == '#'))
			continue;

		errline = strdup(line);
		if (*(q = &errline[strlen(errline) - 1]) == '\n')
			*q = '\0';

		q = parse = missing_field(strsob(line), lnum, errline);
		parse = strson(line);
		if (!*parse) {
			fprintf(stderr,
				"%s: malformed line (missing fields):\n%6d:\t'%s'\n",
				argv0,
				lnum,
				errline);
			exit(1);
		}
		*parse = '\0';

		if (*files) {
			char          **p;

			for (p = files; *p; ++p) {
				if (strcmp(*p, q) == 0)
					break;
			}
			if (!*p) {
				if (debug)
					printf("ignoring %s, not on command line...\n", q);
				continue;
			}
		}

		if (!(tmpentry = (struct conf_entry *) malloc(sizeof(struct conf_entry)))) {
			perror(argv0);
			exit(1);
		}
		if (first == NULL) {	/* have we started the list yet? */
			working = tmpentry;
			first = working;
		} else {
			working->next = tmpentry;
			working = working->next;
		}

		working->log = strdup(q);

		q = parse = missing_field(strsob(++parse), lnum, errline);
		parse = strson(parse);
		if (!*parse) {
			fprintf(stderr,
				"%s: malformed line (missing fields):\n%6d:\t'%s'\n",
				argv0,
				lnum,
				errline);
			exit(1);
		}
		*parse = '\0';

		/*
		 * the next field is optional....
		 *
		 * if it contains a colon, then it is a uid:gid specification,
		 * otherwise there is no uid:gid specifiction.
		 */
		if ((group = strchr(q, ':')) != NULL) {
			*group++ = '\0';
			if (*q) {
				if (!(isdigit(*q)) && *q != '-') {
					working->user = strdup(q);
					working->uid = NO_ID;
					if (!write_metalog) {
						if ((pass = getpwnam(q)) == NULL) {
							fprintf(stderr,
								"%s: error in config file; unknown user: '%s', in line:\n%6d:\t'%s'\n",
								argv0,
								q,
								lnum,
								errline);
							exit(1);
						}
						working->uid = pass->pw_uid;
					}
				} else {
					errno = 0;
					working->uid = strtol(q, &ep, 10);
					if (*ep || errno != 0) {
						fprintf(stderr,
							"%s: error in config file; invalid UID number: '%s', in line:\n%6d:\t'%s'\n",
							argv0,
							q,
							lnum,
							errline);
						exit(1);
					}
					/* XXX should try to look up the username for this ID!!! */
					working->user = NULL;
				}
			} else {
				working->user = NULL;
				working->uid = NO_ID;
			}
			q = group;
			if (*q) {
				if (!(isdigit(*q)) && *q != '-') {
					working->group = strdup(q);
					working->gid = NO_ID;
					if (!write_metalog) {
						if ((grp = getgrnam(q)) == NULL) {
							fprintf(stderr,
								"%s: error in config file; unknown group: '%s', in line:\n%6d:\t'%s'\n",
								argv0,
								q,
								lnum,
								errline);
							exit(1);
						}
						working->gid = grp->gr_gid;
					}
				} else {
					errno = 0;
					working->gid = strtol(q, &ep, 10);
					if (*ep || errno != 0) {
						fprintf(stderr,
							"%s: error in config file; invalid GID number: '%s', in line:\n%6d:\t'%s'\n",
							argv0,
							q,
							lnum,
							errline);
						exit(1);
					}
					/* XXX should try to look up the groupname for this ID!!! */
					working->group = NULL;
				}
			} else {
				working->group = NULL;
				working->gid = NO_ID;
			}
			q = parse = missing_field(strsob(++parse), lnum, errline);
			parse = strson(parse);
			if (!*parse) {
				fprintf(stderr,
					"%s: malformed line (missing fields):\n%6d:\t'%s'\n",
					argv0,
					lnum,
					errline);
				exit(1);
			}
			*parse = '\0';
		} else {
			working->user = working->group = NULL;
			working->uid = working->gid = NO_ID;
		}

		/* XXX someday support "rwxrwxrwx" style permissions? */
		/* XXX use *BSD setmode(3)??? */
		errno = 0;
		working->permissions = strtoul(q, &ep, 8);
		if (*ep || errno != 0) {
			fprintf(stderr,
				"%s: error in config file; %s: '%s', in line:\n%6d:\t'%s'\n",
				argv0,
				strchr(q, '.') ? "old-style 'owner:group' field format" : "bad permissions specification",
				q,
				lnum,
				errline);
			exit(1);
		}
		if (working->permissions & ~(S_IRWXU|S_IRWXG|S_IRWXO)) {
			fprintf(stderr,
				"%s: error in config file; inappropriate permissions specification: '%s', in line:\n%6d:\t'%s'\n",
				argv0,
				q,
				lnum,
				errline);
			exit(1);
		}

		q = parse = missing_field(strsob(++parse), lnum, errline);
		parse = strson(parse);
		if (!*parse) {
			fprintf(stderr,
				"%s: malformed line (missing fields):\n%6d:\t'%s'\n",
				argv0,
				lnum,
				errline);
			exit(1);
		}
		*parse = '\0';

		if (*q == '-') {
			fprintf(stderr,
				"%s: error in config file; negative log count is impossible: '%s', in line\n%6d:\t'%s'\n",
				argv0,
				q,
				lnum,
				errline);
			exit(1);
		}
		errno = 0;
		working->numlogs = strtoul(q, &ep, 10);
		if (*ep || errno != 0) {
			fprintf(stderr,
				"%s: error in config file; invalid number: '%s', in line\n%6d:\t'%s'\n",
				argv0,
				q,
				lnum,
				errline);
			exit(1);
		}

		q = parse = missing_field(strsob(++parse), lnum, errline);
		parse = strson(parse);
		if (!*parse) {
			fprintf(stderr,
				"%s: malformed line (missing fields):\n%6d:\t'%s'\n",
				argv0,
				lnum,
				errline);
			exit(1);
		}
		*parse = '\0';

		if (isdigit(*q)) {
			errno = 0;
			working->size = strtoul(q, &ep, 10);
			if (*ep || errno != 0) {
				fprintf(stderr,
					"%s: error in config file; invalid size: '%s', in line:\n%6d:\t'%s'\n",
					argv0,
					q,
					lnum,
					errline);
				exit(1);
			}
		} else
			working->size = -1;

		q = parse = missing_field(strsob(++parse), lnum, errline);
		if (*q == '#') {
			eol = TRUE;
			q = NULL;
		}
		parse = strson(parse);
		eol = !*parse;
		*parse = '\0';

		working->hours = -1;
		working->trim_at = -1;
		working->flags = 0;

		if (*q != '*') {
			u_long ul;

			if (isdigit(*q)) {
				errno = 0;
				ul = strtoul(q, &ep, 10);
				if (errno != 0) {
					fprintf(stderr,
						"%s: interval value of '%s' is not convertible, in line\n%6d:\t'%s'\n",
						argv0,
						q,
						lnum,
						errline);
					exit(1);
				}
				if (ul > INT_MAX) {
					fprintf(stderr,
						"%s: error in config file; interval too large: '%s', in line\n%6d:\t'%s'\n",
						argv0,
						q,
						lnum,
						errline);
					exit(1);
				}
				q = ep;
				working->hours = (int) ul;
			}
			/*
			 * the interval may be followed by a specification of
			 * when to trim the file (making the interval really a
			 * maximum age)....
			 */
			if ((*q == '-') || (*q == '$') || (working->hours == -1)) {
				if (domidnight != -1) {
					fprintf(stderr,
						"%s: a trim time is nonsensical with %s: '%s', in line\n%6d:\t'%s'\n",
						argv0,
						(domidnight == 0) ? "-M" : "-m",
						q,
						lnum,
						errline);
					exit(1);
				}
				if (parse_dwm(q, &working->trim_at) == -1) {
					fprintf(stderr,
						"%s: error in config file; malformed trim time: '%s', in line\n%6d:\t'%s'\n",
						argv0,
						q,
						lnum,
						errline);
					exit(1);
				}
				if (run_interval == -1) {
					fprintf(stderr,
						"%s: a trim time needs '-i run_interval': '%s', in line\n%6d:\t'%s'\n",
						argv0,
						q,
						lnum,
						errline);
					exit(1);
				}
				working->flags |= CE_TRIMAT;
			}
			if ((working->hours == -1) && (working->trim_at == -1)) {
				fprintf(stderr,
					"%s: error in config file; malformed interval or trim time: '%s', in line\n%6d:\t'%s'\n",
					argv0,
					q,
					lnum,
					errline);
				exit(1);
			}
		}

		if (eol)
			q = NULL;
		else {
			q = parse = strsob(++parse);	/* Optional field */
			if (q && *q == '#') {
				eol = TRUE;
				q = NULL;
			}
			parse = strson(parse);
			if (!*parse)
				eol = TRUE;
			*parse = '\0';
		}
		while (q && *q && !isspace(*q)) {
			if (*q == '/')
				working->flags |= CE_SUBDIR;
			else if ((*q == 'D') || (*q == 'd'))
				working->flags |= CE_NOCREATE;
			else if ((*q == 'Z') || (*q == 'z'))
				working->flags |= CE_COMPACT;
			else if ((*q == 'B') || (*q == 'b'))
				working->flags |= CE_BINARY;
			else if ((*q == 'N') || (*q == 'n'))
				working->flags |= CE_NOSIGNAL;
			else if ((*q == '0') || (*q == 'p') || (*q == 'P'))
				working->flags |= CE_PLAIN0;
			else if ((*q == 'c') || (*q == 'C'))
				working->flags &= ~CE_NOCREATE;
			else if (*q != '-') {
				fprintf(stderr,
					"%s: invalid flag in config file -- %c on line:\n%6d:\t'%s'\n",
					argv0,
					*q,
					lnum,
					errline);
				exit(1);
			}
			q++;
		}

		if (eol)
			q = NULL;
		else {
			q = parse = strsob(++parse);	/* Optional field */
			if (q && *q == '#') {
				eol = TRUE;
				q = NULL;
			}
			parse = strson(parse);
			if (!*parse)
				eol = TRUE;
			prev = *parse;
			*parse = '\0';
		}
		working->pid_file = NULL;
		if (q && *q) {
			if (*q == '/') {
				if (strcmp(_PATH_DEVNULL, q) == 0)
					working->flags |= CE_NOSIGNAL;
				else
					working->pid_file = strdup(q);
			} else {
				*parse = prev;		/* un-terminate the token */
				parse = q - 1;		/* skip back before it */
			}
		}

		if (eol)
			q = NULL;
		else {
			q = parse = strsob(++parse);	/* Optional field */
			if (q && *q == '#') {
				eol = TRUE;
				q = NULL;
			}
			*(parse = strson(parse)) = '\0';
		}
		working->signum = SIGHUP;
		if (q && *q) {
			if ((working->signum = getsig(q)) < 0) {
				fprintf(stderr,
					"%s: invalid signal name/number in config file: %s, on line:\n%6d:\t'%s'\n",
					argv0,
					q,
					lnum,
					errline);
				exit(1);
			}
		}

		free(errline);
	}
	if (working)
		working->next = (struct conf_entry *) NULL;

	/* NOTE: do not fclose(fp) -- it must stay open until the process exits! */

	return (first);
}

/*
 * Check for a missing field.  Print an error message including the specified
 * text and exit if the supplied token is null or empty.
 */
static char    *
missing_field(p, lnum, errline)
	char           *p;		/* parser token */
	int             lnum;		/* input line number */
	char           *errline;	/* input line with error (newline stripped) */
{
	if (!p || !*p) {
		fprintf(stderr,
			"%s: missing field in config file on line:\n%6d:\t'%s'\n",
			argv0,
			lnum,
			errline);
		exit(1);
	}

	return (p);
}


/*
 * Do the actual aging and trimming of log files, notify the daemon if
 * necessary, and compress any old file that needs compression.
 *
 * Note that errors in this stage are not fatal.
 */
static void
do_trim(ent)
	struct conf_entry *ent;
{
	char            file1[PATH_MAX - sizeof(COMPRESS_SUFFIX)];
	char            file2[PATH_MAX - sizeof(COMPRESS_SUFFIX)];
	char            zfile1[PATH_MAX];
	char            zfile2[PATH_MAX];
	char            newlog[PATH_MAX];
	int             log_exists;
	int             fd;
	unsigned int    numlogs;
	struct stat     st;
	pid_t           pid = 0;	/* zero means "don't send signal" */
	struct passwd  *pass;
	struct group   *grp;
	int             might_need_newlog = FALSE;
	int             need_compress = FALSE;
	int             might_timestamp = FALSE;
	int             notified = FALSE;

	/*
	 * first learn about the existing log file, if it exists
	 */
	if (stat(ent->log, &st) < 0) {
		if (!quiet) {
			fprintf(stderr,
				"%s: can't stat file (will %s it): %s: %s.\n",
				argv0,
				(ent->flags & CE_NOCREATE) ? "ignore" : "create",
				ent->log,
				strerror(errno));
		}
		log_exists = FALSE;
		might_need_newlog = TRUE;
		/*
		 * Most implementations of chown(2) "do the right thing" with
		 * -1, but some don't (e.g. AIX-3.1), so we'll just play it
		 * safe and always set them to the most likely correct values.
		 */
		if (ent->uid == NO_ID)
			ent->uid = write_metalog ? 0 : geteuid();
		if (!ent->user)
			ent->user = write_metalog ? strdup("root") : my_uname;
		if (ent->gid == NO_ID)
			ent->gid = write_metalog ? 0 : getegid();
		if (!ent->group)
			ent->group = write_metalog ? strdup("wheel") : my_gname;
	} else {
		log_exists = TRUE;
		/*
		 * preserve the file's UID/GID if none specified in the config.
		 * Note that we also "clobber" the ownerships of aged files too
		 * under the assumption that they should be "protected" in the
		 * same way as the current file.
		 */
		if (ent->uid == NO_ID)
			ent->uid = st.st_uid;
		if (!ent->user) {
			if ((pass = getpwuid(st.st_uid)) == NULL) {
				ent->user = write_metalog ? strdup("root") : my_uname;
				if (!quiet) {
					fprintf(stderr,
						"%s: %s has unknown user-ID: '%u', using '%s'\n",
						argv0,
						ent->log,
						st.st_uid,
						ent->user);
				}
			} else
				ent->user = strdup(pass->pw_name);
		}
		if (ent->gid == NO_ID)
			ent->gid = st.st_gid;
		if (!ent->group) {
			if ((grp = getgrgid(st.st_gid)) == NULL) {
				ent->group = write_metalog ? strdup("wheel") : my_gname;
				if (!quiet) {
					fprintf(stderr,
						"%s: %s has unknown group-ID: '%u', using '%s'\n",
						argv0,
						ent->log,
						st.st_uid,
						ent->group);
				}
			} else
				ent->group = strdup(grp->gr_name);
		}
	}
	/* prepare the temporary name for a newly created log file */
	if (snprintf(newlog, sizeof(newlog), "%s.%u.XXXXXX", ent->log, 0) >= (int) sizeof(file1)) {
		fprintf(stderr, "%s: filename too long: %s.\n", argv0, ent->log);
		return;
	}
	/* form the uncompressed name of the oldest expected log */
	if (snprintf(file1, sizeof(file1),
		     (ent->flags & CE_SUBDIR) ? "%s.old/%04u" : "%s.%u",
		     ent->log,
		     ent->numlogs) >= (int) sizeof(file1)) {
		fprintf(stderr, "%s: filename too long: %s.\n", argv0, ent->log);
		return;
	}
	/* form the compressed name of the oldest expected log */
	(void) strcpy(zfile1, file1);
	(void) strcat(zfile1, COMPRESS_SUFFIX);

	if (log_exists)
		numlogs = ent->numlogs;	/* we don't modify ent's contents */
	else
		numlogs = 0;
	if (numlogs) {
		/*
		 * Remove the oldest expected log, compressed or not.
		 *
		 * We don't really care if that last log exists or not... we
		 * just need to remove it if it does, so we explicitly ignore
		 * any errors.
		 */
		if (verbose)
			printf("# removing oldest (expected) log: %s and/or %s\n", file1, zfile1);
		if (show_script) {
			printf("rm -f %s\n", file1);
			printf("rm -f %s\n", zfile1);
		} else if (!debug) {
			(void) unlink(file1);
			(void) unlink(zfile1);
		}
	}
	/*
	 * Now move backwards down the list of archived log files, incrementing
	 * their generation number by renaming the previous one to the next one
	 * in reverse order.  We don't care if the log is compressed or not,
	 * but we only expect there to be either an uncompressed copy, or a
	 * compressed copy, not both.  This allows an admin to uncompress a
	 * file and look at it in place, and then later recompress it, so long
	 * as the manual (un)compress isn't running when the next rotation
	 * happens.
	 */
	if (numlogs && verbose)
		printf("# attempting to rotate %d archives up by one...\n", numlogs);
	while (numlogs--) {
		(void) strcpy(file2, file1);
		(void) snprintf(file1, sizeof(file1),
				(ent->flags & CE_SUBDIR) ? "%s.old/%04u" : "%s.%u",
				ent->log,
				numlogs);
		(void) strcpy(zfile1, file1);	/* strcpy() OK here */
		(void) strcpy(zfile2, file2);
		if (stat(file1, &st) < 0) {	/* is the file uncompressed? */
			(void) strcat(zfile1, COMPRESS_SUFFIX);	/* strcat() OK too */
			(void) strcat(zfile2, COMPRESS_SUFFIX);
			if (!show_script && stat(zfile1, &st) < 0) {
				if (debug < 2)
					continue; /* not this many aged files yet... */
			}
		}
		if ((ent->flags & CE_COMPACT) && (numlogs == (ent->flags & CE_PLAIN0) ? 1 : 0))
			need_compress = TRUE;	/* expect to compress 'file1'... */
		if (verbose)
			printf("# renaming %s to %s\n", zfile1, zfile2);
		if (show_script)
			printf("mv %s %s\n", zfile1, zfile2);
		else if (!debug)
			(void) rename(zfile1, zfile2); /* XXX error check (fatal? -- may lose archive data!) */
		if (verbose) {
			printf("# forcing owner/perms of %s to %d:%d/0%03o\n",
			       zfile2, ent->uid, ent->gid, ent->permissions);
		}
		if (show_script) {
			printf("chmod 0%03o %s\n", ent->permissions, zfile2);
			printf("chown %d:%d %s\n",
			       ent->uid, ent->gid, zfile2);
		} else if (!debug) {
			(void) chmod(zfile2, ent->permissions); /* XXX error check (non-fatal?) */
			(void) chown(zfile2, ent->uid, ent->gid); /* XXX error check (non-fatal?) */
		}
	}
	if (log_exists && st.st_size > 0) {
		might_need_newlog = TRUE;
		/*
		 * are we supposed to be keeping any aged files at all?
		 */
		if (ent->numlogs) {
			char            old_dir[PATH_MAX];
			struct stat     st;

			/*
			 * first make sure that the archive directory exists, if we need it
			 */
			(void) snprintf(old_dir, sizeof(old_dir), "%s.old", ent->log);
			if (stat(old_dir, &st) == 0) {
				/*
				 * if the pathname exists, make sure it really
				 * is a directory
				 */
				if ((st.st_mode & S_IFMT) != S_IFDIR) {
					errno = ENOTDIR;
					fprintf(stderr,
						"%s: invalid archive directory: %s: %s.\n",
						argv0,
						old_dir,
						strerror(errno));
					return;
				}
				/* don't change modes or ownership of existing old_dir! */
			} else if ((ent->flags & CE_SUBDIR)) {
				if (verbose)
					printf("# creating directory %s for archives\n", old_dir);
				if (show_script) {
					printf("mkdir %s\n", old_dir);
					printf("chmod 0%03o %s\n", ent->permissions, old_dir);
					printf("chown %d:%d %s\n", ent->uid, ent->gid, old_dir);
				} else if (!debug) {
					unsigned int dirperms = ent->permissions;

					/*
					 * for each level of permissions, if
					 * read permission was granted for the
					 * log file, then also add search
					 * permission for the archive directory
					 */
					if (dirperms & S_IRUSR)
						dirperms |= S_IXUSR;
					if (dirperms & S_IRGRP)
						dirperms |= S_IXGRP;
					if (dirperms & S_IROTH)
						dirperms |= S_IXOTH;
					if (mkdir(old_dir, dirperms) == -1) {
						fprintf(stderr,
							"%s: can't create archive directory: %s: %s.\n",
							argv0,
							old_dir,
							strerror(errno));
						return;
					}
					if (chown(old_dir, ent->uid, ent->gid) && !quiet && !write_metalog) {
						fprintf(stderr,
							"%s: can't chown %d:%d archive directory: %s: %s.\n",
							argv0,
							ent->uid, ent->gid,
							old_dir,
							strerror(errno));
					}
				}
			}
			if (verbose)
				printf("# renaming %s to %s\n", ent->log, file1);
			if (show_script)
				printf("mv %s %s\n", ent->log, file1);
			else if (!debug) {
				if (rename(ent->log, file1) < 0 && !write_metalog) {
					fprintf(stderr,
						"%s: can't rename file: %s to %s: %s.\n",
						argv0,
						ent->log,
						file1,
						strerror(errno));
					return;
				}
				if (((ent->pid_file && strcmp(ent->pid_file, _PATH_DEVNULL) == 0) ||
				    (ent->flags & CE_NOSIGNAL)) &&
				    !(ent->flags & CE_NOCREATE) &&
				    verbose) {
					/*
					 * If the creator of the log doesn't
					 * need a signal to roll over to the
					 * new log, and if we are going to be
					 * creating the new log ourselves, then
					 * there's a potential race with the
					 * log creator!
					 *
					 * This is important because some
					 * programs might actually abort with
					 * an error if they are not able to
					 * create their own log file as they
					 * may expect to have to do.
					 */
					printf("# possible race with creator of %s\n", ent->log);
				}
			}
			might_timestamp = TRUE;
		} else {
			/*
			 * if not just remove the current file...
			 */
			if (verbose)
				printf("# not keeping multiples archives, just removing %s\n", ent->log);
			if (show_script)
				printf("rm %s\n", ent->log);
			else if (!debug) {
				if (unlink(ent->log) < 0) {
					fprintf(stderr,
						"%s: can't unlink file: %s: %s.\n",
						argv0,
						ent->log,
						strerror(errno));
				}
			}
		}
	}
	/* logic is split here to keep indentation sane.... */
	if (might_need_newlog && !(ent->flags & CE_NOCREATE)) {
		if (verbose) {
			printf("# creating first archive file for %s with owner %d:%d, mode 0%03o\n",
			       ent->log, ent->uid, ent->gid, ent->permissions);
		}
		if (show_script) {
			printf("newlog=$(mktemp %s)\n", newlog);
			printf("touch $newlog\n");
			printf("chown %d:%d $newlog\n", ent->uid, ent->gid);
			printf("chmod 0%03o $newlog\n", ent->permissions);
			printf("mv $newlog %s\n", ent->log);
		} else if (!debug) {
			if ((fd = mkstemp(newlog)) < 0 && !write_metalog) {
				fprintf(stderr,
					"%s: can't create new log file: %s: %s.\n",
					argv0,
					newlog,
					strerror(errno));
				return;
			} else {
				if (fchown(fd, ent->uid, ent->gid) && !quiet && !write_metalog) {
					fprintf(stderr,
						"%s: can't chown %d:%d new log file: %s: %s.\n",
						argv0,
						ent->uid, ent->gid,
						newlog,
						strerror(errno));
				}
				if (fchmod(fd, ent->permissions) && !quiet && !write_metalog) {
					fprintf(stderr,
						"%s: can't chmod 0%03o new log file: %s: %s.\n",
						argv0,
						ent->permissions,
						newlog,
						strerror(errno));
				}
				if (close(fd) < 0) {
					fprintf(stderr,
						"%s: failed to close new log file: %s: %s.\n",
						argv0,
						newlog,
						strerror(errno));
				}
				if (rename(newlog, ent->log)) {
					fprintf(stderr,
						"%s: failed to rename new log file: %s -> %s: %s.\n",
						argv0,
						newlog,
						ent->log,
						strerror(errno));
					/*
					 * hopefully this never happens as
					 * there is some risk that a directory
					 * in the path to newlog could have
					 * just been changed to a symlink thus
					 * pointing the newlog pathname at some
					 * other existing file.  Of course
					 * we're only removing a file with a
					 * magic random name so the risk that
					 * its name matches one we wouldn't
					 * want to remove is almost zero....
					 */
					if (unlink(newlog) < 0) {
						fprintf(stderr,
							"%s: failed to unlink new log file template: %s: %s.\n",
							argv0,
							newlog,
							strerror(errno));
					}
					/*
					 * Non-Fatal!  Don't return here if the
					 * rename() failed -- it likely failed
					 * because the new log was already
					 * created by the process which wants
					 * to write to it.  In any case we want
					 * to go on and possibly send a signal
					 * anyway, as well as do the
					 * compression of the archived log.
					 */
				}
			}
			/*
			 * When run as part of the NetBSD system build we print
			 * part of a "METALOG" (mtree like) entry for every
			 * file created so that the NetBSD src/etc/Makefile can
			 * know what files were created and what permissions
			 * and ownerships they should have in the final
			 * installed system.  These lines will be fixed up with
			 * any localisations (e.g. append a "tag=etc" field) by
			 * a sed filter in the Makefile and then appened to the
			 * install METALOG file.
			 */
			if (write_metalog) {
				printf("%s type=file mode=%#o uname=%s gname=%s\n",
				       ent->log,
				       ent->permissions,
				       ent->user,
				       ent->group);
			}
		}
#ifdef LOG_TURNOVER_IN_NEW_FILE_TOO
		/*
		 * this is probably a bad idea -- it'll screw up the size test
		 * above, may use the wrong format entry, etc.; though of
		 * course if you want to cycle logs regardless of whether
		 * they're used, this is one way to do it....
		 *
		 * Note also this might not end up the first entry since it
		 * happens after the temporary new file has been renamed to
		 * become the live log file (though it does happen before the
		 * daemon is signaled).
		 */
		if (!(ent->flags & CE_BINARY) && (ent->flags & CE_NOCREATE)) {
			if (note_trim(ent->log)) {
				fprintf(stderr,
					"%s: can't add status message to log: %s: %s.\n",
					argv0,
					ent->log,
					strerror(errno));
			}
		}
#endif
	}
	if (might_need_newlog && !(ent->flags & CE_NOSIGNAL) && send_signals) {
		if (ent->pid_file)
			pid = get_pid_file(ent->pid_file);
		else
			pid = syslogd_pid;
		if (pid) { /* && ent->signum */
			char            signame[SIG2STR_MAX];

			if (sig2str(ent->signum, signame) == -1)
				sprintf(signame, "(unknown signal %d)", ent->signum);
			if (verbose) {
				printf("# sending SIG%s to process in %s, %d\n",
				       signame,
				       ent->pid_file ? ent->pid_file : syslogd_pidfile,
				       (int) pid);
			}
			if (show_script || debug) {
				notified = TRUE;	/* pretend it works.... */
				if (show_script)
					printf("kill -%d %d\n", ent->signum, (int) pid);
				if (!(ent->flags & CE_PLAIN0)) {
					if (verbose)
						puts("# small pause now to allow daemon to close log.");
					if (show_script)
						puts("sleep 5");
				}
			} else if (kill(pid, ent->signum)) {
				fprintf(stderr,
					"%s: cannot notify daemon with SIG%s, pid %d: %s.\n",
					argv0,
					signame,
					(int) pid,
					strerror(errno));
			} else {
				notified = TRUE;
				if ((ent->flags & CE_COMPACT) && !(ent->flags & CE_PLAIN0)) {
					if (verbose)
						printf("...small pause now to allow daemon to close log... ");
					(void) sleep(5);
					if (verbose)
						puts("done.");
				}
			}
		} else if (verbose)
			printf("# no signal sent for %s (no PID found)\n", ent->log);
	}
	if (might_timestamp && !(ent->flags & CE_BINARY))
		(void) note_trim(file1);
	if (ent->flags & CE_COMPACT) {
		int             rt;

		/* sprintf() is safe here */
		sprintf(zfile1,
			(ent->flags & CE_SUBDIR) ? "%s.old/%04u" : "%s.%u",
			ent->log,
			(ent->flags & CE_PLAIN0) ? 1 : 0);
		if (!(ent->flags & CE_PLAIN0) && pid && !notified) {
			fprintf(stderr,
				"%s: %s (level zero) not compressed because daemon was not notified.\n",
				argv0, zfile1);
		} else if (debug || show_script ||
			   ((rt = stat(zfile1, &st)) >= 0 && (st.st_size > 0))) {
			/*
			 * We'll compress the file if it's there to be
			 * compressed even if we didn't just do the rename
			 * (i.e. even if not need_compress)....
			 */
			compress_log(zfile1);	/* do the deed (or say how to) */
		} else if (need_compress || verbose) {
			/*
			 * .... but we'll only complain if we really expected
			 * to have to compress it (and we're not in debug mode
			 * or generating a script).
			 */
			fprintf(stderr, "%s: %s not compressed: %s.\n",
				argv0,
				zfile1,
				(rt < 0) ? "no such file" : "is empty");
		}
	}

	return;
}

/*
 * Note the fact that the logfile was turned over in the logfile
 *
 * XXX ideally this would try to use the same timestamp format as determined by
 * read_first_timestamp(), but on the other hand since syslog files are our
 * primary concern, syslog timestamp format will have to do.
 */
static int
note_trim(log)
	char           *log;
{
	FILE           *fp;

	if (verbose)
		printf("# adding time stamp to most recent archive %s\n", log);
	if (show_script) {
		printf("echo '%s %s newsyslog[%d]: logfile turned over' >> %s\n",
		       daytime, hostname, (int) getpid(), log);
	} else if (!debug) {
		if ((fp = fopen(log, "a")) == NULL) {
			fprintf(stderr, "%s: failed to open log to append trim notice: %s: %s.\n",
				argv0, log, strerror(errno));
			return (-1);
		}
		(void) fprintf(fp, "%s %s newsyslog[%d]: logfile turned over\n",
			       daytime, hostname, (int) getpid());
		if (fclose(fp) == EOF) {
			fprintf(stderr, "%s: failed to close log after appending trim notice: %s: %s.\n",
				argv0, log, strerror(errno));
			return (-1);
		}
	}

	return (0);
}

/*
 * Fork off a compression utility to compress the old log file
 */
static void
compress_log(log)
	char           *log;
{
	pid_t           pid;
	static char     c_path[] = PATH_COMPRESS;
	char           *c_prog;
	int             retries = MAX_FORK_RETRIES; /* try hard to fork() */

	assert(retries >= 0);	/* allow no foolish -D's! */

	c_prog = (c_prog = strrchr(c_path, '/')) ? c_prog + 1 : c_path;

	if (verbose)
		printf("# starting compressor [%s] as '%s -f %s &'\n", c_path, c_prog, log);
	if (show_script)
		printf("%s -f %s &\n", c_path, log);
	if (debug || show_script)
		return;
	/*
	 * XXX we probably could use vfork() if available since we immediately
	 * execl() the compressor, but that means yet another configure.in test
	 * ...  :-)
	 */
	while ((pid = fork()) < 0 && errno == EAGAIN && --retries >= 0) {
		fprintf(stderr, "%s: fork(): %s\n", argv0, strerror(errno));
		(void) sleep(2);	/* sleep a bit between retries */
	}
	if (pid < 0) {
		perror("fork()");
		return;
	} else if (!pid) {
		(void) execl(c_path, c_prog, "-f", log, (char *) NULL);
		perror(c_path);
		return;
	} else if (verbose)
		printf("%s: started '%s -f %s &' as pid# %d\n", argv0, c_prog, log, pid);
	/*
	 * NOTE: we'll just leave our zombie children to wait until we exit
	 */
	return;
}

/*
 * Return size, in kilobytes, of the specified file
 */
static off_t
check_file_size(file)
	char           *file;
{
	struct stat     sb;

	if (stat(file, &sb) < 0)
		return (-1);

	return (sb.st_size / 1024);
}

/*
 * Return the age of the old log file (file.0) in hours, or if the old file
 * does not exist then try to read the timestamp from the first log entry of
 * the current log file.
 */
static int
check_old_log_age(ent)
	struct conf_entry *ent;
{
	char           *file = ent->log;
	struct stat     sb;
	char            *tmp;

	if (!(tmp = malloc(strlen(file) + sizeof(".0") + sizeof(COMPRESS_SUFFIX)))) {
		perror(argv0);
		return (-1);
	}
	(void) strcpy(tmp, file);
	if (stat(strcat(tmp, ".0"), &sb) < 0) {
		if (stat(strcat(tmp, COMPRESS_SUFFIX), &sb) < 0) {
			if (ent->flags & CE_BINARY)
				return (-1); /* can't get tmstamp from a binary! */
			else if ((sb.st_mtime = read_first_timestamp(file)) <= 0)
				return (-1);
		}
	}
	free(tmp);

	/* 1/2 hr, or run_interval, older than reality */
	return (int) ((timenow - sb.st_mtime + (run_interval > 0) ? run_interval : 1800) / 3600);
}

/*
 * Make an attempt to guess the age of a log by trying to parse the timestamp
 * from its first entry.  Only a few well known timestamp formats are tried.
 * The reliability of this function rests squarely on the correctness of
 * the strptime() implementation.
 */
static time_t
read_first_timestamp(file)
	char           *file;
{
	FILE           *fp;
	char            line[BUFSIZ];
	time_t          tmstamp = -1;
	struct tm       tms;

	if ((fp = fopen(file, "r")) == NULL) {
		fprintf(stderr, "%s: can't open %s to read initial timestamp: %s.\n",
			argv0, file, strerror(errno));
		return tmstamp;
	}
	if (fgets(line, BUFSIZ, fp)) {
		tmstamp = parse_tmstamp(file, line);
	} else {
		fprintf(stderr, "%s: can't read %s for initial timestamp: %s.\n",
			argv0, file,
			feof(fp) ? "file is empty" :
			ferror(fp) ? strerror(errno) : "unknown fgets() failure");
		tmstamp = -1;
	}
	(void) fclose(fp);

	return tmstamp;
}

static time_t
parse_tmstamp(file, line)
	char           *file;
	char           *line;
{
	time_t          tmstamp = -1;
	struct tm       tms;

	bzero((void *) &tms, sizeof(tms));

	/*
	 * XXX common strptime(3) implementations do not provide built-in
	 * parsing for time zones.  Things get really messy if we want to try
	 * to handle timestamps which are not given in local time.
	 */
	if (strptime(line, "%b %d %T ", &tms)) /* syslog */
		;
	else if (strptime(line, "%m/%d/%Y %T", &tms)) /* old smail-3 */
		;
#if 0
	else if (strptime(line, "%Y-%m-%dT%TZ", &tms)) { /* strict ISO 8601 (UTC) */
		/* XXX adjust back to local time? */
	}
#endif
	else if (strptime(line, "%Y-%m-%dT%T", &tms)) /* strict ISO 8601 (correct separators, but no zone) */
		;
	else if (strptime(line, "%Y/%m/%d %T", &tms)) /* westernized ISO 8601 as used by smail-3 */
		;
	else if (strptime(line, "%Y/%m/%d-%T", &tms)) /* kinda ISO 8601 ("wrong" separators) */
		;
	else if (strptime(line, "[%d/%b/%Y:%T ", &tms)) /* httpd [DD/Mon/YYYY:HH:MM:SS +/-ZONE] ignoring zone */
		;
	else if (strptime(line, "%c", &tms)) /* ctime */
		;
	else if (strptime(line, "[%c]", &tms)) /* old httpd */
		;
	else {
		fprintf(stderr, "%s: can't parse initial timestamp from %s:\ntext is: '%s'",
			argv0, file, line);
		return -1;
	}
	tms.tm_isdst = -1;	/* make mktime() guess the right time */
	if ((tmstamp = mktime(&tms)) < 0) {
		struct tm *tmy;

		/* some formats don't include the current year, so we
		 * try to provide it....
		 */
		tmy = localtime(&timenow);
		tms.tm_year = tmy->tm_year;
		tmstamp = mktime(&tms);
	}
	if (tmstamp == -1) {
		fprintf(stderr, "%s: can't understand initial timestamp from %s:\n -- %s",
			argv0, file, line);
		return -1;
	}
	if (tmstamp > timenow) {
		tms.tm_year--;	/* musta been last year! */
		tmstamp = mktime(&tms);	/* XXX and if this fails? */
	}

	return tmstamp;
}

/*
 * Read a process ID from the specified file.
 *
 * Reports any errors to stderr, and then returns zero.
 */
static pid_t
get_pid_file(pid_file)
	const char     *pid_file;
{
	FILE           *fp;
	char            line[BUFSIZ];
	pid_t           pid = 0;	/* XXX should we use -1 instead? */

#ifdef notyet
	char		tmp[PATH_MAX];

	if (pid_file[0] != '/')
		snprintf(tmp, sizeof(tmp), "%s%s", _PATH_VARRUN, pid_file);
	else
		strlcpy(tmp, pid_file, sizeof(tmp));
#endif
	if ((fp = fopen(pid_file, "r")) == NULL) {
		fprintf(stderr, "%s: can't open PID file: %s: %s.\n",
			argv0, pid_file, strerror(errno));
		return 0;
	}
	if (fgets(line, BUFSIZ, fp)) {
		unsigned long   ulval;
		char           *ep;
		const char     *errmsg = NULL;

		errno = 0;
		ulval = strtoul(line, &ep, 10);
		if (line[0] == '\0')
			errmsg = "empty file";
		else if (line[0] == '-')
			errmsg = "process numbers should not be negated";
		else if (ep == line)
			errmsg = "first line does not start with a valid number";
		else if (*ep != '\0' && *ep != '\n')
			errmsg = "not a valid BASE-10 integer (trailing non-digit characters)";
		else if (errno == ERANGE && ulval == ULONG_MAX)
			errmsg = "number out of range for conversion";
		else if (ulval == 0 /* && errno == 0 */)
			errmsg = "zero is never a valid process number";
		else if (ulval < MIN_PID || ulval > MAX_PID)
			errmsg = "preposterous process number";
		if (errmsg)
			fprintf(stderr, "%s: %s: %s: in '%s'", argv0, pid_file, errmsg, line);
		else if (errno != 0)
			fprintf(stderr, "%s: %s: strtoul(): %s: converting '%s'", argv0, pid_file, strerror(errno), line);
		else
			pid = (pid_t) ulval;
	} else {
		fprintf(stderr, "%s: can't read pid file: %s: %s.\n", argv0, pid_file,
			feof(fp) ? "file is empty" : ferror(fp) ? strerror(errno) : "unknown fgets() failure");
	}
	(void) fclose(fp);

	return pid;
}

/*
 * Skip Over Blanks
 */
static char    *
strsob(p)
	register char  *p;
{
	if (!p)
		return (0);
	while (*p && isspace(*p))
		p++;

	return (p);
}

/*
 * Skip Over Non-Blanks
 */
static char    *
strson(p)
	register char  *p;
{
	if (!p)
		return (0);
	while (*p && !isspace(*p))
		p++;

	return (p);
}

/*
 * Translate a signal number or name into an integer
 */
static int
getsig(sig)
	char *sig;
{
	int n;

	if (!strncasecmp(sig, "sig", (size_t) 3))
		sig += 3;
	if (str2sig(sig, &n) != -1)
		return n;

	return (-1);
}

/*
 * Parse a cyclic time specification, the format is as follows:
 *
 *	Dhh or Wd[Dhh] or Mdd[Dhh]
 *
 * to rotate a logfile cyclic
 *
 *	- every day (D) within a specific hour (hh)	(hh = 0...23)
 *	- once a week (W) at a specific day (d)    OR	(d = 0..6, 0 = Sunday)
 *	- once a month (M) at a specific day (d)	(d = 1..31,l|L)
 *
 * We don't accept a timezone specification; missing fields are defaulted to
 * the current date but with time-of-day at zero (i.e. midnight).
 */
static int
parse_dwm(s, trim_at)
	char *s;
	time_t *trim_at;
{
	char *t = NULL;
	struct tm tms;
	u_long ul;
	int nd;
	static int mtab[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int WMseen = FALSE;
	int Dseen = FALSE;

	tms = *localtime(&timenow);

	/*
	 * set up the number of days per month
	 *
	 * in theory this could also be done using strftime(%d, now+24h) to see
	 * if tomorrow is the first day of the month or not....
	 */
	nd = mtab[tms.tm_mon];

	if (tms.tm_mon == 1) {
		if (((tms.tm_year + 1900) % 4 == 0) &&
		    ((tms.tm_year + 1900) % 100 != 0) &&
		    ((tms.tm_year + 1900) % 400 == 0)) {
			nd++;   /* leap year, 29 days in february */
		}
	}
	tms.tm_hour = tms.tm_min = tms.tm_sec = 0;

	for (;;) {
		switch (*s) {
		case '-':
		case '$':
			/* this is primarily just to skip any optional initial
			 * hyphen, but also allows sub-fields in the time spec
			 * to be hyphen separated as well...
			 */
			t = ++s;
			break;

		case 'd':
		case 'D':
			if (Dseen) {
				fprintf(stderr, "%s: already saw a 'D' in trimtime!\n", argv0);
				return (-1);
			}
			Dseen = TRUE;
			s++;
			errno = 0;
			ul = strtoul(s, &t, 10);
			if (errno != 0 || ul > 23) {
				fprintf(stderr, "%s: nonsensical hour-of-the-day (D) value: %lu!\n", argv0, ul);
				return (-1);
			}
			tms.tm_hour = (int) ul;
			break;

		case 'w':
		case 'W':
			if (WMseen) {
				fprintf(stderr, "%s: already saw a 'W' in trimtime!\n", argv0);
				return (-1);
			}
			WMseen = TRUE;
			s++;
			errno = 0;
			ul = strtoul(s, &t, 10);
			if (errno != 0 || ul > 6) {
				fprintf(stderr, "%s: nonsensical day-of-the-week (W) value: %lu!\n", argv0, ul);
				return (-1);
			}
			if (ul != (unsigned long) tms.tm_wday) {
				int save;

				if (ul < (unsigned long) tms.tm_wday) {
					save = 6 - tms.tm_wday;
					save += ((int) ul + 1);
				} else {
					save = (int) ul - tms.tm_wday;
				}
				tms.tm_mday += save;
				if (tms.tm_mday > nd) {
					tms.tm_mon++;
					tms.tm_mday = tms.tm_mday - nd;
				}
			}
			break;

		case 'm':
		case 'M':
			if (WMseen) {
				fprintf(stderr, "%s: already saw a 'M' in trimtime!\n", argv0);
				return (-1);
			}
			WMseen = TRUE;
			s++;
			if (tolower(*s) == 'l') {
				tms.tm_mday = nd;
				s++;
				t = s;
			} else {
				errno = 0;
				ul = strtoul(s, &t, 10);
				/* if (s == t) then (ul == 0 && errno == ERANGE) */
				if (ul < 1 || ul > 31 || errno != 0) {
					fprintf(stderr, "%s: nonsensical day-of-the-month (M) value: %lu!\n", argv0, ul);
					return (-1);
				}
				/*
				 * XXX should we warn about any number > 28,
				 * not just if it's too large for this month?
				 */
				if (ul > (unsigned long) nd) {
					fprintf(stderr, "%s: day-of-the-month (M) value out of range for month %d: %lu, using %d\n", argv0, tms.tm_mon, ul, nd);
					ul = nd;
				}
				tms.tm_mday = (int) ul;
			}
			break;

		default:
			fprintf(stderr, "%s: invalid trimtime specification: '%c'!\n", argv0, *s);
			return (-1);
		}

		if (*t == '\0' || isspace(*t))
			break;
		else
			s = t;
	}

	if ((*trim_at = mktime(&tms)) == (time_t) -1) {
		fprintf(stderr, "%s: mktime() failed!\n", argv0);
		return (-1);
	}

	return 0;
}
