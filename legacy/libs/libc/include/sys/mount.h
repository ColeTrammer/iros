#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H 1

#define MS_RDONLY 1
#define MS_NOSUID 2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int mount(const char *source, const char *target, const char *fs_type, unsigned long flags, const void *data);
int umount(const char *target);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_MOUNT_H */
