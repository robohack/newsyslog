#
#	Makefile.am - input for automake
#
# Copyright (C) 1997,1998,1999,2000,2001 Planix, Inc. - All rights reserved.
#
#ident	"@(#)newsyslog:$Name:  $:$Id: Makefile.am,v 1.19 2002/01/04 00:43:54 woods Exp $"

AUTOMAKE_OPTIONS = gnu no-dependencies cvs cvs-testbuild dist-tarZ
MAINT_CHARSET = latin1

bin_PROGRAMS = newsyslog
newsyslog_SOURCES = newsyslog.c signame.c
newsyslog_LDADD = @LIBOBJS@

man_MANS = newsyslog.conf.5 newsyslog.8

soelim_MANS = newsyslog.conf.5so newsyslog.8so
catman_MANS = newsyslog.conf.cat5 newsyslog.cat8
catman_MANS_dist = newsyslog.conf.cat5-dist newsyslog.cat8-dist

catmandir = @mandir@

# things that exist in the repository
DIST_OTHER = README.developers ToDo VERSION

# things that must be made during 'make dist'
DIST_MAKE = $(catman_MANS_dist) Makefile.BSD-dist

EXTRA_DIST = $(DIST_OTHER) $(DIST_MAKE)

PATH_CONFIG = @path_config@

PATH_SYSLOGD_PIDFILE = @syslog_pidfile@

# Mention here files which are created using '--add-missing'
MAINTAINERCLEANFILES = install-sh missing mkinstalldirs INSTALL README.developers $(catman_MANS_dist)

CLEANFILES = $(catman_MANS) $(man_MANS)

##
##	local extensions to automake
##

NROFF = nroff
TROFF = troff
ROFFDEV = -Tps
PIC = pic
TBL = tbl
SOELIM = soelim

SUFFIXES = .5 .8 .5so .8so .cat8 .cat5 .ps .txt

.8.cat8 .5.cat5:
	@rm -f $@
	$(NROFF) -mandoc $(srcdir)/$< > $@ || (rm -f $@; exit 1)

.5so.5 .8so.8:
	@rm -f $@
	$(SOELIM) $(srcdir)/$< > $@ || (rm -f $@; exit 1)

.8.txt:
	$(SOELIM) $(srcdir)/$? | $(TBL) | $(NROFF) -mdoc > $@

.8.ps:
	$(SOELIM) $(srcdir)/$? | $(TBL) | $(TROFF) -mdoc $(ROFFDEV) > $@

.5.txt:
	$(SOELIM) $(srcdir)/$? | $(TBL) | $(NROFF) -mdoc > $@

.5.ps:
	$(SOELIM) $(srcdir)/$? | $(TBL) | $(TROFF) -mdoc $(ROFFDEV) > $@

#
# hard-coded dependencies and extensions
#

$(PROGRAMS): Makefile Makefile.BSD

$(man_MANS): config.status

man catman: $(catman_MANS)

all-local: catman

# install the cat-able manual pages too...
#
install-data-local: install-catman-local

# the normal manuals are installed by automake rules
#
install-catman-local:
	$(mkinstalldirs) $(catmandir)/cat5
	$(mkinstalldirs) $(catmandir)/cat8
	$(INSTALL_DATA) newsyslog.conf.cat5 $(catmandir)/cat5/newsyslog.conf.0
	$(INSTALL_DATA) newsyslog.cat8 $(catmandir)/cat8/newsyslog.0

# if you have no mandoc macros you might be able to use this rule, or
# something similar, to install the distributed versions of the
# cattable manual pages.  Make this before "make install"
#
copy-dist-catman:
	cp newsyslog.conf.cat5-dist newsyslog.conf.cat5
	cp newsyslog.cat8-dist newsyslog.cat8

newsyslog.conf.5: newsyslog.conf newsyslog.conf.5so

# (no .so's in here yet....)
newsyslog.8: newsyslog.8so

newsyslog.conf.cat5-dist: newsyslog.conf.cat5
	@rm -f $@
	cp $? $@

newsyslog.cat8-dist: newsyslog.cat8
	@rm -f $@
	cp $? $@

Makefile.BSD-dist: Makefile.BSD
	@rm -f $@
	cp $? $@

Makefile.BSD: Makefile.BSD.in config.status

newsyslog.o: config.h
version.o: config.h
