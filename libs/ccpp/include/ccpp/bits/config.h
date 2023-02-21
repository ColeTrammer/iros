#pragma once

#ifdef __cplusplus
#define __CCPP_BEGIN_DECLARATIONS extern "C" {
#define __CCPP_END_DECLARATIONS   }
#else
#define __CCPP_BEGIN_DECLARATIONS
#define __CCPP_END_DECLARATIONS
#endif

#define __CCPP_RESTRICT __restrict
