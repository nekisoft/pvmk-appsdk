#!/usr/bin/env bash
#toybox_build.sh
#builds Toybox utils for PVMK application SDK (need this stuff on Windows!)
#Bryan E. Topp <betopp@betopp.com> 2025

set -e

mkdir -p out/bin/$(uname -o)/$(uname -m)/

mkdir -p obj
cd obj
tar -xf ../src/toybox-0.8.12.tar.gz
cd toybox-0.8.12
cp ../../src/toybox.config ./.config

#Toybox scripts call out /bin/bash on their bangpath - use /usr/bin/env bash instead
find . -name "*.sh" | xargs -n1 sed -i -e 's/\/bin\/bash/\/usr\/bin\/env bash/g'

#Toybox always wants to build "taskset" and "ps" command...?
echo "" > toys/other/taskset.c
echo "" > toys/posix/ps.c

#Patch out xattr support in cp
patch toys/posix/cp.c <<EOF
123a124
> #if 0
147a149
> #endif
174a177
> #if 0
175a179,180
> #endif
> 
301c306
< 
---
> #if 0
302a308
> #endif
EOF

#Patch out xattr support in tar
patch toys/posix/tar.c <<EOF
392a393
> #if 0
420a422
> #endif
EOF

#Patch some calls that Windows is missing
patch scripts/config2help.c <<EOF
16c16
< #include <regex.h>
---
> //#include <regex.h>
18,20c18,20
< #include <termios.h>
< #include <poll.h>
< #include <sys/socket.h>
---
> //#include <termios.h>
> //#include <poll.h>
> //#include <sys/socket.h>
229a230,257
> char *our_strndup(const char *str, size_t len)
> {
>       if(len >= strlen(str))
>               return strdup(str);
>  
>       char *ret = malloc(len+1);
>       memcpy(ret, str, len);
>       ret[len] = 0;
>       return ret;
> }
>  
> ssize_t our_getline(char **line, size_t *len, FILE *fp)
> {
>       static char tempbuf[65536] = {0};
>       char *fgets_val = fgets(tempbuf, sizeof(tempbuf)-1, fp);
>       if(fgets_val == NULL)
>               return -1;
>  
>       if(strlen(tempbuf) >= *len || *line == NULL)
>       {
>               *line = realloc(*line, strlen(tempbuf)+1);
>               *len = strlen(tempbuf);
>       }
>  
>       strcpy(*line, tempbuf);
>       return strlen(tempbuf);
> }
>  
241c269
<     if (getline(&line, &len, fp) < 1) break;
---
>     if (our_getline(&line, &len, fp) < 1) break;
322c350
<     if (getline(&line, &len, fp) < 1) break;
---
>     if (our_getline(&line, &len, fp) < 1) break;
365c393
<         name = strndup(usage, len);
---
>         name = our_strndup(usage, len);
EOF

patch scripts/mkflags.c <<EOF
148a149,154
> static char *our_stpcpy(char *dst, const char *src)
> {
>       strcpy(dst, src);
>       return dst + strlen(src);
> }
>  
252c258
<     out = stpcpy(out, "#endif\\n\\n");
---
>     out = our_stpcpy(out, "#endif\\n\\n");
EOF

patch toys.h <<EOF
16c16
< #include <grp.h>
---
> //#include <grp.h>
20,22c20,22
< #include <paths.h>
< #include <pwd.h>
< #include <regex.h>
---
> //#include <paths.h>
> //#include <pwd.h>
> //#include <regex.h>
33,34c33,34
< #include <sys/mman.h>
< #include <sys/resource.h>
---
> //#include <sys/mman.h>
> //#include <sys/resource.h>
36c36
< #include <sys/statvfs.h>
---
> //#include <sys/statvfs.h>
38,43c38,43
< #include <sys/times.h>
< #include <sys/uio.h>
< #include <sys/utsname.h>
< #include <sys/wait.h>
< #include <syslog.h>
< #include <termios.h>
---
> //#include <sys/times.h>
> //#include <sys/uio.h>
> //#include <sys/utsname.h>
> //#include <sys/wait.h>
> //#include <syslog.h>
> //#include <termios.h>
50,57c50,57
< #include <arpa/inet.h>
< #include <netdb.h>
< #include <net/if.h>
< #include <netinet/in.h>
< #include <netinet/tcp.h>
< #include <poll.h>
< #include <sys/socket.h>
< #include <sys/un.h>
---
> //#include <arpa/inet.h>
> //#include <netdb.h>
> //#include <net/if.h>
> //#include <netinet/in.h>
> //#include <netinet/tcp.h>
> //#include <poll.h>
> //#include <sys/socket.h>
> //#include <sys/un.h>
61c61
< #include <langinfo.h>
---
> //#include <langinfo.h>
67,69c67,69
< #include <sys/ioctl.h>
< #include <sys/syscall.h>
< #include <sys/ttydefaults.h>
---
> //#include <sys/ioctl.h>
> //#include <sys/syscall.h>
> //#include <sys/ttydefaults.h>
EOF

patch lib/portability.h <<EOF
24c24
< #include <regex.h>
---
> //#include <regex.h>
35c35
< #include <fnmatch.h>
---
> //#include <fnmatch.h>
146c146
< #include <byteswap.h>
---
> //#include <byteswap.h>
166c166
< #include <sys/mount.h>
---
> //#include <sys/mount.h>
176c176
< #include <pty.h>
---
> //#include <pty.h>
216,217c216,217
< static inline long statfs_bsize(struct statfs *sf) { return sf->f_iosize; }
< static inline long statfs_frsize(struct statfs *sf) { return sf->f_bsize; }
---
> //static inline long statfs_bsize(struct statfs *sf) { return sf->f_iosize; }
> //static inline long statfs_frsize(struct statfs *sf) { return sf->f_bsize; }
219,220c219,220
< static inline long statfs_bsize(struct statfs *sf) { return sf->f_bsize; }
< static inline long statfs_frsize(struct statfs *sf) { return sf->f_frsize; }
---
> //static inline long statfs_bsize(struct statfs *sf) { return sf->f_bsize; }
> //static inline long statfs_frsize(struct statfs *sf) { return sf->f_frsize; }
361c361
< char *fs_type_name(struct statfs *statfs);
---
> //char *fs_type_name(struct statfs *statfs);
EOF

OUTDIR=$(readlink -f ../..)/out
PLATDIR=${OUTDIR}/bin/$(uname -o)/$(uname -m)/

MAKE=gmake
if [ "$(which ${MAKE})" == "" ]
then
        MAKE=make
fi
LDFLAGS="--static" ${MAKE} toybox

mkdir -p ${PLATDIR}
cp toybox ${PLATDIR}/toybox
