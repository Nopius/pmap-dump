SHELL = /bin/sh
CFLAGS = -g
CC = gcc
prefix  ?= /usr
sbindir ?= $(prefix)/sbin
INSTALL ?= install
INSTALL_PROGRAM ?= $(INSTALL)

all:    pmap-dump

pmap-dump: pmap-dump.c
	$(CC) $(CFLAGS) -o $@ $<

install: pmap-dump
	$(INSTALL_PROGRAM) -D $< $(DESTDIR)$(sbindir)/$<

uninstall:
	rm $(DESTDIR)$(sbindir)/pmap-dump

clean: 
	rm pmap-dump
