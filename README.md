# pmap-dump

Utility to dump running process memory by virtual addresses into files.

It's merely the same as:

> gdb -pid [pid]

>  dump memory pmem-pid.hex 0x00007f8b00000000 0x00007f8b00000000+65512

but without gdb and without stopping process (though possibly inconsistent)
  
## HOW TO BUILD

make

## HOW TO INSTALL

make install

## HOW TO UNINSTALL

make uninstall
  
## HOW TO RUN

This program works on Linux (tested on RHEL 7.x), man process_vm_readv for details.
- [pgrep process_name]
- [pmap -x <PID> - look at the address (#1) and KBytes (#2) fields, append 0x for addr]
run as root:
- pmap-dump [-f <PREFIX>] -p <PID> addr1 len1(in KB) [ addr2 len2 ... ]
       -p - pid
       -f - filename prefix
            arguments read from left to right, dump is performed while parsing, 
            you may use -p and -f multiple times
       
### EXAMPLE 1 (one pid, two segments)

>  pgrep -lf mysqld
20392 mysqld

> pmap -x 20392
pmap -x 20392
20392:   /usr/sbin/mysqld --daemonize --pid-file=/var/run/mysqld/mysqld.pid
Address           Kbytes     RSS   Dirty Mode  Mapping
0000000000400000   22648   12920       0 r-x-- mysqld
0000000001c1d000     952     584     436 r---- mysqld
0000000001d0b000     700     508     200 rw--- mysqld
0000000001dba000     904     752     752 rw---   [ anon ]
00007f835a7ff000       4       0       0 -----   [ anon ]
00007f835a800000   45056      96      96 rw---   [ anon ]
00007f835d7e1000       4       0       0 -----   [ anon ]
00007f835d7e2000    8192       8       8 rw---   [ anon ]
...

 > pmap-dump -p 20392 0x0000000001dba000 904 0x00007f835a800000 45056
Pid: 20392 Prefix: pmap- Addr: 0x1dba000 Len: 925696
Pid: 20392 Prefix: pmap- Addr: 0x7f835a800000 Len: 46137344

> ls -al
-rw-------.   1 root   root     925696 Jul 22 16:31 pmap-20392-0x0000000001dba000.hex
-rw-------.   1 root   root   46137344 Jul 22 16:31 pmap-20392-0x00007f835a800000.hex

### EXAMPLE 2 (two pids, one segment from each)

> pgrep -lf 'X|mysqld'
5356  X
20392 mysqld

> pmap -x 5356 
5356:   /usr/bin/X :0 -background none -noreset -audit 4 -verbose -auth /run/gdm/auth-for-gdm-FfPcyz/database -seat seat0 -nolisten tcp vt1
Address           Kbytes     RSS   Dirty Mode  Mapping
0000559981e78000    2284    1388       0 r-x-- Xorg
...

> pmap -x 20392
pmap -x 20392
20392:   /usr/sbin/mysqld --daemonize --pid-file=/var/run/mysqld/mysqld.pid
Address           Kbytes     RSS   Dirty Mode  Mapping
0000000000400000   22648   12920       0 r-x-- mysqld
...
> pmap-dump -p 5356 0x0000559981e78000 2284 -p 20392 0x0000000000400000 22648
Pid: 5356 Prefix: pmap- Addr: 0x559981e78000 Len: 2338816
Pid: 20392 Prefix: pmap- Addr: 0x400000 Len: 23191552

> ls -al
-rw-------. 1 root   root   23191552 Jul 22 16:34 pmap-20392-0x0000000000400000.hex
-rw-------. 1 root   root    2338816 Jul 22 16:34 pmap-5356-0x0000559981e78000.hex

### EXAMPLE 3 (one-liner to dump all readable segments of the process by name)
Beware, all shared segments are dumped into separate files without gaps, you can easily eat all disk!

> pgrep -f mysqld | while read pid; do ( sudo pmap -x $pid | awk -vPID=$pid "BEGIN{ printf(\"pmap-dump -p \" PID)};(\$5~/^r/){printf(\" 0x\" \$1 \" \" \$2)};END{printf(\"\\n\")}"); done | while read line; do sudo $line; done
