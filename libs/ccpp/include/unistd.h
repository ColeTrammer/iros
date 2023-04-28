#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/getopt_short.h>
#include <ccpp/bits/intptr_t.h>
#include <ccpp/bits/off_t.h>
#include <ccpp/bits/size_t.h>
#include <ccpp/bits/ssize_t.h>
#include <ccpp/bits/suseconds_t.h>

#include __CCPP_PLATFORM_PATH(seek_constants.h)

__CCPP_BEGIN_DECLARATIONS

unsigned alarm(unsigned __seconds);

int usleep(suseconds_t __useconds);

off_t lseek(int __fd, off_t __offset, int __whence);

int close(int __fd);
ssize_t read(int __fd, void* __buffer, size_t __count);
ssize_t write(int __fd, void const* __buffer, size_t __count);

int chdir(char const* __path);
int fchdir(int __fd);
char* getcwd(char* __buffer, size_t __size);

__CCPP_END_DECLARATIONS
