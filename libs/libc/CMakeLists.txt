set(WORDEXP_LIB_SOURCES
    fnmatch/fnmatch.c
    glob/glob.c
    wordexp/wordexp.c
)

add_library(libwordexp OBJECT ${WORDEXP_LIB_SOURCES})
set_property(TARGET libwordexp PROPERTY POSITION_INDEPENDENT_CODE 1)

set(REGEX_GENERATED_FILES
    regex/regex_token_type.h
    regex/generic_regex_parser_impl.cpp
    regex/generic_regex_parser.h
    regex/regex.info
)

set(REGEX_LIB_SOURCES
    ${REGEX_GENERATED_FILES}
    regex/regex_lexer.cpp
    regex/regex_graph.cpp
    regex/generic_regex_parser.cpp
    regex/regcomp.cpp
    regex/regerror.cpp
    regex/regexec.cpp
    regex/regfree.cpp
)

add_custom_command(
    OUTPUT ${REGEX_GENERATED_FILES}
    COMMAND /bin/mkdir -p regex
    COMMAND ${NATIVE_PARSER} -p regex ${CMAKE_CURRENT_SOURCE_DIR}/regex/regex.y 2> regex/regex.info
    VERBATIM
    DEPENDS ${NATIVE_PARSER}
    MAIN_DEPENDENCY regex/regex.y
)
add_custom_target(generate-regex-parser DEPENDS ${REGEX_GENERATED_FILES})

add_library(libregex OBJECT ${REGEX_LIB_SOURCES})
target_link_libraries(libregex PRIVATE libliim parser_headers)
target_include_directories(libregex PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/regex)
add_dependencies(libregex generate-regex-parser)
set_property(TARGET libregex PROPERTY POSITION_INDEPENDENT_CODE 1)

if (${NATIVE_BUILD})
    target_compile_definitions(libregex PRIVATE USERLAND_NATIVE)
    target_compile_definitions(libwordexp PRIVATE USERLAND_NATIVE)
