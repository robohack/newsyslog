/*
 *	newsyslog - roll over selected logs at the appropriate time,
 *		keeping the a specified number of backup files around.
 */

/*
 * This file contains changes from the Open Software Foundation.
 */

/*
 * This file contains changes from Greg A. Woods; Planix, Inc.
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
	"@(#)newsyslog:$Name:  $:$Id: newsyslog.c,v 1.16 1999/01/17 06:36:46 woods Exp $";
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include <config.h>
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

#define CE_COMPACT	01	/* Compact the achived log files */
#define CE_BINARY	02	/* Logfile is in binary, don't add status messages */
#define CE_PLAIN0	04	/* Keep .0 file plain (needed for smail, httpd, etc.) */
#define NONE		-1

struct conf_entry {
	char           *log;		/* Name of the log */
	char           *pid_file;	/* PID file */
	int             uid;		/* Owner of log */
	int             gid;		/* Group of log */
	int             numlogs;	/* Number of logs to keep */
	int             size;		/* Size cutoff to trigger trimming the log */
	int             hours;		/* Hours between log trimming */
	int             permissions;	/* File permissions on the log */
	int             flags;		/* Flags (CE_*)  */
	struct conf_entry *next;	/* Linked list pointer */
};

char           *argv0 = PACKAGE;
char            package[] = PACKAGE;	/* the original dist name */
char            version[] = VERSION;

int             verbose = 0;	/* Print out what's going on */
int             needroot = 1;	/* Root privs are necessary */
int             noaction = 0;	/* Don't do anything, just show it */
int             domidnight = -1;/* ignore(-1) do(1) don't(0) do midnight run */
int             force = 0;	/* force all files to be trimmed */
char           *config_file = PATH_CONFIG;/* Configuration file to use */
char           *syslogd_pidfile = PATH_SYSLOGD_PIDFILE;/* syslogd's pid file */
time_t          timenow;
pid_t           syslogd_pid;	/* read in from /etc/syslog.pid */

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

char            hostname[MAXHOSTNAMELEN + 1];	/* hostname */
char           *daytime;		/* timenow in human readable form */

#if (!defined(OSF) && !defined(BSD)) || ((BSD + 0) < 199103)	/* XXX HACK!!! */
extern char            *strdup __P((const char *));
#endif

int                     main __P((int, char **)); /* XXX HACK for WARNS=1 */

static struct conf_entry *parse_file __P((void));
static char            *strsob __P((char *));
static char            *strson __P((char *));
static char            *missing_field __P((char *, char *));
static void             do_entry __P((struct conf_entry *));
static void             parse_options __P((int, char **));
static void             usage __P((void));
static void             do_trim __P((struct conf_entry *));
static int              note_trim __P((char *));
static void             compress_log __P((char *));
static int              check_file_size __P((char *));
static int              check_old_log_age __P((struct conf_entry *));
static time_t           read_first_timestamp __P((char *));
static pid_t            get_pid __P((char *));

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
		fprintf(stderr, "%s: you do not have root privileges.\n", argv0);
		exit(1);
	}
	p = q = parse_file();

	syslogd_pid = get_pid(syslogd_pidfile);

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
	int             size;
	int             modtime;

	if (verbose) {
		printf("%s <#%d,%s%s%s>: ", ent->log, ent->numlogs,
		       (ent->flags & CE_COMPACT) ? "Z" : "",
		       (ent->flags & CE_BINARY) ? "b" : "",
		       (ent->flags & CE_PLAIN0) ? "0" : "");
	}
	size = check_file_size(ent->log);
	if (size < 0) {
		if (verbose)
			printf("does not exist ");
	} else if (size == 0) {
		if (verbose)
			printf("is empty ");
	}
	if (size > 0) {
		if (verbose && (ent->size > 0))
			printf("size (Kb): %d [allow %d] ", size, ent->size);
		if ((ent->size > 0) && (size >= ent->size))
			we_trim_it = 1;
		modtime = check_old_log_age(ent);
		if (verbose && (ent->hours > 0))
			printf(" age (hr): %d [allow %d] ", modtime, ent->hours);
		assert(domidnight == -1 || domidnight == 1 || domidnight == 0);
		if (((ent->hours > 0) && ((modtime >= ent->hours) || (modtime < 0))))
			we_trim_it = 1;
	}
	if (domidnight == -1 || (domidnight == 1 && (ent->hours % 24) == 0)) {
		if (verbose && domidnight == 1 && we_trim_it)
			printf("(daily) ");
	} else if (domidnight == 0 && (ent->hours % 24) == 0) {
		we_trim_it = 0;
		if (verbose && !force)
			printf("not doing daily trim ");
	}
	if (force)
		we_trim_it = 1;
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
	char           *p;

	timenow = time((time_t *) 0);
	daytime = ctime(&timenow) + 4;
	daytime[15] = '\0';

	/* Let's get our hostname */
	(void) gethostname(hostname, sizeof(hostname));

	/* Truncate domain */
	if ((p = strchr(hostname, '.'))) {
		*p = '\0';
	}
	optind = 1;		/* Start options parsing */
	while ((c = getopt(argc, argv, "FMVf:mnp:rt:v")) != -1)
		switch (c) {
		case 'F':
			force = 1;
			break;
		case 'M':
			domidnight = 0;
			break;
		case 'V':
			printf("%s: version %s-%s.\n", argv0, package, version);
			exit(0);
			/* NOTREACHED */
		case 'm':
			domidnight = 1;
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
		case 'f':
			config_file = optarg;
			break;
		default:
			usage();
		}
}

