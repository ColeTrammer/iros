add_os_headers(kernel_headers)

if (NOT ${NATIVE_BUILD})
    add_subdirectory("arch/${ARCH}")

    set(SOURCES
        ${KERNEL_ARCH_SOURCES}
        fs/cached_dirent.c
        fs/dev.c
        fs/ext2.c
        fs/initrd.c
        fs/pipe.c
        fs/procfs.c
        fs/tmp.c
        fs/tnode.c
        fs/vfs.c
        hal/devices.c
        hal/processor.c
        hal/ptmx.c
        irqs/irqs.c
        main/kernel.c
        mem/anon_vm_object.c
        mem/inode_vm_object.c
        mem/page_frame_allocator.c
        mem/phys_page.c
        mem/phys_vm_object.c
        mem/vm_allocator.c
        mem/vm_object.c
        mem/vm_region.c
        net/arp.c
        net/dhcp.c
        net/ethernet.c
        net/icmp.c
        net/inet_socket.c
        net/interface.c
        net/ip.c
        net/loopback.c
        net/mac.c
        net/net.c
        net/network_task.c
        net/port.c
        net/socket.c
        net/socket_syscalls.c
        net/tcp.c
        net/udp.c
        net/udp_socket.c
        net/unix_socket.c
        proc/blockers.c
        proc/elf64.c
        proc/exec.c
        proc/pid.c
        proc/process.c
        proc/profile.c
        proc/task.c
        proc/task_finalizer.c
        proc/user_mutex.c
        proc/wait_queue.c
        sched/task_sched.c
        time/clock.c
        time/timer.c
        util/hash_map.c
        util/mutex.c
        util/random.c
        util/spinlock.c
        util/validators.c
    )

    add_executable(kernel ${SOURCES})

    target_compile_options(kernel PRIVATE -ffreestanding ${KERNEL_ARCH_CFLAGS})
    target_link_options(kernel PRIVATE -nostdlib ${KERNEL_ARCH_LDFLAGS})

    target_compile_definitions(kernel PRIVATE __is_kernel)

    target_link_libraries(kernel libk kernel_headers)

    install(TARGETS kernel RUNTIME DESTINATION boot)
    set(KERNEL_TARGETS kernel PARENT_SCOPE)
endif()
