#
#	Makefile.am - input for automake
#
# Copyright (C) 1997 Planix, Inc. - All rights reserved.
#
#ident	"@(#)newsyslog:$Name:  $:$Id: Makefile.am,v 1.9 1998/03/16 03:03:17 woods Exp $"

AUTOMAKE_OPTIONS = gnu no-dependencies cvs cvs-testbuild dist-tarZ
MAINT_CHARSET = latin1

bin_PROGRAMS = newsyslog

man_MANS = newsyslog.conf.5 newsyslog.8

catman_MANS = newsyslog.conf.cat5 newsyslog.cat8
soelim_MANS = newsyslog.conf.5so newsyslog.8so

# things that exist in the repository
DIST_OTHER = README.developers ToDo

# things that must be made during 'make dist'
DIST_MAKE = $(soelim_MANS) $(catman_MANS) Makefile.BSD-dist

EXTRA_DIST = $(DIST_OTHER) $(DIST_MAKE)

PATH_CONFIG = @path_config@

PATH_SYSLOG_PIDFILE = @syslog_pidfile@

##
##	local extensions to automake
##

NROFF = nroff
SOELIM = soelim

SUFFIXES = .5 .8 .5so .8so .cat8 .cat5

.8.cat8 .5.cat5:
	@rm -f $@
	$(NROFF) -mandoc $(srcdir)/$< > $@ || (rm -f $@; exit 1)

.5so.5 .8so.8:
	@rm -f $@
	$(SOELIM) $< > $@ || (rm -f $@; exit 1)

#
# hard-coded dependencies
#

$(PROGRAMS): Makefile

$(man_MANS): config.status

newsyslog.conf.5: newsyslog.conf newsyslog.conf.5so

# (no .so's in here yet....)
newsyslog.8: newsyslog.8so

Makefile.BSD-dist: Makefile.BSD
	@rm -f $@
	cp $? $@

newsyslog.o: config.h
version.o: config.h
