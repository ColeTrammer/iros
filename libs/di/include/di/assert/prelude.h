#include <di/assert/assert_binary.h>
#include <di/assert/assert_bool.h>

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_ASSERT)
#define ASSERT        DI_ASSERT
#define ASSERT_EQ     DI_ASSERT_EQ
#define ASSERT_NOT_EQ DI_ASSERT_NOT_EQ
#define ASSERT_LT     DI_ASSERT_LT
#define ASSERT_LT_EQ  DI_ASSERT_LT_EQ
#define ASSERT_GT     DI_ASSERT_GT
#define ASSERT_GT_EQ  DI_ASSERT_GT_EQ
#endif
