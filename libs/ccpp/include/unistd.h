#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/intptr_t.h>
#include <ccpp/bits/off_t.h>
#include <ccpp/bits/useconds_t.h>

#include __CCPP_PLATFORM_PATH(seek_constants.h)

__CCPP_BEGIN_DECLARATIONS

unsigned alarm(unsigned __seconds);

int usleep(useconds_t __useconds);

off_t lseek(int __fd, off_t __offset, int __whence);

extern char* optarg;

extern int optind;
extern int opterr;
extern int optopt;

int getopt(int __argc, char* const __argv[], char const* __envp);

__CCPP_END_DECLARATIONS
