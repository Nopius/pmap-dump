#!/bin/bash
# $1 - process name, dump all segments to files
#!/bin/bash
# $1 - process name, dump all segments, one-liner
pgrep -f $1 | \
while read pid
do
  (
    sudo pmap -x $pid | \
    awk -vPID=$pid 'BEGIN{ printf("pmap-dump -p " PID)};($5~/^r/){printf(" 0x" $1 " " $2)};END{printf("\n")}'
  )
done | while read line; do sudo $line; done
