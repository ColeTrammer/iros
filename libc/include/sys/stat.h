#ifndef _SYS_STAT_H
#define _SYS_STAT_H 1

#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 0070
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IRWXO 0007
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001

#define S_ISUID 04000
#define S_ISGID 02000
#define S_SIVTX 01000

#endif /* _SYS_STAT_H */