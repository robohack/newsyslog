/*
 *	newsyslog - roll over selected logs at the appropriate time,
 *		keeping the a specified number of backup files around.
 */

/*
 * This file contains changes from Greg A. Woods; Planix, Inc.
 *
 * Copyright for these changes is hereby assigned to MIT.
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
	"@(#)newsyslog:$Name:  $:$Id: newsyslog.c,v 1.32 2000/12/08 20:45:32 woods Exp $";
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#else
extern void exit();
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/proc.h>			/* PID_MAX ? */
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
# include <strings.h>
extern char *strchr(), *strrchr(), *strtok();
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#else
# ifndef errno
extern int errno;
# endif /* !errno */
#endif /* HAVE_ERRNO_H */
#include <assert.h>

#ifndef PATH_MAX
# ifdef MAXPATHLEN
#  define PATH_MAX	MAXPATHLEN
# else
#  define PATH_MAX	1024
# endif
#endif /* PATH_MAX */

#ifndef TRUE					/* XXX should be #undef ??? */
# define TRUE		1
#endif
#ifndef FALSE					/* XXX should be #undef ??? */
# define FALSE		0
#endif

#define MAX_PERCENTD	10	 /* XXX should this be based on sizeof(int)?
				  * the maximum number of ASCII digits in an
				  * integer, used in calculating pathname
				  * buffer sizes
				  */

#ifndef _PATH_DEVNULL
# define _PATH_DEVNULL	"/dev/null"
#endif

#ifndef HAVE_BZERO
# define bzero(p, l)	memset((p), '0', (l));
#endif
#define kbytes(size)	(((size) + 1023) >> 10)
#ifdef _IBMR2
# define dbtob(db)	((unsigned)(db) << UBSHIFT) /* Calculates (db * DEV_BSIZE) */
#endif

#define CE_COMPACT	001	/* Compact the achived log files */
#define CE_BINARY	002	/* Logfile is binary, don't add status mesg */
#define CE_PLAIN0	004	/* Keep .0 file uncompressed */
#define CE_NOSIGNAL	010	/* Don't send a signal when file is trimmed */
#define CE_NOCREATE	020	/* Don't create the new log file */
#define CE_TRIMAT	040	/* we have a valid trim time specification
				 * XXX we could use the value -1 in the field
				 * (struct conf_entry *)->trim_at to indicate
				 * no trim time spec was given, but that means
				 * adding another place where it's harder to
				 * change time_t to an unsigned value...
				 */

#define NO_ID		((uid_t) -1)	/* no UID/GID specified -- preserve */

struct conf_entry {
	char           *log;		/* Name of the log */
	char           *pid_file;	/* PID file */
	uid_t           uid;		/* Owner of log */
	uid_t           gid;		/* Group of log */
	int             numlogs;	/* Number of logs to keep */
	int             size;		/* maximum log size in KB */
	int             hours;		/* maximum hours between log trimming */
	time_t          trim_at;	/* time to trim log at */
	int             permissions;	/* File permissions on the log */
	int             flags;		/* Flags (CE_*)  */
	int             signum;		/* Signal to send to daemon (SIG*) */
	struct conf_entry *next;	/* Linked list pointer */
};

char           *argv0 = PACKAGE;
char            package[] = PACKAGE;	/* the original dist name */
char            version[] = VERSION;

int             verbose = 0;	/* Print out what's going on */
int             needroot = 1;	/* Root privs are necessary for default conf */
int             noaction = 0;	/* Don't do anything, just show it */
int             domidnight = -1;/* ignore(-1) do(1) don't(0) do midnight run */
int             run_interval = -1;/* interval at which we are run by cron */
int             force = 0;	/* force all files to be trimmed */
char           *config_file = PATH_CONFIG;/* Configuration file to use */
char           *syslogd_pidfile = PATH_SYSLOGD_PIDFILE;/* syslogd's pid file */
time_t          timenow;
pid_t           syslogd_pid;	/* read in from /etc/syslog.pid */
char            hostname[MAXHOSTNAMELEN + 1];	/* hostname */
char           *localdomain = NULL;
char           *daytime;		/* timenow in human readable form */

