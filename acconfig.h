#ident	"@(#)newsyslog:$Name:  $:$Id: acconfig.h,v 1.3 1999/01/17 06:30:38 woods Exp $";

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
#define COMPRESS_POSTFIX	".Z"
