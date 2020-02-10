#ifndef FMTMSG_H
#define FMTMSG_H 1

#define MM_HARD    1
#define MM_SOFT    2
#define MM_FIRM    4
#define MM_APPL    8
#define MM_UTIL    16
#define MM_OPSYS   32
#define MM_RECOVER 64
#define MM_NRECOV  128
#define MM_HALT    256
#define MM_ERROR   512
#define MM_WARNING 1024
#define MM_INFO    2048
#define MM_NOSEV   4096
#define MM_PRINT   8192
#define MM_CONSOLE 16384

#define MM_NULLLBL ((char *) 0)
#define MM_NULLSEV 0
#define MM_NULLMC  0L
#define MM_NULLTXT ((char *) 0)
#define MM_NULLACT ((char *) 0)
#define MM_NULLTAG ((char *) 0)

#define MM_OK    0
#define MM_NOTOK -1
#define MM_NOMSG -2
#define MM_NOCON -3

#ifdef __cpluslpus
extern "C" {
#endif /* __cpluslpus */

int fmtmsg(long classification, const char *label, int severity, const char *text, const char *action, const char *tag);

#ifdef __cpluslpus
}
#endif /* __cpluslpus */

#endif /* FMTMSG_H */