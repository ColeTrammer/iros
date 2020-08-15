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
        netinet/in.c
        search/queue.c
        stdio/printf.c
        stdio/scanf.c
        stdlib/abort.c
        stdlib/conversions.c
        stdlib/malloc.c
        stdlib/stdlib.c
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
        fcntl/creat.c
        fcntl/fcntl.c
        fcntl/open.c
        fcntl/openat.c
        ftw/ftw.c
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
        locale/locale.c
        netdb/endhostent.c
        netdb/endprotoent.c
        netdb/gethostbyname.c
        netdb/gethostent.c
        netdb/gethostent_r.c
        netdb/getprotobyname.c
        netdb/getprotobynumber.c
        netdb/getprotoent.c
        netdb/getprotoent_r.c
        netdb/h_errno.c
        netdb/netdb.c
        netdb/sethostent.c
        netdb/setprotoent.c
        poll/poll.c
        poll/ppoll.c
        pwd/endpwent.c
        pwd/getpwent.c
        pwd/getpwent_r.c
        pwd/getpwnam.c
        pwd/getpwnam_r.c
        pwd/getpwuid.c
        pwd/getpwuid_r.c
        pwd/setpwent.c
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
        stdio/fsetpos.c
        stdio/ftell.c
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
        sys/mman/mman.c
        sys/os_2/create_task.c
        sys/os_2/disable_profiling.c
        sys/os_2/enable_profiling.c
        sys/os_2/exit_task.c
        sys/os_2/getcpuclockid.c
        sys/os_2/os_mutex.c
        sys/os_2/read_profile.c
        sys/os_2/set_thread_self_pointer.c
        sys/os_2/tgkill.c
        sys/resource/getrlimit.c
        sys/resource/setrlimit.c
        sys/socket/socket.c
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
        sys/time/gettimeofday.c
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
        time/clock_getcpuclockid.c
        time/clock_getres.c
        time/clock_gettime.c
        time/clock_nanosleep.c
        time/clock_settime.c
        time/mktime.c
        time/nanosleep.c
        time/time.c
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
        unistd/getopt.c
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
        wctype/iswupper.c
        wctype/towlower.c
        cxx.cpp
        init.c
    )

    set(SOURCES ${FREE_SOURCES})
    add_os_static_library(libk k FALSE)

    set(SOURCES ${SOURCES} ${HOSTED_SOURCES})
    add_os_static_library(libc_static c FALSE)
    target_compile_definitions(libc_static PRIVATE __is_libc __is_static)
    target_include_directories(libc_static PUBLIC include)
    target_link_libraries(libc_static libwordexp libregex)
    add_dependencies(libc_static crt0_object)

    add_os_library(libc c TRUE)
    target_compile_definitions(libc PRIVATE __is_libc __is_shared)
    target_include_directories(libc PUBLIC include)
    target_link_libraries(libc libwordexp libregex)
    target_link_options(libc PRIVATE -nostartfiles -nostdlib)
    add_dependencies(libc crt0_object)

    target_compile_definitions(libk PRIVATE __is_libk)
    set_target_properties(libk PROPERTIES COMPILE_OPTIONS "${KERNEL_ARCH_CFLAGS}")
    target_link_libraries(libk PRIVATE kernel_headers)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -nostdinc++")

    add_custom_target(
        install-libc
        COMMAND ${CMAKE_COMMAND} -DBUILD_TYPE=${CMAKE_BUILD_TYPE} -P ${CMAKE_BINARY_DIR}/libs/libc/cmake_install.cmake > /dev/null
        COMMAND ${CMAKE_COMMAND} -DBUILD_TYPE=${CMAKE_BUILD_TYPE} -P ${CMAKE_BINARY_DIR}/libs/libm/cmake_install.cmake > /dev/null
        COMMAND ${CMAKE_COMMAND} -DBUILD_TYPE=${CMAKE_BUILD_TYPE} -P ${CMAKE_BINARY_DIR}/libs/libpthread/cmake_install.cmake > /dev/null
        DEPENDS libc libc_static libk libm libpthread
    )

    install(FILES ${CRT0_PATH} DESTINATION usr/lib RENAME crt0.o)
endif()
