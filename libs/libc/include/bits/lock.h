#ifndef _BITS_LOCK_H
#define _BITS_LOCK_H 1

#ifndef __is_libk

void __lock(unsigned int *lock);
int __trylock(unsigned int *lock);
void __unlock(unsigned int *lock);

#endif /* __is_libk */

#endif /* _BITS_LOCK_H */