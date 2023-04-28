#pragma once

#include <ccpp/bits/config.h>
#include <ccpp/bits/signal.h>

__CCPP_BEGIN_DECLARATIONS

typedef int sig_atomic_t;

void (*signal(int __sig, void (*__handler)(int)))(int);
int raise(int __sig);

__CCPP_END_DECLARATIONS
