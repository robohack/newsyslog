#
#	Makefile.am - input for automake
#
# Copyright (C) 1997,1998,1999,2000,2001,2002 Planix, Inc.
# - All rights reserved.
#
#ident	"@(#)newsyslog:$Name:  $:$Id: Makefile.am,v 1.23 2002/02/09 21:46:31 woods Exp $"

AUTOMAKE_OPTIONS = gnu no-dependencies 1.4.5
MAINT_CHARSET = latin1

bin_PROGRAMS = newsyslog
newsyslog_SOURCES = newsyslog.c signame.c
newsyslog_LDADD = @LIBOBJS@

man_MANS = newsyslog.conf.5 newsyslog.8

soelim_MANS = newsyslog.conf.5so newsyslog.8so
catman_MANS = newsyslog.conf.cat5 newsyslog.cat8
catman_MANS_dist = newsyslog.conf.cat5-dist newsyslog.cat8-dist
psman_MANS = newsyslog.conf.5.ps newsyslog.8.ps
htmlman_MANS = newsyslog.conf.5.html newsyslog.8.html

DIST_MANS = $(catman_MANS_dist) $(psman_MANS) $(htmlman_MANS)

catmandir = @mandir@

COMPAT_HDRS = compat/sys/cdefs.h compat/machine/ansi.h

COMPAT_MANS = str2sig.3

# things that exist in the repository and need to be in the distribution
#
DIST_OTHER = README.developers ToDo VERSION newsyslog.conf \
		newsyslog2netbsd.sh .cvsignore \
		$(COMPAT_HDRS) $(COMPAT_MANS)

# things that must be made during 'make dist'
#
DIST_MAKE = $(DIST_MANS) Makefile.BSD-dist

EXTRA_DIST = $(DIST_OTHER) $(DIST_MAKE)

PATH_CONFIG = @path_config@

PATH_SYSLOGD_PIDFILE = @syslog_pidfile@

# Mention here files which are created using '--add-missing', as well
# as files created during 'make dist' and which can be re-created.
# (We don't mention Makefile.BSD-dist because it should be a virgin
# copy, never locally edited)
#
MAINTAINERCLEANFILES = install-sh missing mkinstalldirs INSTALL \
			$(catman_MANS_dist) $(psman_MANS) $(htmlman_MANS)

CLEANFILES = $(catman_MANS) $(man_MANS) $(psman_MANS) $(htmlman_MANS)

##
##	local extensions to automake
##

NROFF = nroff
TROFF = groff
ROFFDEV = -Tps
PIC = pic
TBL = tbl
SOELIM = soelim

SUFFIXES = .5 .8 .5so .8so .cat5 .cat8 .5.html .8.html .5.ps .8.ps

.5so.5 .8so.8:
	@rm -f $@
	($(SOELIM) -I $(srcdir) $< > $@) || (rm -f $@; exit 1)

.8.cat8 .5.cat5:
	@rm -f $@
	($(TBL) $< | $(NROFF) -mandoc > $@) || (rm -f $@; exit 1)

.8.8.ps:
	@rm -f $@
	($(TBL) $< | $(TROFF) -mdoc $(ROFFDEV) > $@) || (rm -f $@; exit 1)

.8.8.html:
	@rm -f $@
	($(TBL) $< | $(NROFF) -mdoc2html > $@) || (rm -f $@; exit 1)

.5.5.ps:
	@rm -f $@
	($(TBL) $< | $(TROFF) -mdoc $(ROFFDEV) > $@) || (rm -f $@; exit 1)

.5.5.html:
	@rm -f $@
	($(TBL) $< | $(NROFF) -mdoc2html > $@) || (rm -f $@; exit 1)

#
# hard-coded dependencies and extensions
#

$(PROGRAMS): Makefile Makefile.BSD

$(man_MANS): config.status

man catman: $(catman_MANS)

all-local: catman psman htmlman

psman: $(psman_MANS)

htmlman: $(htmlman_MANS)

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
