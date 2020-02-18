#
#	Makefile.am - input for automake
#
# Copyright (C) by Planix, Inc.
# - All rights reserved.
#
#ident	"@(#)newsyslog:Makefile.am:$Format:%D:%ci:%cN:%h$"

AUTOMAKE_OPTIONS = gnu no-dependencies 1.10
MAINT_CHARSET = latin1

bin_PROGRAMS = newsyslog
newsyslog_SOURCES = newsyslog.c newsyslog.h
newsyslog_LDADD = $(LIBOBJS)

man_MANS = newsyslog.conf.5 newsyslog.8

soelim_MANS = newsyslog.conf.5so newsyslog.8so
catman_MANS = newsyslog.conf.cat5 newsyslog.cat8
dist_catman_MANS = newsyslog.conf.cat5-dist newsyslog.cat8-dist
psman_MANS = newsyslog.conf.5.ps newsyslog.8.ps
dist_psman_MANS = newsyslog.conf.5.ps-dist newsyslog.8.ps-dist
htmlman_MANS = newsyslog.conf.5.html newsyslog.8.html
dist_htmlman_MANS = newsyslog.conf.5.html-dist newsyslog.8.html-dist

DIST_MANS = $(dist_catman_MANS) $(dist_psman_MANS) $(dist_htmlman_MANSt)

catmandir = @mandir@

COMPAT_HDRS = compat/sys/cdefs.h compat/machine/ansi.h

COMPAT_MANS = str2sig.3

# things that exist in the repository and need to be in the distribution
#
DIST_OTHER = README.developers ToDo VERSION newsyslog.conf \
		newsyslog2netbsd.sh .cvsignore \
		mksigname.sh siglist.in \
		$(COMPAT_HDRS) $(COMPAT_MANS)

# things that must be made during 'make dist'
#
DIST_MAKE = $(DIST_MANS) Makefile.BSD-dist

EXTRA_DIST = $(DIST_OTHER) $(DIST_MAKE)

PATH_CONFIG = @path_config@

PATH_SYSLOGD_PIDFILE = @syslog_pidfile@

# Mention here files which are created using 'auto*' tools and which
# can be re-created by re-running them again.
#
# NOTE: these files were removed automatically by past automake's.
# 
GNU_AUTO_MADE_FILES = Makefile.in \
			aclocal.m4 \
			configure \
			config.h.in

# Mention here files which are created using '--add-missing', as well
# as files created during 'make dist' and which can be re-created.
#
# (Don't list 'Makefile.BSD-dist' because it should always be a virgin
# copy, never locally edited, and never rebuilt unless the user does
# "gmake dist", and even if the user is making local changes(?))
#
MAINTAINERCLEANFILES = install-sh missing mkinstalldirs INSTALL \
			$(GNU_AUTO_MADE_FILES) \
			$(dist_catman_MANS) $(dist_psman_MANS) $(dist_htmlman_MANS)

# XXX automake still isn't quite current with autoconf it seems....
# configure makes these so we need to clean them on 'distclean'
# (autom4te.cache will be removed by maintainer-clean)
#
distclean-local:
	rm -rf autom4te.cache
	rm -f aclocal.m4~ config.h.in~

CLEANFILES = sys_signame.c $(catman_MANS) $(man_MANS) $(psman_MANS) $(htmlman_MANS)

##
##	local custom rules
##
sys_signame.c: mksigname.sh siglist.in Makefile
	@rm -f $@
	${SHELL} $(srcdir)/mksigname.sh $(srcdir)/siglist.in $@

##
##	local extensions to automake
##

NROFF = nroff
TROFF = groff
ROFFDEV = -Tps
PIC = pic
TBL = tbl
SOELIM = soelim

SUFFIXES = .5 .8 .5so .8so .cat5 .cat5-dist .cat8 .cat8-dist .5.html .8.html \
		.5.ps .8.ps .ps .ps-dist .html .html-dist .BSD .BSD-dist

# XXX ignore the stupid automake POSIX warnings about this!
#
.5so.5 .8so.8:
	@rm -f $@
	($(SOELIM) -I $(srcdir) $< > $@) || (rm -f $@; exit 1)

# XXX ignore the stupid automake POSIX warnings about this!
#
.8.cat8 .5.cat5:
	@rm -f $@
	($(TBL) $< | $(NROFF) -mdoc > $@) || (rm -f $@; exit 1)

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

.cat5.cat5-dist:
	@rm -f $@
	cp $? $@

.cat8.cat8-dist:
	@rm -f $@
	cp $? $@

.html.html-dist:
	@rm -f $@
	cp $? $@

.ps.ps-dist:
	@rm -f $@
	cp $? $@

.BSD.BSD-dist:
	@rm -f $@
	cp $? $@

#
# hard-coded dependencies and extensions
#

$(PROGRAMS): Makefile Makefile.BSD

$(man_MANS): config.status

all-local: catman psman htmlman

man catman: $(catman_MANS)

psman: $(psman_MANS)

htmlman: $(htmlman_MANS)

# we do distribute files which may not necessarily be rebuildable on
# all platforms, but we do want them to be built by default if
# possible....
#
# this as per the notes in the "Dist" node of the automake manual:
#
distcleancheck_listfiles = \
	find . -type f -exec sh -c 'test -f $(srcdir)/{} || echo {}' ';'

# install the cat-able manual pages too...
#
install-data-local: install-catman-local

# the unformatted manuals are installed by automake rules
#
install-catman-local:
	$(MKDIR_P) $(DESTDIR)$(catmandir)/cat5
	$(MKDIR_P) $(DESTDIR)$(catmandir)/cat8
	$(INSTALL_DATA) newsyslog.conf.cat5 $(DESTDIR)$(catmandir)/cat5/newsyslog.conf.0
	$(INSTALL_DATA) newsyslog.cat8 $(DESTDIR)$(catmandir)/cat8/newsyslog.0

# automake>=1.6 doesn't have separate uninstalls for "exec" and "data"
#
uninstall-local: uninstall-catman-local

uninstall-catman-local:
	rm -f $(DESTDIR)$(catmandir)/cat5/newsyslog.conf.0
	rm -f $(DESTDIR)$(catmandir)/cat8/newsyslog.0

# if you have no mandoc macros you might be able to use this rule, or
# something similar, to install the distributed versions of the
# cat-able manual pages.  Make this before "make install"
#
copy-dist-mans:
	for distman in $(DIST_MANS) ; do			\
		cp $$distman $$(basename $$distman -dist);	\
	done

# copy project page sub-files into place
#
# (expects to be run from within an un-packed distribution directory)
#
update-project-web-files: $(dist_htmlman_MANS)
	for htmlman in $(dist_htmlman_MANS) ; do 					\
		cp $$htmlman ~/public_html/projects/$$(basename $$htmlman -dist); 	\
	done
	cp -f NEWS ~/public_html/projects/newsyslog.NEWS.txt
	cp -f ToDo ~/public_html/projects/newsyslog.ToDo.txt
	@echo "NOTE: now make sure to run ~/public_html/publish.sh"

# miscellaneous dependencies follow....
#

newsyslog.conf.5: newsyslog.conf newsyslog.conf.5so

# (no .so's in here (at least not yet...))
newsyslog.8: newsyslog.8so

Makefile.BSD: Makefile.BSD.in config.status

newsyslog.o: config.h
version.o: config.h