/*
 * MIN_PID & MAX_PID are used to sanity-check the pid_file contents.
 */
#ifndef MIN_PID
# define MIN_PID	5
#endif
#ifndef MAX_PID
# ifdef MAXPID
#  define MAX_PID	MAXPID
# else
#  ifdef PID_MAX
#   define MAX_PID	PID_MAX
#  else
#   define MAX_PID	30000
#  endif
# endif
#endif

#if (!defined(OSF) && !defined(BSD)) || ((BSD + 0) < 199103)	/* XXX HACK!!! */
extern char            *strdup __P((const char *));
#endif

int                     main __P((int, char **)); /* XXX HACK for WARNS=1 */

static struct conf_entry *parse_file __P((char **));
static char            *strsob __P((char *));
static char            *strson __P((char *));
static char            *missing_field __P((char *, int, char *));
static void             do_entry __P((struct conf_entry *));
static void             parse_options __P((int, char **));
static void             usage __P((void));
static void             do_trim __P((struct conf_entry *));
static int              note_trim __P((char *));
static void             compress_log __P((char *));
static int              check_file_size __P((char *));
static int              check_old_log_age __P((struct conf_entry *));
static time_t           read_first_timestamp __P((char *));
static pid_t            get_pid_file __P((char *));
static int              isnumber __P((char *));
static int              getsig __P((char *));
static int              parse_dwm __P((char *, time_t *));

extern int              optind;
extern char            *optarg;

int
main(argc, argv)
	int             argc;
	char           *argv[];
{
	struct conf_entry *p, *q;

	argv0 = (argv0 = strrchr(argv[0], '/')) ? argv0 + 1 : argv[0];

	parse_options(argc, argv);

	if (needroot && getuid() && geteuid()) {
		fprintf(stderr, "%s: you do not have root privileges\n", argv0);
		exit(1);
	}

	p = q = parse_file(argv + optind);

	syslogd_pid = get_pid_file(syslogd_pidfile);

	while (p) {
		do_entry(p);
		p = p->next;
		free((char *) q);
		q = p;
	}
	return (0);
}

static void
do_entry(ent)
	struct conf_entry *ent;

{
	int             we_trim_it = 0;
	int             size;			/* in kbytes */
	int             modtime = 0;		/* in hours */

