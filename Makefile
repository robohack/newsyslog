#	$Id: Makefile,v 1.3 1997/10/28 06:57:04 woods Exp $

SHELL=	/bin/sh

PROG=	newsyslog
SRCS=	newsyslog.c
OBJS=	newsyslog.o

DEFS=	-DCONF=\"/etc/newsyslog.conf\" \
	-DPIDFILE=\"/etc/syslog.pid\" \
	-DCOMPRESS_PATH=\"/usr/ucb/compress\" \
	-DCOMPRESS_PROG=\"compress\" \
	-DCOMPRESS_POSTFIX=\".Z\"

SDB=#	-g
OPTIM=	-O
CFLAGS=	${SDB} ${OPTIM} ${DEFS} ${INCDIROPTS}

LIBS=#	-lnsl

BINOWN=	root
MAN8EXT=8
MAN8=	newsyslog.${MAN8EXT}

${PROG}:	${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o $@ ${LIBS}

clean:
	-rm -f ${PROG} ${OBJS} core
