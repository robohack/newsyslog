#ident	"@(#)newsyslog:$Name:  $:$Id: acconfig.h,v 1.6 2001/02/23 01:30:01 woods Exp $";

/*
 * Copyright (C) 1997 Planix, Inc. - All rights reserved.
 */

@TOP@

/* The name of the package. */
#define PACKAGE			"newsyslog"

/* The package release identifier. */
#define VERSION			"Pre-Release"

#define VERSION_MAJOR		0
#define VERSION_MINOR		0
#define VERSION_PATCH		0
#define VERSION_SUFFIX		"unconfigured"
#define VERSION_COMMENT		"Please run ./configure!"

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
