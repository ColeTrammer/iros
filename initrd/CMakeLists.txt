set(INITRD_PROGRAMS
    aligned_alloc_test.c
    barrier_test.c
    broken.cpp
    cancel_test.cpp
    cleanup_handler_test.c
    cond_test.c
    coroutine_test.cpp
    deflate_test.cpp
    detached_thread_test.cpp
    dl_iter_test.cpp
    dlopen_test.c
    event_loop_timer_test.cpp
    event_loop_thread_test.cpp
    execve_waitpid_test.c
    fault_test.c
    fread_test.c
    fwrite.c
    getcpuclockid_test.c
    getopt_long_test.c
    getopt_test.c
    getrusage_test.c
    glob_test.c
    graphics_test.cpp
    guess.c
    hash_map_test.cpp
    if_test.c
    ipc_test.cpp
    itimer_test.cpp
    json_test.cpp
    linked_list_test_a.cpp
    local_socket_client_test.c
    local_socket_server_test.c
    map_shared_test.c
    mmap_file_test.c
    mmap_test.cpp
    mprotect_test.c
    mutex_test.c
    netdb_test.c
    nftw_test.c
    once_test.c
    orphan_test.c
    pipe_test.c
    png_test.cpp
    poll_test.c
    protections_test.c
    pthread_atfork_test.c
    pthread_attr_test.c
    queue_test.c
    rect_set_test.cpp
    regex_test.c
    rwlock_test.c
    scanf_test.c
    sched_stress_test.c
    select_test.c
    sem_test.c
    serial.c
    set_jump_test.c
    shared_memory_test.c
    sigaltstack_test.cpp
    siginfo_test.cpp
    sigqueue_test.cpp
    sigset_test.c
    sigsuspend_test_a.cpp
    smart_pointer_test.cpp
    socket_pair_test.c
    spawn_test.c
    stack_trace_test.c
    strcasecmp_test.c
    string_test.cpp
    strtoken_test.c
    syscall_sysret_test.S
    syslog_test.c
    tcp_server_test_a.c
    tcp_test_a.c
    tcp_test_b.c
    test_argv.c
    test_cpp_a.cpp
    tgkill_test.cpp
    threaded_execve.c
    thread_specific_data_test.cpp
    thread_test.c
    timer_overrun_test.c
    timer_test.cpp
    times_test.c
    tls_test.c
    tmpnam_test.c
    tui_test.cpp
    udp_test.c
    vector_test.cpp
    vm_internet_test.c
    waitpid_test.c
    we_test.c
)

foreach (SOURCE ${INITRD_PROGRAMS})
    get_filename_component(PROGRAM_NAME ${SOURCE} NAME_WE)
    set(SOURCES programs/${SOURCE})
    add_os_executable(${PROGRAM_NAME} boot/initrd)
    set(PRIVATE_INITRD_TARGETS ${PRIVATE_INITRD_TARGETS} ${PROGRAM_NAME})
endforeach()

set(INITRD_TARGETS ${PRIVATE_INITRD_TARGETS} PARENT_SCOPE)

target_link_libraries(barrier_test ${PTHREAD_LIB})
target_link_libraries(cancel_test ${PTHREAD_LIB})
target_link_libraries(cleanup_handler_test ${PTHREAD_LIB})
target_link_libraries(cond_test ${PTHREAD_LIB})
target_link_libraries(coroutine_test libliim)
target_link_libraries(deflate_test libext)
target_link_libraries(detached_thread_test ${PTHREAD_LIB})
target_link_libraries(dlopen_test ${DL_LIB} ${PTHREAD_LIB})
target_link_options(dlopen_test PRIVATE -rdynamic)
target_link_libraries(event_loop_timer_test PRIVATE libapp)
target_link_libraries(event_loop_thread_test PRIVATE libeventloop)
target_link_libraries(getcpuclockid_test ${PTHREAD_LIB})
target_link_libraries(graphics_test libgui)
target_link_libraries(hash_map_test libliim)
target_link_libraries(ipc_test libipc)
target_link_libraries(itimer_test libapp)
target_link_libraries(json_test libext)
target_link_libraries(linked_list_test_a libliim)
target_link_libraries(once_test ${PTHREAD_LIB})
target_link_libraries(map_shared_test ${PTHREAD_LIB})
target_link_libraries(mutex_test ${PTHREAD_LIB})
target_link_libraries(png_test libgui)
target_link_libraries(pthread_atfork_test ${PTHREAD_LIB})
target_link_libraries(pthread_attr_test ${PTHREAD_LIB})
target_link_libraries(rect_set_test libgui)
target_link_libraries(regex_test libliim)
target_link_libraries(rwlock_test ${PTHREAD_LIB})
target_link_libraries(sem_test ${PTHREAD_LIB})
target_link_libraries(shared_memory_test ${REALTIME_LIB})
target_link_libraries(smart_pointer_test libliim)
target_link_libraries(string_test libliim)
target_link_libraries(thread_specific_data_test ${PTHREAD_LIB})
target_link_libraries(thread_test ${PTHREAD_LIB})
target_link_libraries(threaded_execve ${PTHREAD_LIB})
target_link_libraries(tls_test ${PTHREAD_LIB})
target_link_libraries(test_cpp_a ${PTHREAD_LIB})
target_link_libraries(tgkill_test ${PTHREAD_LIB})
target_link_libraries(timer_overrun_test ${REALTIME_LIB})
target_link_libraries(timer_test ${REALTIME_LIB})
target_link_libraries(tui_test libtui)
target_link_libraries(vector_test libliim)
