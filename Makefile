#	$Id: Makefile,v 1.1 1997/09/26 21:07:07 woods Exp $

SHELL=	/bin/sh

PROG=	newsyslog
SRCS=	newsyslog.c
OBJS=	newsyslog.o

DEFS=	-DOSF\
	-DCONF=\"/etc/newsyslog.conf\" \
	-DPIDFILE=\"/etc/syslog.pid\" \
	-DCOMPRESS=\"/usr/bin/gzip\" \
	-DCOMPRESS_POSTFIX=\".gz\" \

CC=	gcc -pipe
CFLAGS=	-O2 ${DEFS}

LIBS=#	-lnsl


BINOWN=	root
MAN8=	newsyslog.0


${PROG}:	${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o $@ ${LIBS}


clean:
	-rm -f ${PROG} ${OBJS} core
