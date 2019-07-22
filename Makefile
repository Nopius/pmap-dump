all:	pmap-dump
	
pmap-dump:
	gcc -g -o pmap-dump pmap-dump.c

install:
	install --strip pmap-dump /usr/sbin/pmap-dump
	
uninstall:
	rm /usr/sbin/pmap-dump
