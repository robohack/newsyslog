#!/bin/sh
#
#	newsyslog2netbsd - upgrade from latest newsyslog release
#
# This file Copyright (C) by Planix, Inc.
# - see COPYING for details.
#
# NOTE: needs a fully POSIX /bin/sh to run properly....
#
#ident	"@(#)newsyslog:newsyslog2netbsd.sh:$Format:%D:%ci:%cN:%h$"

# The following variables can be adjusted as necessary
#
PREFIX="/usr"
BINDIR=${PREFIX}/bin
SYSCONFDIR="/etc"
LOCALSTATEDIR="/var"
NEWSYSLOG_CONF=${SYSCONFDIR}/newsyslog.conf
SYSLOGD_PID=${LOCALSTATEDIR}/run/syslogd.pid

#
# NOTHING user adjustable below this line!
#

IMPORTDIR="import.d"
BUILDDIR="nbsd-import"

version=$(git describe --tags)
releasetag=$(echo $version | sed -e 's/\./_/g')

set -e

autoreconf -f -i

mkdir ${BUILDDIR}
cd ${BUILDDIR}

cpsed ()
{
	if [ ! -r $1 ] ; then
		echo "$1: no such file!" 1>&2
		exit 1
	fi
	# assume that there will only be one #ident line....
	sed -e '/^#ident/i\
#ident	"@(#)$\Name$:$\NetBSD$"\
' \
	    -e '/^\.\\"ident/i\
.\\"ident	"@(#)$\Name$:$\NetBSD$"\
' \
	    -e '/^\.\\"#ident/i\
.\\"#ident	"@(#)$\Name$:$\NetBSD$"\
' $1 > $2
}

rm -rf ${IMPORTDIR}
mkdir ${IMPORTDIR}

if [ -f Makefile ] ; then
	make distclean
fi
AWK=awk ../configure --prefix=${PREFIX} --bindir=${BINDIR} --sysconfdir=${SYSCONFDIR} --localstatedir=${LOCALSTATEDIR} --with-newsyslog-conf=${NEWSYSLOG_CONF} --with-syslogd_pid=${SYSLOGD_PID} --with-gzip

# note the couple of renames....
cpsed ../AUTHORS ${IMPORTDIR}/AUTHORS
cpsed ../COPYING ${IMPORTDIR}/COPYING
cpsed Makefile.BSD ${IMPORTDIR}/Makefile
cpsed ../NEWS ${IMPORTDIR}/NEWS
cpsed ../README ${IMPORTDIR}/README
cpsed ../ToDo ${IMPORTDIR}/ToDo
cpsed ../VERSION ${IMPORTDIR}/VERSION
cpsed newsyslog.8so ${IMPORTDIR}/newsyslog.8
cpsed newsyslog.conf.5so ${IMPORTDIR}/newsyslog.conf.5so
cpsed ../newsyslog.c ${IMPORTDIR}/newsyslog.c
cpsed ../newsyslog.h ${IMPORTDIR}/newsyslog.h
cpsed ../sig2str.c ${IMPORTDIR}/sig2str.c
cpsed ../str2sig.c ${IMPORTDIR}/str2sig.c
cpsed ../newsyslog.conf ${IMPORTDIR}/newsyslog.conf
cpsed ../newsyslog2netbsd.sh ${IMPORTDIR}/newsyslog2netbsd.sh

# tell them what to do....
echo "# Now do this:"
echo "cd ${BUILDDIR}/${IMPORTDIR} && cp * /usr/src/usr.bin/newsyslog"
# XXX if NetBSD is/was still using CVS:
#echo "cd ${BUILDDIR}/${IMPORTDIR} ; cvs import -m 'Import of Planix newsyslog version $version' src/usr.bin/newsyslog PLANIX $releasetag"

exit 0
