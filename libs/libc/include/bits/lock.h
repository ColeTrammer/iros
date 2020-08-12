#ifndef _BITS_LOCK_H
#define _BITS_LOCK_H 1

#ifndef __is_libk

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct __lock {
    unsigned int __lock;
};

struct __recursive_lock {
    unsigned int __lock;
    unsigned int __count;
};

void __lock(struct __lock *lock);
int __trylock(struct __lock *lock);
void __unlock(struct __lock *lock);

void __lock_recursive(struct __recursive_lock *lock);
int __trylock_recursive(struct __recursive_lock *lock);
void __unlock_recursive(struct __recursive_lock *lock);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __is_libk */

#endif /* _BITS_LOCK_H */
