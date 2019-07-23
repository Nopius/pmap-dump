CFLAGS = -g
CC = gcc
prefix  ?= /usr
sbindir ?= /sbin
INSTALL_PROGRAM ?= install

all:	pmap-dump
	
pmap-dump: pmap-dump.c
	$(CC) $(CFLAGS) -o pmap-dump pmap-dump.c

install:
	$(INSTALL_PROGRAM) -D pmap-dump $(DESTDIR)$(prefix)$(sbindir)/pmap-dump
	
uninstall:
	rm $(DESTDIR)$(prefix)$(sbindir)/pmap-dump

clean: 
	rm pmap-dump
