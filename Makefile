# $Id: Makefile,v 1.31 2011/04/21 08:50:44 jmaggard Exp $
# DLNAProxy project
# (c) 2011 Lukas Prettenthaler
# for use with GNU Make
# To install use :
# $ DESTDIR=/dummyinstalldir make install
# or :
# $ INSTALLPREFIX=/usr/local make install
# or :
# $ make install
#
#CFLAGS = -Wall -O -D_GNU_SOURCE -g -DDEBUG
#CFLAGS = -Wall -g -Os -D_GNU_SOURCE
CFLAGS = -Wall -g -O3 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
#STATIC_LINKING: CFLAGS += -DSTATIC
#STATIC_LINKING: LDFLAGS = -static
CC = gcc
RM = rm -f
INSTALL = install

INSTALLPREFIX ?= $(DESTDIR)/usr
SBININSTALLDIR = $(INSTALLPREFIX)/sbin
ETCINSTALLDIR = $(DESTDIR)/etc

BASEOBJS = dlnaproxy.o upnpdescgen.o utils.o \
           upnpreplyparse.o minixml.o \
           getifaddr.o daemonize.o upnpglobalvars.o \
           options.o minissdp.o uuid.o upnpevents.o \
           tcptunnel.o log.o

ALLOBJS = $(BASEOBJS) $(LNXOBJS)

#LIBS = -lpthread
#STATIC_LINKING: LIBS = -lm -lpthread -lz

EXECUTABLES = dlnaproxy

.PHONY:	all clean distclean install depend

all:	$(EXECUTABLES)

clean:
	$(RM) $(ALLOBJS)
	$(RM) $(EXECUTABLES)

distclean: clean

install:	dlnaproxy
	$(INSTALL) -d $(SBININSTALLDIR)
	$(INSTALL) dlnaproxy $(SBININSTALLDIR)
	$(INSTALL) -d $(ETCINSTALLDIR)
	$(INSTALL) --mode=0644 dlnaproxy.conf $(ETCINSTALLDIR)

dlnaproxy:	$(BASEOBJS) $(LNXOBJS) $(LIBS)
	@echo Linking $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(BASEOBJS) $(LNXOBJS) $(LIBS)

depend:	makedepend -f$(MAKEFILE_LIST) -Y \
	$(ALLOBJS:.o=.c) 2>/dev/null

# DO NOT DELETE

dlnaproxy.o: config.h upnpglobalvars.h dlnaproxytypes.h tcptunnel.h
dlnaproxy.o: upnpdescgen.h getifaddr.h
dlnaproxy.o: options.h minissdp.h daemonize.h upnpevents.h log.h
upnpdescgen.o: config.h upnpdescgen.h upnpglobalvars.h
upnpdescgen.o: dlnaproxytypes.h upnpdescstrings.h log.h
upnpreplyparse.o: upnpreplyparse.h minixml.h log.h
minixml.o: minixml.h
getifaddr.o: getifaddr.h log.h
daemonize.o: daemonize.h config.h log.h
upnpglobalvars.o: config.h upnpglobalvars.h
upnpglobalvars.o: dlnaproxytypes.h
options.o: options.h config.h upnpglobalvars.h
options.o: dlnaproxytypes.h
minissdp.o: config.h upnpdescstrings.h
minissdp.o: upnpglobalvars.h dlnaproxytypes.h minissdp.h log.h
upnpevents.o: config.h upnpevents.h upnpglobalvars.h
upnpevents.o: dlnaproxytypes.h upnpdescgen.h log.h uuid.h
uuid.o: uuid.h
upnpdescgen.o: config.h upnpdescgen.h upnpglobalvars.h
upnpdescgen.o: dlnaproxytypes.h upnpdescstrings.h
utils.o: utils.h
tcptunnel.o: log.h
log.o: log.h

.SUFFIXES: .c .o

.c.o:
	@echo Compiling $*.c
	@$(CC) $(CFLAGS) -o $@ -c $< && exit 0;\
		echo "The following command failed:" 1>&2;\
		echo "$(CC) $(CFLAGS) -o $@ -c $<";\
		$(CC) $(CFLAGS) -o $@ -c $< &>/dev/null
