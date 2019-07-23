/*     2019-07-23, by nopius@nopius.com
       This program can attach to a running process and dump virtual memory regions into hex files
       memory region should be readble 'has an r' flag
  
       BUILD:
       make

       INSTALL:
       make install

       UNINSTALL:
       make uninstall
  
       USE (man process_vm_readv for details):
       pmap-dump -h

       [pgrep process_name]
       [pmap -x <PID> - look at the address (#1) and KBytes (#2) fields, append 0x for addr]
       run as root:
       pmap-dump [-f <PREFIX>] -p <PID> addr1 len1(in KB) [ addr2 len2 ... ]
       -p - pid
       -f - filename prefix
            arguments read from left to right, dump is performed while parsing, 
            you may use -p and -f multiple times
       
       EXAMPLE 1 (one pid, two segments):

  # pgrep -lf mysqld
20392 mysqld

  # pmap -x 20392
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

 #./pmap-dump -p 20392 0x0000000001dba000 904 0x00007f835a800000 45056
Pid: 20392 Prefix: pmap- Addr: 0x1dba000 Len: 925696
Pid: 20392 Prefix: pmap- Addr: 0x7f835a800000 Len: 46137344

  RESULT FILES:

  # ls -al
-rw-------.   1 root   root     925696 Jul 22 16:31 pmap-20392-0x0000000001dba000.hex
-rw-------.   1 root   root   46137344 Jul 22 16:31 pmap-20392-0x00007f835a800000.hex

       EXAMPLE 2 (two pids, one segment from each):

  # pgrep -lf 'X|mysqld'
5356  X
20392 mysqld
  # pmap -x 5356 
5356:   /usr/bin/X :0 -background none -noreset -audit 4 -verbose -auth /run/gdm/auth-for-gdm-FfPcyz/database -seat seat0 -nolisten tcp vt1
Address           Kbytes     RSS   Dirty Mode  Mapping
0000559981e78000    2284    1388       0 r-x-- Xorg
...
  # pmap -x 20392
pmap -x 20392
20392:   /usr/sbin/mysqld --daemonize --pid-file=/var/run/mysqld/mysqld.pid
Address           Kbytes     RSS   Dirty Mode  Mapping
0000000000400000   22648   12920       0 r-x-- mysqld
...
  # ./pmap-dump -p 5356 0x0000559981e78000 2284 -p 20392 0x0000000000400000 22648
Pid: 5356 Prefix: pmap- Addr: 0x559981e78000 Len: 2338816
Pid: 20392 Prefix: pmap- Addr: 0x400000 Len: 23191552

  RESULT FILES: 
-rw-------. 1 root   root   23191552 Jul 22 16:34 pmap-20392-0x0000000000400000.hex
-rw-------. 1 root   root    2338816 Jul 22 16:34 pmap-5356-0x0000559981e78000.hex

*/
/*     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/uio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


extern char *optarg;
extern int optind, opterr, optopt;
int getopt(int argc, char * const argv[], const char *optstring);

// BUFSIZE should be 4K - aligned
#define BUFSIZE 1024*1024
#define DEFAULT_PREFIX "pmap-"
#define DEFAULT_SUFFIX ".hex"
#define min(A,B) A<B?A:B

// Print usage 
void usage(char *argv[]){
  printf("Dump memory segments into files\n");
  printf("Usage: %s -h | -p <PID> [-f <PREFIX>] addr len [addr1 len1 ...] [-p <PID2> [-f <PREFIX2> ' addr2 len2 ...]] ...\n", argv[0]);
  printf("       -h          - get help\n");
  printf("       -p <PID>    - pid of the running process to dump memory\n");
  printf("       -f <PREFIX> - path and filename prefix to create memory dump files (default: \"%s\") \n",DEFAULT_PREFIX);
  printf("       addr        - address of the readable memory segment as shown in 'pmap -x' prefixed with 0x: 0xAABBCC.. \n");
  printf("       len         - length of specified segment in KBytes (base 1024), should divide by 4 (4K page size)\n");
  printf("       options are parsed from left to right, multiple -p and -f override prefious values\n");
}

/* -f "vmem_"    - file prefix where to save dump
 * -p XXX      - process pid
 * addr len [addr2 len2] ... - virtual memory address (hex) and length (decimal in Kbytes) to dump
 * OUTPUT: files named 'vmem-0x...hex" of size 'len' with content of this segment
*/
int
main(int argc, char *argv[])
{
  int opt;                      // parsed option
  int fd;                       // file descriptor
  char *prefix=NULL; 		// file prefix
  char *suffix=DEFAULT_SUFFIX;  // file suffix
  char *str_ptr=NULL;		// string ptr for conversions
  char *pid_ptr, *addr_ptr, *len_ptr; // pid, addr and len from argv
  char *file_name=NULL;	        // filename for current memory region
  pid_t pid=(pid_t)0;           // running process id 
  void *addr=NULL;              // void * made from addr_ptr
  size_t len=0;                 // number of bytes left to dumpe
  ssize_t nread;		// number of bytes read on current iteration
  ssize_t nwritten;		// number of bytes written to file on current iteration
  struct iovec local;		// buffers and addr structures to read memory
  struct iovec remote;		// ...
  char buffer[BUFSIZE];

  while ((opt = getopt(argc, argv, "-p:f:h")) != -1) {
    switch (opt) {
    case 'h':
      usage(argv);
      return 0;
    case 'f':
      prefix=optarg;
      break;
    case 'p':
      pid_ptr=optarg;
      pid=(pid_t)atol(optarg);
      break;
    case (char) 1:
      if (NULL == addr) {
        addr_ptr=argv[optind-1];
        addr=(void *)(strtoul(addr_ptr, &str_ptr, 16));
        len=(size_t)0;
      }
      else if (0 == len) {
        len_ptr=argv[optind-1];
        len=(size_t)strtoul(len_ptr, &str_ptr, 10)*1024;
        if ( len % 4096 ) {
            printf("Segment length should divide by 4\n");
            return 1;
        }
        if (!pid) {
          printf("Usage: %s -p pid [-f prefix] addr len_in_KB ...\n", argv[0]);
          return 2;
        }
        // dump memory here
        if (!prefix) prefix=DEFAULT_PREFIX;
        printf ("Pid: %d Prefix: %s Addr: 0x%llx Len: %ld\n", pid, prefix, addr, len);
        file_name=realloc(file_name,strlen(prefix)+strlen(pid_ptr)+1+strlen(addr_ptr)+strlen(suffix)+1);
        if (NULL==file_name) {
          perror("Realloc error");
          return 3;
        };
        sprintf(file_name,"%s%s-%s%s",prefix,pid_ptr,addr_ptr,suffix);
        fd=open(file_name,O_CREAT|O_APPEND|O_WRONLY, S_IRUSR|S_IWUSR);
        if (fd<0) {
          perror("File open error");
          return 4;
        };
        local.iov_base  = buffer;
        local.iov_len   = BUFSIZE;
        remote.iov_base = (void *) addr;
        remote.iov_len  = min(BUFSIZE,len);
        while (len>0) {
            nread = process_vm_readv(pid, &local, 1, &remote, 1, 0);
            if (nread>0) {
              nwritten=write(fd, local.iov_base, nread);
              if (nwritten<nread) {
                perror("Write error");
                return 5;
              }
              else {
                len=len-nread;
                remote.iov_base+=nread;
              }
            }
            else if (nread<0) {
              perror("Read error");
              return 6;
            }
            else {
              break;
            }
        }
        len=0;
        addr=NULL;
        close(fd);
      }
    }
  }
  if (pid==0) usage(argv);
  return 0;
}
