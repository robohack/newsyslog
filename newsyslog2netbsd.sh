#! /bin/sh
:
#
#	newsyslog2netbsd - upgrade from latest newsyslog release
#
# This file Copyright (C) 1999 Planix, Inc. -- see COPYING for details.
#
# NOTE: needs a fully POSIX /bin/sh to run properly....
#
#ident	"@(#)newsyslog:$Name:  $:$Id: newsyslog2netbsd.sh,v 1.4 2000/12/01 19:49:32 woods Exp $"

# The following variables can be adjusted as necessary
#
PREFIX="/usr"
BINDIR=${PREFIX}/bin
SYSCONFDIR="/etc"
LOCALSTATEDIR="/var"
NEWSYSLOG_CONF=${SYSCONFDIR}/newsyslog.conf
SYSLOGD_PID=${LOCALSTATEDIR}/run/syslogd.pid

IMPORTDIR="import.d"

#
# NOTHING user adjustable below this line!
#

PWD=$(/bin/pwd)
version=$(basename $PWD | sed -e 's/newsyslog-//')
releasetag=$(basename $PWD | sed -e 's/\./_/g')

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

if [ ! -f Makefile.BSD ] ; then
	./configure --prefix=${PREFIX} --bindir=${BINDIR} --sysconfdir=${SYSCONFDIR} --localstatedir=${LOCALSTATEDIR} --with-newsyslog-conf=${NEWSYSLOG_CONF} --with-syslogd_pid=${SYSLOGD_PID} --with-gzip
fi

# note the couple of renames....
cpsed AUTHORS ${IMPORTDIR}/AUTHORS
cpsed COPYING ${IMPORTDIR}/COPYING
cpsed Makefile.BSD ${IMPORTDIR}/Makefile
cpsed NEWS ${IMPORTDIR}/NEWS
cpsed README ${IMPORTDIR}/README
cpsed ToDo ${IMPORTDIR}/ToDo
cpsed newsyslog.8so ${IMPORTDIR}/newsyslog.8
cpsed newsyslog.conf.5so ${IMPORTDIR}/newsyslog.conf.5so
cpsed newsyslog.c ${IMPORTDIR}/newsyslog.c
cpsed newsyslog.conf ${IMPORTDIR}/newsyslog.conf
cpsed newsyslog2netbsd.sh ${IMPORTDIR}/newsyslog2netbsd.sh

# tell them what to do....
echo "# Now do this:"
echo "cd ${IMPORTDIR} ; cvs import -m 'Import of Planix newsyslog version $version' src/usr.bin/newsyslog PLANIX $releasetag"

exit 0