else()
    target_compile_definitions(libregex PRIVATE __is_libc)
    target_compile_definitions(libwordexp PRIVATE __is_libc)

    add_subdirectory("arch/${ARCH}")

    set(CRT0_PATH ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/crt0_object.dir/arch/${ARCH}/crt0.S.o)

    add_library(crt0_object OBJECT arch/${ARCH}/crt0.S)

    set(FREE_SOURCES
        ${ARCH_FREE_SOURCES}
        arpa/inet/inet_addr.c
        arpa/inet/inet_aton.c
        arpa/inet/inet_ntoa.c
        ctype/ctype.c
        search/queue.c
        stdio/printf.c
        stdio/scanf.c
        stdlib/abort.c
        stdlib/conversions.c
        stdlib/exit.c
        stdlib/libgcc_polyfill.c
        stdlib/malloc.c
        stdlib/mbstowcs.c
        stdlib/mbtowc.c
        stdlib/stdlib.c
        stdlib/wcstombs.c
        stdlib/wctomb.c
        string/memchr.c
        string/memcmp.c
        string/memcpy.c
        string/memmove.c
        string/memset.c
        string/stpcpy.c
        string/stpncpy.c
        string/strcat.c
        string/strchr.c
        string/strcmp.c
        string/strcpy.c
        string/strcspn.c
        string/strdup.c
        string/strerror.c
        string/strlen.c
        string/strncat.c
        string/strncmp.c
        string/strncpy.c
        string/strndup.c
        string/strnlen.c
        string/strpbrk.c
        string/strrchr.c
        string/strsignal.c
        string/strspn.c
        string/strstr.c
        string/strtok_r.c
        string/strtok.c
        strings/strings.c
    )

    set(HOSTED_SOURCES
        ${ARCH_HOSTED_SOURCES}
        assert/assert.c
        bits/lock/__lock.c
        bits/lock/__lock_recursive.c
        bits/lock/__trylock.c
        bits/lock/__trylock_recursive.c
        bits/lock/__unlock.c
        bits/lock/__unlock_recursive.c
        bits/__allocate_thread_control_block.c
        bits/__cxa_atexit.c
        bits/__cxa_finalize.c
        bits/__free_thread_control_block.c
        bits/__parse_fields.c
        dirent/dirent.c
        dirent/rewinddir.c
        dlfcn/dl_error_storage.c
        dlfcn/dladdr.c
        dlfcn/dlclose.c
        dlfcn/dlerror.c
        dlfcn/dlopen.c
        dlfcn/dlsym.c
        execinfo/dump_backtrace.c
        fcntl/creat.c
        fcntl/fcntl.c
        fcntl/open.c
        fcntl/openat.c
        ftw/ftw.c
        getopt/__do_getopt.c
        getopt/getopt.c
        getopt/getopt_long.c
        getopt/getopt_long_only.c
        grp/endgrent.c
        grp/getgrent.c
        grp/getgrent_r.c
        grp/getgrgid.c
        grp/getgrgid_r.c
        grp/getgrnam.c
        grp/getgrnam_r.c
        grp/getgrouplist.c
        grp/initgroups.c
        grp/setgrent.c
        grp/setgroups.c
        libgen/basename.c
        libgen/dirname.c
        link/dl_iterate_phdr.c
        locale/locale.c
        math/math.c
        mntent/endmntent.c
        mntent/getmntent.c
        mntent/setmntent.c
        net/if/if_freenameindex.c
        net/if/if_indextoname.c
        net/if/if_nameindex.c
        net/if/if_nametoindex.c
        netdb/endhostent.c
        netdb/endnetent.c
        netdb/endprotoent.c
        netdb/endservent.c
        netdb/gethostbyaddr.c
        netdb/gethostbyname.c
        netdb/gethostent.c
        netdb/gethostent_r.c
        netdb/getnetbyaddr.c
        netdb/getnetbyname.c
        netdb/getnetent.c
        netdb/getnetent_r.c
        netdb/getprotobyname.c
        netdb/getprotobynumber.c
        netdb/getprotoent.c
        netdb/getprotoent_r.c
        netdb/getservbyname.c
        netdb/getservbyport.c
        netdb/getservent.c
        netdb/getservent_r.c
        netdb/h_errno.c
        netdb/netdb.c
        netdb/sethostent.c
        netdb/setnetent.c
        netdb/setprotoent.c
        netdb/setservent.c
        netinet/in6_addr.c
        poll/poll.c
        poll/ppoll.c
        pthread/atfork.c
        pthread/attr.c
        pthread/barrier.c
        pthread/barrierattr.c
        pthread/cancel.c
        pthread/cond.c
        pthread/condattr.c
        pthread/getcpuclockid.c
        pthread/key.c
        pthread/mutex.c
        pthread/mutexattr.c
        pthread/once.c
        pthread/pthread.c
        pthread/rwlock.c
        pthread/rwlockattr.c
        pthread/spinlock.c
        pwd/endpwent.c
        pwd/getpwent.c
        pwd/getpwent_r.c
        pwd/getpwnam.c
        pwd/getpwnam_r.c
        pwd/getpwuid.c
        pwd/getpwuid_r.c
        pwd/setpwent.c
        sched/sched_yield.c
        semaphore/__sem_wait.c
        semaphore/sem_close.c
        semaphore/sem_destroy.c
        semaphore/sem_getvalue.c
        semaphore/sem_init.c
        semaphore/sem_open.c
        semaphore/sem_post.c
        semaphore/sem_timedwait.c
        semaphore/sem_trywait.c
        semaphore/sem_unlink.c
        semaphore/sem_wait.c
        setjmp/__sigsetjmp.c
        setjmp/siglongjmp.c
        signal/kill.c
        signal/killpg.c
        signal/psiginfo.c
        signal/psignal.c
        signal/raise.c
        signal/sigaction.c
        signal/sigaddset.c
        signal/sigaltstack.c
        signal/sigdelset.c
        signal/sigemptyset.c
        signal/sigfillset.c
        signal/siginterrupt.c
        signal/sigismember.c
        signal/sigismember.c
        signal/signal.c
        signal/sigpending.c
        signal/sigprocmask.c
        signal/sigqueue.c
        signal/sigsuspend.c
        spawn/posix_spawnattr_destroy.c
        spawn/posix_spawnattr_getflags.c
        spawn/posix_spawnattr_getpgroup.c
        spawn/posix_spawnattr_getschedparam.c
        spawn/posix_spawnattr_getschedpolicy.c
        spawn/posix_spawnattr_getsigdefault.c
        spawn/posix_spawnattr_getsigmask.c
        spawn/posix_spawnattr_init.c
        spawn/posix_spawnattr_setflags.c
        spawn/posix_spawnattr_setpgroup.c
        spawn/posix_spawnattr_setschedparam.c
        spawn/posix_spawnattr_setschedpolicy.c
        spawn/posix_spawnattr_setsigdefault.c
        spawn/posix_spawnattr_setsigmask.c
        spawn/posix_spawn.c
        spawn/posix_spawn_file_actions_addclose.c
        spawn/posix_spawn_file_actions_adddup2.c
        spawn/posix_spawn_file_actions_add_internal.c
        spawn/posix_spawn_file_actions_addopen.c
        spawn/posix_spawn_file_actions_destroy.c
        spawn/posix_spawn_file_actions_init.c
        spawn/posix_spawn_internal.c
        spawn/posix_spawnp.c
        stdio/clearerr_unlocked.c
        stdio/clearerr.c
        stdio/fclose.c
        stdio/fdopen.c
        stdio/feof_unlocked.c
        stdio/feof.c
        stdio/ferror_unlocked.c
        stdio/ferror.c
        stdio/fflush_unlocked.c
        stdio/fflush.c
        stdio/fgetc_unlocked.c
        stdio/fgetc.c
        stdio/fgetpos.c
        stdio/fgets_unlocked.c
        stdio/fgets.c
        stdio/fileno_unlocked.c
        stdio/fileno.c
        stdio/flockfile.c
        stdio/fopen_internal.c
        stdio/fopen.c
        stdio/fputc_unlocked.c
        stdio/fputc.c
        stdio/fputs_unlocked.c
        stdio/fputs.c
        stdio/fread_unlocked.c
        stdio/fread.c
        stdio/freopen.c
        stdio/fseek_unlocked.c
        stdio/fseek.c
        stdio/fseeko_unlocked.c
        stdio/fseeko.c
        stdio/fsetpos.c
        stdio/ftell_unlocked.c
        stdio/ftell.c
        stdio/ftello_unlocked.c
        stdio/ftello.c
        stdio/ftrylockfile.c
        stdio/funlockfile.c
        stdio/fwrite_unlocked.c
        stdio/fwrite.c
        stdio/getc_unlocked.c
        stdio/getc.c
        stdio/getchar_unlocked.c
        stdio/getchar.c
        stdio/getdelim.c
        stdio/getline.c
        stdio/gets.c
        stdio/init_files.c
        stdio/pclose.c
        stdio/perror.c
        stdio/popen.c
        stdio/putc_unlocked.c
        stdio/putc.c
        stdio/putchar_unlocked.c
        stdio/putchar.c
        stdio/puts.c
        stdio/remove.c
        stdio/rewind.c
        stdio/setbuf.c
        stdio/setvbuf.c
        stdio/tmpfile.c
        stdio/tmpnam.c
        stdio/ungetc.c
        stdlib/_Exit.c
        stdlib/atexit.c
        stdlib/bsearch.c
        stdlib/env.c
        stdlib/grantpt.c
        stdlib/init_env.c
        stdlib/realpath.c
        stdlib/unlockpt.c
        string/strcoll.c
        string/strxfrm.c
        sys/ioctl/ioctl.c
        sys/mman/mlock.c
        sys/mman/mman.c
        sys/mount/mount.c
        sys/mount/umount.c
        sys/os_2/create_task.c
        sys/os_2/disable_profiling.c
        sys/os_2/enable_profiling.c
        sys/os_2/exit_task.c
        sys/os_2/getcpuclockid.c
        sys/os_2/os_mutex.c
        sys/os_2/read_profile.c
        sys/os_2/set_thread_self_pointer.c
        sys/os_2/tgkill.c
        sys/resource/getpriority.c
        sys/resource/getrlimit.c
        sys/resource/getrusage.c
        sys/resource/setpriority.c
        sys/resource/setrlimit.c
        sys/socket/recvmsg.c
        sys/socket/sendmsg.c
        sys/socket/socket.c
        sys/socket/socketpair.c
        sys/select/select.c
        sys/stat/chmod.c
        sys/stat/fchmod.c
        sys/stat/fchmodat.c
        sys/stat/fstat.c
        sys/stat/fstatat.c
        sys/stat/futimens.c
        sys/stat/lstat.c
        sys/stat/mkdir.c
        sys/stat/mkfifo.c
        sys/stat/mknod.c
        sys/stat/stat.c
        sys/stat/umask.c
        sys/stat/utimensat.c
        sys/statvfs/fstatvfs.c
        sys/statvfs/statvfs.c
        sys/syscall/syscall_to_string.c
        sys/time/getitimer.c
        sys/time/gettimeofday.c
        sys/time/setitimer.c
        sys/time/utimes.c
        sys/times/times.c
        sys/uio/readv.c
        sys/uio/writev.c
        sys/utsname/utsname.c
        sys/wait/wait.c
        syslog/closelog.c
        syslog/logmask.c
        syslog/openlog.c
        syslog/setlogmask.c
        syslog/syslog.c
        syslog/syslog_internal.c
        syslog/vsyslog.c
        termios/termios.c
        termios/cfmakeraw.c
        time/clock_getcpuclockid.c
        time/clock_getres.c
        time/clock_gettime.c
        time/clock_nanosleep.c
        time/clock_settime.c
        time/mktime.c
        time/nanosleep.c
        time/time.c
        time/timer_create.c
        time/timer_delete.c
        time/timer_getoverrun.c
        time/timer_gettime.c
        time/timer_settime.c
        ulimit/ulimit.c
        unistd/_exit.c
        unistd/access.c
        unistd/alarm.c
        unistd/chdir.c
        unistd/chown.c
        unistd/close.c
        unistd/confstr.c
        unistd/dup.c
        unistd/dup2.c
        unistd/execl.c
        unistd/execle.c
        unistd/execlp.c
        unistd/execv.c
        unistd/execve.c
        unistd/execvp.c
        unistd/execvpe.c
        unistd/faccessat.c
        unistd/fchdir.c
        unistd/fchown.c
        unistd/fchownat.c
        unistd/fork.c
        unistd/fpathconf.c
        unistd/fsync.c
        unistd/ftruncate.c
        unistd/getcwd.c
        unistd/getegid.c
        unistd/getgroups.c
        unistd/geteuid.c
        unistd/getgid.c
        unistd/gethostname.c
        unistd/getlogin_r.c
        unistd/getlogin.c
        unistd/getpagesize.c
        unistd/getpgid.c
        unistd/getpgrp.c
        unistd/getpid.c
        unistd/getppid.c
        unistd/getsid.c
        unistd/getuid.c
        unistd/isatty.c
        unistd/lchown.c
        unistd/link.c
        unistd/lseek.c
        unistd/nice.c
        unistd/pathconf.c
        unistd/pause.c
        unistd/pipe.c
        unistd/pread.c
        unistd/pwrite.c
        unistd/read.c
        unistd/readlink.c
        unistd/rename.c
        unistd/rmdir.c
        unistd/sbrk.c
        unistd/setegid.c
        unistd/seteuid.c
        unistd/setgid.c
        unistd/setpgid.c
        unistd/setsid.c
        unistd/setuid.c
        unistd/sleep.c
        unistd/symlink.c
        unistd/sync.c
        unistd/sysconf.c
        unistd/tcgetpgrp.c
        unistd/tcsetpgrp.c
        unistd/truncate.c
        unistd/ttyname_r.c
        unistd/ttyname.c
        unistd/unlink.c
        unistd/usleep.c
        unistd/write.c
        utime/utime.c
        wchar/wcschr.c
        wchar/wcscpy.c
        wchar/wcslen.c
        wctype/iswupper.c
        wctype/towlower.c
        cxx.cpp
        init.c
    )

    set(SOURCES
        ../libext/checksum_crc32.cpp
        ../libext/checksum_internet.cpp
        ${FREE_SOURCES}
    )
    add_os_static_library(libk k FALSE)

    set(SOURCES ${FREE_SOURCES} ${HOSTED_SOURCES})
    add_os_static_library(libc_static c FALSE)
    target_compile_definitions(libc_static PRIVATE __is_libc)
    target_include_directories(libc_static PUBLIC include)
    target_link_libraries(libc_static libwordexp libregex dns_service_headers kernel_headers)
    add_dependencies(libc_static crt0_object)

    add_os_library(libc c TRUE)
    target_compile_definitions(libc PRIVATE __is_libc)
    target_include_directories(libc PUBLIC include)
    target_link_libraries(libc libwordexp libregex dns_service_headers kernel_headers)
    target_link_options(libc PRIVATE -nostartfiles -nostdlib)
    add_dependencies(libc crt0_object)

    target_compile_definitions(libk PRIVATE __is_libk)
    set_target_properties(libk PROPERTIES COMPILE_OPTIONS "${KERNEL_ARCH_CFLAGS}")
    target_link_libraries(libk PRIVATE kernel_headers)
    target_include_directories(libk PUBLIC ../libext/include ../libliim/include)

    set(SOURCES bits/empty_library_stub.c)
    add_os_library(libdl dl FALSE)
    add_os_static_library(libdl_static dl FALSE)
    add_os_library(libm m FALSE)
    add_os_static_library(libm_static m FALSE)
    add_os_library(libpthread pthread FALSE)
    add_os_static_library(libpthread_static pthread FALSE)
    add_os_library(librt rt FALSE)
    add_os_static_library(librt_static rt FALSE)
    target_link_options(libdl PRIVATE -nostartfiles -nostdlib)
    target_link_options(libdl_static PRIVATE -nostartfiles -nostdlib)
    target_link_options(libm PRIVATE -nostartfiles -nostdlib)
    target_link_options(libm_static PRIVATE -nostartfiles -nostdlib)
    target_link_options(libpthread PRIVATE -nostartfiles -nostdlib)
    target_link_options(libpthread_static PRIVATE -nostartfiles -nostdlib)
    target_link_options(librt PRIVATE -nostartfiles -nostdlib)
    target_link_options(librt_static PRIVATE -nostartfiles -nostdlib)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -nostdinc++")

    add_custom_target(
        bootstrap-core-libs
        COMMAND readelf -d "${CMAKE_CURRENT_BINARY_DIR}/libc.so" | grep "libgcc_s.so" >/dev/null || patchelf --add-needed libgcc_s.so "${CMAKE_CURRENT_BINARY_DIR}/libc.so"
        COMMAND ${CMAKE_COMMAND} -DBUILD_TYPE=${CMAKE_BUILD_TYPE} -P ${CMAKE_BINARY_DIR}/libs/libc/cmake_install.cmake > /dev/null
        COMMAND ${CMAKE_COMMAND} -DBUILD_TYPE=${CMAKE_BUILD_TYPE} -P ${CMAKE_BINARY_DIR}/libs/loader/cmake_install.cmake > /dev/null
        DEPENDS libc libc_static libdl libdl_static libk libm libm_static libpthread libpthread_static librt librt_static loader
    )

    install(FILES ${CRT0_PATH} DESTINATION usr/lib RENAME crt0.o)
endif()
