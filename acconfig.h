#ident	"@(#)newsyslog:$Name:  $:$Id: acconfig.h,v 1.7 2001/02/23 01:36:58 woods Exp $";

/*
 * Copyright (C) 1997 Planix, Inc. - All rights reserved.
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

@BOTTOM@

/*
 * End of this file
 */
