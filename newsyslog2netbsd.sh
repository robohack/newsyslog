#! /bin/sh
:
#
#	newsyslog2netbsd - upgrade from latest newsyslog release
#
# NOTE: needs a POSIX /bin/sh to run....
#
#ident	"@(#)newsyslog:$Name:  $:$Id: newsyslog2netbsd.sh,v 1.1 1999/01/17 06:44:53 woods Exp $"

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
' \
	    -e 's;@\path_config@;/etc/newsyslog.conf;' \
	    -e 's;@\syslogd_pidfile@;/var/run/syslog.pid;' $1 > $2
}

IMPORTDIR="import.d"

rm -rf ${IMPORTDIR}
mkdir ${IMPORTDIR}

if [ ! -r Makefile.BSD ] ; then
	./configure
fi

# note the renames....
cpsed Makefile.BSD ${IMPORTDIR}/Makefile
cpsed newsyslog.8so.in ${IMPORTDIR}/newsyslog.8
cpsed newsyslog.conf.5so.in ${IMPORTDIR}/newsyslog.conf.5so
cpsed newsyslog.c ${IMPORTDIR}/newsyslog.c
cpsed newsyslog.conf ${IMPORTDIR}/newsyslog.conf
cpsed newsyslog2netbsd.sh ${IMPORTDIR}/newsyslog2netbsd.sh

# tell them what to do....
echo "cd ${IMPORTDIR} ; cvs import -m 'Import of Planix newsyslog version $version' src/usr.bin/newsyslog PLANIX $releasetag"

exit 0
