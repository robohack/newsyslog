#
#	Makefile.am - input for automake
#
# Copyright (C) 1997 Planix, Inc. - All rights reserved.
#
#ident	"@(#)fingerd:$Name:  $:$Id: Makefile.am,v 1.3 1998/03/05 05:27:52 woods Exp $"

AUTOMAKE_OPTIONS = gnu no-dependencies cvs cvs-testbuild dist-tarZ
MAINT_CHARSET = latin1

bin_PROGRAMS = newsyslog

man_MANS = newsyslog.conf.5 newsyslog.8

catman_MANS = newsyslog.conf.cat5 newsyslog.cat8

DIST_OTHER = README.developers $(catman_MANS)

PATH_CONFIG = @path_config@

PATH_SYSLOG_PIDFILE = @syslog_pidfile@

##
##	local extensions to automake
##

NROFF = nroff

SUFFIXES = .5 .8 .cat8 .cat5

.8.cat8 .5.cat5:
	$(NROFF) -mandoc $(srcdir)/$? > $@

#
# hard-coded dependencies
#

$(PROGRAMS): Makefile

$(man_MANS): config.status

newsyslog.o: config.h
version.o: config.h
