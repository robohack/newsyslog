#ident	"@(#)newsyslog:$Name:  $:$Id: acconfig.h,v 1.9 2002/01/04 00:42:15 woods Exp $";

/*
 * Copyright (C) 1997,1998,1999,2000,2001 Planix, Inc. - All rights reserved.
 */

@TOP@

/* The name of the package. */
#define PACKAGE			"newsyslog"

/* The package release identifier. */
#define VERSION			"Pre-Release"

/* The package release major number. */
#define VERSION_MAJOR		0

/* The package release minor number. */
#define VERSION_MINOR		0

/* The package release patch number. */
#define VERSION_PATCH		0

/* The package release suffix (usually -Prelease in unreleased code). */
#define VERSION_SUFFIX		"unconfigured"

/* Optional package release comment (WARNING: keep as one word in acconfig.h!). */
#define VERSION_COMMENT		"Please_run_./configure!"

/* Configuration file */
#define PATH_CONFIG		"/etc/newsyslog.conf"

/* Default pid file */
#define PATH_SYSLOGD_PIDFILE	"/etc/syslog.pid"

/* File compression program */
#define PATH_COMPRESS		"/usr/ucb/compress"

/* Suffix string */
#define COMPRESS_SUFFIX		".Z"

/* Define if <signal.h> doesn't declare 'sys_signame' */
#undef SYS_SIGNAME_DECLARED

@BOTTOM@

/* Define if you have a sig2str() function declaration in <signal.h>.  */
#define HAVE_DECL_SIG2STR	0

/* Define if you have a str2sig() function declaration in <signal.h>.  */
#define HAVE_DECL_STR2SIG	0

/* Define if you have a strchr() function declaration in <string[s].h>.  */
#define HAVE_DECL_STRCHR	0

/* Define if you have a strdup() function declaration in <string[s].h>.  */
#define HAVE_DECL_STRDUP	0

/* Define if you have a strptime() function declaration in <time.h>.  */
#define HAVE_DECL_STRPTIME	0

/* Define if you have a strrchr() function declaration in <string[s].h>.  */
#define HAVE_DECL_STRRCHR	0

/* Define if you have a strtok() function declaration in <string[s].h>.  */
#define HAVE_DECL_STRTOK	0

/*
 * End of this file
 */