	if (verbose) {
		printf("%s <#%d,%s%s%s%s%s>: ", ent->log, ent->numlogs,
		       (ent->flags & CE_NOCREATE) ? "C" : "",
		       (ent->flags & CE_COMPACT) ? "Z" : "",
		       (ent->flags & CE_BINARY) ? "b" : "",
		       (ent->flags & CE_NOSIGNAL) ? "n" : "",
		       (ent->flags & CE_PLAIN0) ? "0" : "");
	}
	size = check_file_size(ent->log);
	if (size < 0 && verbose)
		printf("does not exist ");
	else if (size == 0 && verbose)
		printf("is empty ");
	if (size > 0) {				/* ignore empty/missing ones */
		if (verbose && (ent->size > 0))
			printf("size (Kb): %d [allow %d] ", size, ent->size);
		if ((ent->size > 0) && (size >= ent->size))
			we_trim_it = 1;
		if (ent->hours > 0) {
			modtime = check_old_log_age(ent);
			if (verbose)
				printf(" age (hr): %d [allow %d] ", modtime, ent->hours);
			/* always trim if timestamp FUBAR */
			if (modtime >= ent->hours || modtime < 0)
				we_trim_it = 1;
		} else if (ent->flags & CE_TRIMAT) {
			/*
			 * if there was no interval, but just a trim time spec
			 * then check to see if it's time to trim it now...
			 */
			if ((timenow >= ent->trim_at) &&
			    (difftime(timenow, ent->trim_at) <= 60 * (run_interval - 1))) {
				printf("(time to trim) ");
				we_trim_it = 1;
			}
		}
	}
	assert(domidnight == -1 || domidnight == 1 || domidnight == 0);
	if (domidnight == -1) {
		if (verbose)
			printf("(regular, no -m/-M) ");
		if (ent->flags & CE_TRIMAT) {
			/*
			 * if there was a trim time spec, make sure we're
			 * within the valid time interval
			 */
			if ((timenow < ent->trim_at) ||
			    (difftime(timenow, ent->trim_at) > 60 * (run_interval - 1))) {
				if (verbose)
					printf("(not time to trim) ");
				we_trim_it = 0;
			}
		}
	} if (domidnight == 1 && (ent->hours % 24) == 0) {
		/*
		 * we've already set we_trim_it above if the file is as old or
		 * older than ent->hours
		 */
		if (verbose && we_trim_it)
			printf("(daily with -m) ");
	} else if (domidnight == 0 && (ent->hours % 24) == 0) {
		if (verbose && !force)
			printf("(not the daily trim) ");
		we_trim_it = 0;
	}
	if (force & !we_trim_it) {
		if (verbose)
			printf("(forced )");
		we_trim_it = 1;
	}
	if (we_trim_it) {
		if (verbose)
			printf("--> trimming log...\n");
		do_trim(ent);
	} else {
		if (verbose)
			printf("--> skipping.\n");
	}
	if (ent->log)
		free(ent->log);
	if (ent->pid_file)
		free(ent->pid_file);
}

static void
parse_options(argc, argv)
	int             argc;
	char          **argv;
{
	int             c;
	long            l;
	char           *p;

	timenow = time((time_t *) 0);
	daytime = ctime(&timenow) + 4;
	daytime[15] = '\0';

	/* Let's get our hostname */
	(void) gethostname(hostname, sizeof(hostname));

	/* and truncate the domain part off */
	if ((p = strchr(hostname, '.'))) {
		*p++ = '\0';
		localdomain = p;
	} else
		localdomain = "";
	optind = 1;		/* Start options parsing */
	while ((c = getopt(argc, argv, "FMVf:i:mnp:rt:v")) != -1)
		switch (c) {
		case 'F':
			force = 1;
			break;
		case 'M':		/* NOT the midnight run */
			domidnight = 0;
			break;
		case 'V':
			printf("%s: version %s-%s.\n", argv0, package, version);
			exit(0);
			/* NOTREACHED */
		case 'f':
			config_file = optarg;
			break;
		case 'i':		/* run interval in minutes */
			l = strtol(optarg, (char **) NULL, 10);
			if (l == LONG_MIN || l == LONG_MAX) {
				fprintf(stderr,
					"%s: run interval of '%s' is not valid\n",
					argv0,
					optarg);
				exit(2);
			} else if (l > INT_MAX) {
				fprintf(stderr,
					"%s: run interval of %ld is too large\n",
					argv0,
					l);
				exit(2);
			}
			run_interval = (int) l;
			break;
		case 'm':
			domidnight = 1;	/* the midnight run */
			break;
		case 'n':
			noaction++;
			needroot = 0;	/* don't need needroot */
			break;
		case 'p':
			syslogd_pidfile = optarg;
			break;
		case 'r':
			needroot = 0;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}
}

