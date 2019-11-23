#ifndef __null_defined
#define __null_defined 1


#ifdef __cplusplus
#if __cplusplus <= 199711L
#define NULL (0)
#else
#define NULL (nullptr)
#endif /* __cplusplus <= 199711L */
#else
#define NULL ((void*) 0)
#endif /* __cplusplus */

#endif /* __null_defined */