static void 
usage()
{
	fprintf(stderr, "Usage: %s [-V] [-M|-m] [-FInrv] [-f config-file] [-p syslogd-pidfile]\n", argv0);
	exit(1);
}

/* Parse a configuration file and return a linked list of all the logs
 * to process
 */
static struct conf_entry *
parse_file()
{
	FILE           *fp;
	char            line[BUFSIZ], *parse, *q;
	char           *errline, *group;
	struct conf_entry *first = NULL;
	struct conf_entry *working = NULL;
	struct passwd  *pass;
	struct group   *grp;
	int             eol;

	if (strcmp(config_file, "-") == 0) {
		fp = stdin;
		config_file = "STDIN";
	} else
		fp = fopen(config_file, "r");
	if (!fp) {
		fprintf(stderr, "%s: could not open config file %s: %s.\n", argv0, config_file, strerror(errno));
		exit(1);
	}
	while (fgets(line, BUFSIZ, fp)) {
		struct conf_entry *tmpentry;

		if ((line[0] == '\n') || (line[0] == '#'))
			continue;
		errline = strdup(line);
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

		q = parse = missing_field(strsob(line), errline);
		parse = strson(line);
		if (!*parse) {
			fprintf(stderr, "%s: malformed line (missing fields):\n'%s'\n", argv0, errline);
			exit(1);
		}
		*parse = '\0';
		working->log = strdup(q);

		q = parse = missing_field(strsob(++parse), errline);
		parse = strson(parse);
		if (!*parse) {
			fprintf(stderr, "%s: malformed line (missing fields):\n'%s'\n", argv0, errline);
			exit(1);
		}
		*parse = '\0';
		if ((group = strchr(q, '.')) != NULL) {
			*group++ = '\0';
			if (*q) {
				if (!(isdigit(*q))) {
					if ((pass = getpwnam(q)) == NULL) {
						fprintf(stderr, "%s: error in config file; unknown user:\n'%s'\n",
							argv0, errline);
						exit(1);
					}
					working->uid = pass->pw_uid;
				} else
					working->uid = atoi(q);
			} else
				working->uid = NONE;

			q = group;
			if (*q) {
				if (!(isdigit(*q))) {
					if ((grp = getgrnam(q)) == NULL) {
						fprintf(stderr, "%s: error in config file; unknown group:\n'%s'\n",
							argv0, errline);
						exit(1);
					}
					working->gid = grp->gr_gid;
				} else
					working->gid = atoi(q);
			} else
				working->gid = NONE;

			q = parse = missing_field(strsob(++parse), errline);
			parse = strson(parse);
			if (!*parse) {
				fprintf(stderr, "%s: malformed line (missing fields):\n'%s'\n", argv0, errline);
				exit(1);
			}
			*parse = '\0';
		} else
			working->uid = working->gid = NONE;

		if (!sscanf(q, "%o", &working->permissions)) {
			fprintf(stderr, "%s: error in config file; bad permissions:\n'%s'\n",
				argv0, errline);
			exit(1);
		}

		q = parse = missing_field(strsob(++parse), errline);
		parse = strson(parse);
		if (!*parse) {
			fprintf(stderr, "%s: malformed line (missing fields):\n'%s'\n", argv0, errline);
			exit(1);
		}
		*parse = '\0';
		if (!sscanf(q, "%d", &working->numlogs)) {
			fprintf(stderr, "%s: error in config file; bad number:\n'%s'\n", argv0, errline);
			exit(1);
		}

		q = parse = missing_field(strsob(++parse), errline);
		parse = strson(parse);
		if (!*parse) {
			fprintf(stderr, "%s: malformed line (missing fields):\n'%s'\n", argv0, errline);
			exit(1);
		}
		*parse = '\0';
		if (isdigit(*q))
			working->size = atoi(q);
		else
			working->size = -1;

		q = parse = missing_field(strsob(++parse), errline);
		parse = strson(parse);
		eol = !*parse;
		*parse = '\0';
		if (isdigit(*q))
			working->hours = atoi(q);
		else
			working->hours = -1;

		if (eol)
			q = NULL;
		else {
			q = parse = strsob(++parse);	/* Optional field */
			parse = strson(parse);
			if (!*parse)
				eol = 1;
			*parse = '\0';
		}

		working->flags = 0;
		while (q && *q && !isspace(*q)) {
			if ((*q == 'Z') || (*q == 'z'))
				working->flags |= CE_COMPACT;
			else if ((*q == 'B') || (*q == 'b'))
				working->flags |= CE_BINARY;
			else if (*q == '0')
				working->flags |= CE_PLAIN0;
			else if (*q != '-') {
				fprintf(stderr, "%s: illegal flag in config file -- %c.\n", argv0, *q);
				exit(1);
			}
			q++;
		}

		if (eol)
			q = NULL;
		else {
			q = parse = strsob(++parse);	/* Optional field */
			*(parse = strson(parse)) = '\0';
		}

		working->pid_file = NULL;
		if (q && *q) {
			if (*q == '/')
				working->pid_file = strdup(q);
			else {
				fprintf(stderr, "%s: illegal pid file in config file: %s.\n", argv0, q);
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
missing_field(p, errline)
	char           *p;		/* parser token */
	char           *errline;	/* error message */
{
	if (!p || !*p) {
		fprintf(stderr, "%s: missing field in config file:\n'%s'\n", argv0, errline);
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
#define MAX_PERCENTD 10	/* XXX maximum number of ascii digits in an integer */
	char            file1[PATH_MAX - sizeof(COMPRESS_POSTFIX) - MAX_PERCENTD - 1];
	char            file2[PATH_MAX - sizeof(COMPRESS_POSTFIX) - MAX_PERCENTD - 1];
	char            zfile1[PATH_MAX];
	char            zfile2[PATH_MAX];
	int             notified;
	int             need_notification;
        int             fd;
	int             o_numlogs;
	struct stat     st;
	pid_t           pid;

	/*
	 * Most implementations of chown(2) "do the right thing" with -1, but
	 * some don't (eg. AIX-3.1), so we'll just play it safe and always set
	 * them correctly.
	 */
	if (ent->uid == -1)
		ent->uid = geteuid();
	if (ent->gid == -1)
		ent->gid = getegid();

	/* Remove oldest log */
	if (snprintf(file1, sizeof(file1), "%s.%d", ent->log, ent->numlogs) >= sizeof(file1)) {
		fprintf(stderr, "%s: filename too long: %s.\n", argv0, ent->log);
		return;
	}
	(void) strcpy(zfile1, file1);
	(void) strcat(zfile1, COMPRESS_POSTFIX);

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
		if (lstat(file1, &st) < 0) {
			(void) strcat(zfile1, COMPRESS_POSTFIX);
			(void) strcat(zfile2, COMPRESS_POSTFIX);
			if (lstat(zfile1, &st) < 0)
				continue; /* not this many aged files yet... */
		}
		if (noaction) {
			printf("mv %s %s\n", zfile1, zfile2);
			printf("chmod %o %s\n", ent->permissions, zfile2);
			printf("chown %d.%d %s\n",
			       ent->uid, ent->gid, zfile2);
		} else {
			(void) rename(zfile1, zfile2); /* XXX error check (non-fatal?) */
			(void) chmod(zfile2, ent->permissions); /* XXX error check (non-fatal?) */
			(void) chown(zfile2, ent->uid, ent->gid); /* XXX error check (non-fatal?) */
		}
	}
	if (!noaction && !(ent->flags & CE_BINARY)) {
		if (note_trim(ent->log))
			fprintf(stderr, "%s: can't add final status message to log: %s: %s.\n", argv0, ent->log, strerror(errno));
	}
	if (lstat(ent->log, &st) < 0)
		fprintf(stderr, "%s: can't stat file: %s: %s.\n", argv0, ent->log, strerror(errno));
	else if (st.st_size > 0) {
		/*
		 * are we keeping any aged files at all?
		 */
		if (o_numlogs != 0) {
			if (noaction)
				printf("rm %s\n", ent->log);
			else
				if (unlink(ent->log) < 0)
					fprintf(stderr, "%s: can't unlink file: %s: %s.\n", argv0, ent->log, strerror(errno));
		} else {
			if (noaction)
				printf("mv %s %s\n", ent->log, file1);
			else {
				if (rename(ent->log, file1) < 0)
					fprintf(stderr, "%s: can't rename file: %s to %s: %s.\n", argv0, ent->log, file1, strerror(errno));
			}
		}
		if (noaction) {
			printf("touch %s\n", ent->log);
			printf("chown %d.%d %s\n", ent->uid, ent->gid, ent->log);
		} else {
			fd = creat(ent->log, ent->permissions);
			if (fd < 0)
				fprintf(stderr, "%s: can't create new log: %s: %s.\n", argv0, ent->log, strerror(errno));
			else {
				if (fchown(fd, ent->uid, ent->gid))
					fprintf(stderr, "%s: can't chown new log file: %s: %s.\n", argv0, ent->log, strerror(errno));
				if (close(fd) < 0)
					fprintf(stderr, "%s: failed to close new log file: %s: %s.\n", argv0, ent->log, strerror(errno));
			}
			if (!(ent->flags & CE_BINARY)) {
#ifdef LOG_TURNOVER_IN_NEW_FILE_TOO
				if (note_trim(ent->log))
					fprintf(stderr, "%s: can't add status message to log: %s: %s.\n", argv0, ent->log, strerror(errno));
#endif
			}
		}
	}
	if (noaction)
		printf("chmod %o %s\n", ent->permissions, ent->log);
	else {
		if (chmod(ent->log, ent->permissions) < 0)
			fprintf(stderr, "%s: can't chmod log file %s: %s.\n", argv0, ent->log, strerror(errno));
	}
	pid = 0;
	need_notification = 0;
	notified = 0;
	if (ent->pid_file) {
		if (strcmp(_PATH_DEVNULL, ent->pid_file) != 0) {
			need_notification = 1;
			pid = get_pid(ent->pid_file);
		}
	} else if (!(ent->flags & CE_BINARY)) {
		/* XXX is it wrong to assume that binaries without an
		 * explicitly specified pid file are not written to by syslogd?
		 * The worst thing likely to happen is someone will set the
		 * CE_BINARY flag on a syslogd log file and syslog might not
		 * get notified right away about this file being trimmed.  Only
		 * if all syslogd files are marked binary will they never be
		 * properly re-opened by syslogd after being trimmed.
		 */
		need_notification = 1;
		pid = syslogd_pid;
	}
	if (pid) {
		if (noaction) {
			notified = 1;
			printf("kill -HUP %d\n", (int) pid);
		} else if (kill(pid, SIGHUP))
			fprintf(stderr, "%s: can't notify daemon, pid %d: %s\n.", argv0, (int) pid, strerror(errno));
		else {
			notified = 1;
			if (verbose)
				printf("daemon with pid %d notified\n", (int) pid);
		}
	}
	if ((ent->flags & CE_COMPACT)) {
		sprintf(zfile1, "%s.%s", ent->log, (ent->flags & CE_PLAIN0) ? "1" : "0");	/* sprintf() OK here */
		if (!(ent->flags & CE_PLAIN0) && need_notification && !notified) {
			fprintf(stderr, "%s: %s not compressed because daemon not notified.\n", argv0, ent->log);
		} else if (noaction) {
			if (notified) {
				if (verbose)
					printf("small pause to allow daemon to close log\n");
				puts("sleep 5");
			}
			printf("%s %s\n", PATH_COMPRESS, zfile1);
		} else {
			if (notified) {
				if (verbose)
					printf("small pause to allow daemon to close log\n");
				sleep(5);
			}
			compress_log(zfile1);
		}
	}
}

/*
 * Note the fact that the  were turned over
 */
static int 
note_trim(log)
	char           *log;
{
	FILE           *fp;

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
	}
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

	if (!(tmp = malloc(strlen(file) + sizeof(".0") + sizeof(COMPRESS_POSTFIX)))) {
		perror(argv0);
		return (-1);
	}
	(void) strcpy(tmp, file);
	if (stat(strcat(tmp, ".0"), &sb) < 0) {
		if (stat(strcat(tmp, COMPRESS_POSTFIX), &sb) < 0) {
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
		else if (strptime(line, "[%Y/%m/%d:%T]", &tms)) /* ISO (httpd common log) */
			;
		else if (strptime(line, "%c", &tms)) /* ctime */
			;
		else if (strptime(line, "[%c]", &tms)) /* httpd */
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
get_pid(pid_file)
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