static void
usage()
{
	fprintf(stderr,
		"Usage: %s [-V] [-M|-m] [-FInrv] [-f config-file] [-p syslogd-pidfile] [file ...]\n",
		argv0);
	exit(1);
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
	int             eol;
	int             lnum = 0;
	char            prev = '\0';

	if (strcmp(config_file, "-") == 0) {
		fp = stdin;
		config_file = "STDIN";
	} else {
		int	fd;

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
		 * NOTE: because we want an  exclusive lock, we have to ask for
		 * a "write"  lock, and on  some systems (eg.  some/all 4.4BSD)
		 * this  means we  need to  open the  file in  read/write mode.
		 * This is really stupid, but that's the way of POSIX!  :-/
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
				if (verbose)
					printf("skipping %s...\n", q);
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
		if ((group = strchr(q, ':')) != NULL) {
			*group++ = '\0';
			if (*q) {
				if (!(isdigit(*q))) {
					if ((pass = getpwnam(q)) == NULL) {
						fprintf(stderr,
							"%s: error in config file; unknown user: '%s' in line:\n%6d:\t'%s'\n",
							argv0,
							q,
							lnum,
							errline);
						exit(1);
					}
					working->uid = pass->pw_uid;
				} else
					working->uid = atoi(q);
			} else
				working->uid = NO_ID;

			q = group;
			if (*q) {
				if (!(isdigit(*q))) {
					if ((grp = getgrnam(q)) == NULL) {
						fprintf(stderr,
							"%s: error in config file; unknown group: '%s' in line:\n%6d:\t'%s'\n",
							argv0,
							q,
							lnum,
							errline);
						exit(1);
					}
					working->gid = grp->gr_gid;
				} else
					working->gid = atoi(q);
			} else
				working->gid = NO_ID;

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
		} else
			working->uid = working->gid = NO_ID;

		if (!sscanf(q, "%o", &working->permissions)) {
			fprintf(stderr,
				"%s: error in config file; bad permissions: '%s' in line:\n%6d:\t'%s'\n",
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
		if (!sscanf(q, "%d", &working->numlogs)) {
			fprintf(stderr,
				"%s: error in config file; bad number: '%s' in line\n%6d:\t'%s'\n",
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
		if (isdigit(*q))
			working->size = atoi(q);
		else
			working->size = -1;

		q = parse = missing_field(strsob(++parse), lnum, errline);
		if (*q == '#') {
			eol = 1;
			q = NULL;
		}
		parse = strson(parse);
		eol = !*parse;
		*parse = '\0';

		working->hours = -1;
		working->trim_at = -1;

		if (*q != '*') {
			u_long ul;

			if (isdigit(*q)) {
				if ((ul = strtol(q, &q, 10)) > INT_MAX) {
					fprintf(stderr,
						"%s: error in config file; interval too large: '%s' in line\n%6d:\t'%s'\n",
						argv0,
						q,
						lnum,
						errline);
					exit(1);
				}
				working->hours = ul;
			}
			/*
			 * interval may be followed by a specification of when
			 * to trim the file....
			 */
			if ((*q == '-') || (working->hours == -1)) {
				if (domidnight != -1) {
					fprintf(stderr,
						"%s: trim time nonsensical with %s: '%s' in line\n%6d:\t'%s'\n",
						argv0,
						(domidnight == 0) ? "-M" : "-m",
						q,
						lnum,
						errline);
					exit(1);
				}
				if (parse_dwm(q, &working->trim_at) == -1) {
					fprintf(stderr,
						"%s: error in config file; malformed trim time: '%s' in line\n%6d:\t'%s'\n",
						argv0,
						q,
						lnum,
						errline);
					exit(1);
				}
				if (run_interval == -1) {
					fprintf(stderr,
						"%s: a trim time needs '-i run_interval': '%s' in line\n%6d:\t'%s'\n",
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
					"%s: error in config file; malformed interval or trim time: '%s' in line\n%6d:\t'%s'\n",
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
				eol = 1;
				q = NULL;
			}
			parse = strson(parse);
			if (!*parse)
				eol = 1;
			*parse = '\0';
		}
		working->flags = 0;
		while (q && *q && !isspace(*q)) {
			if ((*q == 'D') || (*q == 'd'))
				working->flags |= CE_NOCREATE;
			else if ((*q == 'Z') || (*q == 'z'))
				working->flags |= CE_COMPACT;
			else if ((*q == 'B') || (*q == 'b'))
				working->flags |= CE_BINARY;
			else if ((*q == 'N') || (*q == 'n'))
				working->flags |= CE_NOSIGNAL;
			else if ((*q == '0') || (*q == 'p') || (*q == 'P'))
				working->flags |= CE_PLAIN0;
			else if (*q != '-') {
				fprintf(stderr,
					"%s: illegal flag in config file -- %c on line:\n%6d:\t'%s'\n",
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
				eol = 1;
				q = NULL;
			}
			parse = strson(parse);
			if (!*parse)
				eol = 1;
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
				eol = 1;
				q = NULL;
			}
			*(parse = strson(parse)) = '\0';
		}
		working->signum = SIGHUP;
		if (q && *q) {
			if ((working->signum = getsig(q)) < 0) {
				fprintf(stderr,
					"%s: illegal signal name/number in config file: %s, on line:\n%6d:\t'%s'\n",
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
	(void) fclose(fp);

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
 * Do the actual aging and trimming of log files.  Note that errors in this
 * stage are not fatal.
 */
static void
do_trim(ent)
	struct conf_entry *ent;
{
	char            file1[PATH_MAX - sizeof(COMPRESS_SUFFIX) - MAX_PERCENTD - 1];
	char            file2[PATH_MAX - sizeof(COMPRESS_SUFFIX) - MAX_PERCENTD - 1];
	char            zfile1[PATH_MAX];
	char            zfile2[PATH_MAX];
	int             log_exists;
	int             notified;
	int             need_notification;
        int             fd;
	int             o_numlogs;
	struct stat     st;
	pid_t           pid;

	/*
	 * first learn about the existing log file, if it exists
	 */
	if (stat(ent->log, &st) < 0) {
		fprintf(stderr,
			"%s: can't stat file (will %s): %s: %s.\n",
			argv0,
			(ent->flags & CE_NOCREATE) ? "ignore" : "create",
			ent->log,
			strerror(errno));
		log_exists = FALSE;
		/*
		 * Most implementations of chown(2) "do the right thing" with
		 * -1, but some don't (eg. AIX-3.1), so we'll just play it safe
		 * and always set them to the most likely correct values.
		 */
		if (ent->uid == NO_ID)
			ent->uid = geteuid();
		if (ent->gid == NO_ID)
			ent->gid = getegid();
	} else {
		log_exists = TRUE;
		/*
		 * preserve the file's UID/GID if none specified in the config.
		 * Note that we also "clobber" the ownerships of aged files too.
		 */
		if (ent->uid == NO_ID)
			ent->uid = st.st_uid;
		if (ent->gid == NO_ID)
			ent->gid = st.st_gid;
	}
	/* Remove oldest log */
	if (snprintf(file1, sizeof(file1), "%s.%d", ent->log, ent->numlogs) >= sizeof(file1)) {
		fprintf(stderr, "%s: filename too long: %s.\n", argv0, ent->log);
		return;
	}
	(void) strcpy(zfile1, file1);
	(void) strcat(zfile1, COMPRESS_SUFFIX);

	if (noaction) {
		printf("rm -f %s\n", file1);
		printf("rm -f %s\n", zfile1);
	} else {
		/* don't really care if the last in rotation exists or not */
		(void) unlink(file1);
		(void) unlink(zfile1);
	}
	/* Move down log files */
	o_numlogs = ent->numlogs;	/* preserve */
	while (ent->numlogs--) {
		(void) strcpy(file2, file1);
		(void) sprintf(file1, "%s.%d", ent->log, ent->numlogs); /* sprintf() OK here */
		(void) strcpy(zfile1, file1);
		(void) strcpy(zfile2, file2);
		if (stat(file1, &st) < 0) {
			(void) strcat(zfile1, COMPRESS_SUFFIX);
			(void) strcat(zfile2, COMPRESS_SUFFIX);
			if (stat(zfile1, &st) < 0)
				continue; /* not this many aged files yet... */
		}
		if (noaction) {
			printf("mv %s %s\n", zfile1, zfile2);
			printf("chmod %o %s\n", ent->permissions, zfile2);
			printf("chown %d:%d %s\n",
			       ent->uid, ent->gid, zfile2);
		} else {
			(void) rename(zfile1, zfile2); /* XXX error check (non-fatal?) */
			(void) chmod(zfile2, ent->permissions); /* XXX error check (non-fatal?) */
			(void) chown(zfile2, ent->uid, ent->gid); /* XXX error check (non-fatal?) */
		}
	}
	if (log_exists && st.st_size > 0) {
		/*
		 * are we keeping any aged files at all?
		 */
		if (o_numlogs) {
			if (noaction)
				printf("mv %s %s\n", ent->log, file1);
			else {
				if (rename(ent->log, file1) < 0) {
					fprintf(stderr,
						"%s: can't rename file: %s to %s: %s.\n",
						argv0,
						ent->log,
						file1,
						strerror(errno));
				}
			}
			if (!(ent->flags & CE_BINARY)) {
				if (note_trim(file1)) {
					fprintf(stderr,
						"%s: can't add final status message to log: %s: %s.\n",
						argv0,
						file1,
						strerror(errno));
				}
			}
		} else {
			/*
			 * if not just remove the current file...
			 */
			if (noaction)
				printf("rm %s\n", ent->log);
			else {
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
	if (noaction && !(ent->flags & CE_NOCREATE)) {
		printf("touch %s\n", ent->log);
		printf("chown %d:%d %s\n", ent->uid, ent->gid, ent->log);
	} else if (!(ent->flags & CE_NOCREATE)) {
		fd = creat(ent->log, ent->permissions);
		if (fd < 0) {
			fprintf(stderr,
				"%s: can't create new log: %s: %s.\n",
				argv0,
				ent->log,
				strerror(errno));
		} else {
			if (fchown(fd, ent->uid, ent->gid)) {
				fprintf(stderr,
					"%s: can't chown new log file: %s: %s.\n",
					argv0,
					ent->log,
					strerror(errno));
			}
			if (close(fd) < 0) {
				fprintf(stderr,
					"%s: failed to close new log file: %s: %s.\n",
					argv0,
					ent->log,
					strerror(errno));
			}
		}
	}
#ifdef LOG_TURNOVER_IN_NEW_FILE_TOO
	/*
	 * this is probably a bad idea -- it'll screw up the size test above,
	 * may use the wrong format entry, etc.; though of course if you want
	 * to cycle logs regardless of whether they're used, this is one way to
	 * do it....
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
	if (noaction && !(ent->flags & CE_NOCREATE))
		printf("chmod %o %s\n", ent->permissions, ent->log);
	else if (!(ent->flags & CE_NOCREATE)) {
		if (chmod(ent->log, ent->permissions) < 0) {
			fprintf(stderr,
				"%s: can't chmod log file %s: %s.\n",
				argv0,
				ent->log,
				strerror(errno));
		}
	}
	pid = 0;
	need_notification = 0;
	notified = 0;
	if (ent->pid_file && !(ent->flags & CE_NOSIGNAL)) {
		need_notification = 1;
		pid = get_pid_file(ent->pid_file);
	} else if (!(ent->flags & CE_NOSIGNAL)) {
		need_notification = 1;
		pid = syslogd_pid;
	}
	if (pid) {
		if (noaction) {
			notified = 1;
			printf("kill -%d %d\n", ent->signum, (int) pid);
			if (!(ent->flags & CE_PLAIN0))
				puts("sleep 5");
		} else if (kill(pid, ent->signum)) {
			fprintf(stderr,
				"%s: can't notify daemon, pid %d: %s\n.",
				argv0,
				(int) pid,
				strerror(errno));
		} else {
			notified = 1;
			if (verbose)
				printf("daemon with pid %d notified\n", (int) pid);
			if (!(ent->flags & CE_PLAIN0)) {
				if (verbose)
					printf("small pause now to allow daemon to close log... ");
				sleep(5);
				if (verbose)
					puts("done.");
			}
		}
	}
	if ((ent->flags & CE_COMPACT)) {
		int             rt;

		/* sprintf() is OK here */
		sprintf(zfile1,
			"%s.%s",
			ent->log,
			(ent->flags & CE_PLAIN0) ? "1" : "0");
		if (!(ent->flags & CE_PLAIN0) && need_notification && !notified) {
			fprintf(stderr,
				"%s: %s not compressed because daemon not notified.\n",
				argv0,
				zfile1);
		} else {
			if (noaction) {
				printf("%s %s &\n", PATH_COMPRESS, zfile1);
			} else if ((rt = stat(zfile1, &st)) >= 0 && (st.st_size > 0)) {
				compress_log(zfile1);
			} else if (verbose) {
				printf("%s: %s not compressed: %s.\n",
				       argv0,
				       zfile1,
				       (rt < 0) ? "no such file" : "is empty");
			}
		}
	}

	return;
}

/*
 * Note the fact that the  were turned over
 */
static int
note_trim(log)
	char           *log;
{
	FILE           *fp;

	if (noaction) {
		(void) printf("echo '%s %s newsyslog[%d]: logfile turned over' >> %s\n",
			      daytime, hostname, (int) getpid(), log);
	} else {
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

	c_prog = (c_prog = strrchr(c_path, '/')) ? c_prog + 1 : c_path;

	pid = fork();
	if (pid < 0) {
		perror("fork()");
		return;
	} else if (!pid) {
		(void) execl(c_path, c_prog, "-f", log, 0);
		perror(c_path);
		return;
	} else if (verbose)
		printf("%s: started '%s %s &'\n", argv0, c_prog, log);
	/*
	 * NOTE: we'll just leave our zombie children to wait until we exit
	 */
	return;
}

/*
 * Return size, in kilobytes, of the specified file
 */
static int
check_file_size(file)
	char           *file;
{
	struct stat     sb;

	if (stat(file, &sb) < 0)
		return (-1);
	return (kbytes(dbtob(sb.st_blocks)));
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
	return ((timenow - sb.st_mtime + 1800) / 3600);	/* 1/2 hr older than reality */
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
		return -1;
	}
	if (fgets(line, BUFSIZ, fp)) {
		bzero((void *) &tms, sizeof(tms));
		if (strptime(line, "%b %d %T ", &tms)) /* syslog */
			;
		else if (strptime(line, "%m/%d/%Y %T", &tms)) /* smail */
			;
		else if (strptime(line, "%Y/%m/%d %T", &tms)) /* ISO */
			;
		else if (strptime(line, "%Y/%m/%d-%T", &tms)) /* ISO */
			;
		else if (strptime(line, "[%d/%b/%Y:%T ", &tms)) /* httpd [DD/Mon/YYYY:HH:MM:SS +/-ZONE] */
			;
		else if (strptime(line, "%c", &tms)) /* ctime */
			;
		else if (strptime(line, "[%c]", &tms)) /* old httpd */
			;
		else {
			fprintf(stderr, "%s: can't parse initial timestamp from %s:\n --> %s",
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
			tmstamp = mktime(&tms);
		}
	} else {
		fprintf(stderr, "%s: can't read %s for initial timestamp: %s.\n",
			argv0, file,
			feof(fp) ? "file is empty" :
			ferror(fp) ? strerror(errno) : "unknown fgets() failure");
		(void) fclose(fp);
		return -1;
	}
	(void) fclose(fp);

	return tmstamp;
}

/*
 * Read a process ID from the specified file.
 */
static pid_t
get_pid_file(pid_file)
	char           *pid_file;
{
	FILE           *fp;
	char            line[BUFSIZ];
	pid_t           pid = 0;

	if ((fp = fopen(pid_file, "r")) == NULL) {
		fprintf(stderr, "%s: can't open pid file: %s: %s.\n",
			argv0, pid_file, strerror(errno));
		return 0;
	}
	if (fgets(line, BUFSIZ, fp)) {
		pid = atol(line);
		if (pid < MIN_PID || pid > MAX_PID) {
			fprintf(stderr, "%s: preposterous process number: %s.\n",
				argv0, line);
			pid = 0;
		}
	} else {
		fprintf(stderr, "%s: can't read pid file: %s: %s.\n", argv0, pid_file,
			feof(fp) ? "file is empty" : ferror(fp) ? strerror(errno) : "unknown fgets() failure");
		pid = 0;
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
	while (p && *p && isspace(*p))
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
	while (p && *p && !isspace(*p))
		p++;
	return (p);
}

/*
 * Check if string is actually a number, i.e. *all* digits
 */
static int
isnumber(p)
	char *p;
{
	while (*p != '\0') {
		if (*p < '0' || *p > '9')
			return(0);
		p++;
	}
	return (1);
}

/*
 * Translate a signal number or name into an integer
 */
static int
getsig(sig)
	char *sig;
{
	int n;

	if (isnumber(sig)) {
		n = strtol(sig, &sig, 0);
		if ((unsigned)n >= NSIG)
			return (-1);
		return (n);
	}

	if (!strncasecmp(sig, "sig", 3))
		sig += 3;
	for (n = 1; n < NSIG; n++) {
		if (!strcasecmp(sys_signame[n], sig))
			return (n);
	}
	return (-1);
}

/*-
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
	char *t;
	struct tm tm;
	struct tm *tmp;
	u_long ul;
	int nd;
	static int mtab[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int WMseen = 0;
	int Dseen = 0;

	tmp = localtime(&timenow);
	tm = *tmp;

	/* set up the no. of days per month */

	nd = mtab[tm.tm_mon];

	if (tm.tm_mon == 1) {
		if (((tm.tm_year + 1900) % 4 == 0) &&
		    ((tm.tm_year + 1900) % 100 != 0) &&
		    ((tm.tm_year + 1900) % 400 == 0)) {
			nd++;   /* leap year, 29 days in february */
		}
	}
	tm.tm_hour = tm.tm_min = tm.tm_sec = 0;

	for (;;) {
		switch (*s) {
		case '-':
			/* this is primarily to skip any optional first hyphen,
			 * but also allows sub-fields to be hyphen separated
			 */
			s++;			
			break;

		case 'd':
		case 'D':
			if (Dseen)
				return (-1);
			Dseen++;
			s++;
			ul = strtoul(s, &t, 10);
			if (ul < 0 || ul > 23)
				return (-1);
			tm.tm_hour = ul;
			break;

		case 'w':
		case 'W':
			if (WMseen)
				return (-1);
			WMseen++;
			s++;
			ul = strtoul(s, &t, 10);
			if (ul < 0 || ul > 6)
				return (-1);
			if (ul != tm.tm_wday) {
				int save;

				if (ul < tm.tm_wday) {
					save = 6 - tm.tm_wday;
					save += (ul + 1);
				} else {
					save = ul - tm.tm_wday;
				}

				tm.tm_mday += save;

				if (tm.tm_mday > nd) {
					tm.tm_mon++;
					tm.tm_mday = tm.tm_mday - nd;
				}
			}
			break;

		case 'm':
		case 'M':
			if (WMseen)
				return (-1);
			WMseen++;
			s++;
			if (tolower(*s) == 'l') {
				tm.tm_mday = nd;
				s++;
				t = s;
			} else {
				ul = strtoul(s, &t, 10);
				if (ul < 1 || ul > 31)
					return (-1);

				if (ul > nd)
					return (-1);
				tm.tm_mday = ul;
			}
			break;

		default:
			return (-1);
			break;
		}

		if (*t == '\0' || isspace(*t))
			break;
		else
			s = t;
	}

	/*
	 * XXX mktime() has a broken API that will not easily permit time_t to
	 * be changed into an unsigned integer type....
	 */
	if ((*trim_at = mktime(&tm)) == (time_t) -1)
		return (-1);

	return 0;
}
