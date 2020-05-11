#ifndef _BITS_LOCK_H
#define _BITS_LOCK_H 1

#ifndef __is_libk

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void __lock(unsigned int *lock);
int __trylock(unsigned int *lock);
void __unlock(unsigned int *lock);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __is_libk */

#endif /* _BITS_LOCK_H */
