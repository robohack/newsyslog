#ident	"@(#)newsyslog:$Name:  $:$Id: acconfig.h,v 1.4 1999/08/25 00:59:20 woods Exp $";

/*
 * Copyright (C) 1997 Planix, Inc. - All rights reserved.
 */

@TOP@

/* The name of the package. */
#define PACKAGE			"newsyslog"

/* The package release identifier. */
#define VERSION			"Pre-Release"

/* Configuration file */
#define PATH_CONFIG		"/etc/newsyslog.conf"

/* Default pid file */
#define PATH_SYSLOGD_PIDFILE	"/etc/syslog.pid"

/* File compression program */
#define PATH_COMPRESS		"/usr/ucb/compress"

/* Suffix string */
#define COMPRESS_SUFFIX		".Z"
