add_os_headers(kernel_headers)

if (NOT ${NATIVE_BUILD})
    add_subdirectory("arch/${ARCH}")

    set(SOURCES
        ${KERNEL_ARCH_SOURCES}
        boot/boot_info.c
        fs/cached_dirent.c
        fs/dev.c
        fs/disk_sync.c
        fs/ext2.c
        fs/initrd.c
        fs/mount.c
        fs/pipe.c
        fs/procfs.c
        fs/tmp.c
        fs/tnode.c
        fs/vfs.c
        fs/watch.c
        hal/block.c
        hal/devices.c
        hal/gpt.c
        hal/hw_device.c
        hal/hw_timer.c
        hal/input.c
        hal/isa_driver.c
        hal/mbr.c
        hal/partition.c
        hal/pci_driver.c
        hal/pci.c
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
        net/ethernet.c
        net/icmp_socket.c
        net/icmp.c
        net/inet_socket.c
        net/interface.c
        net/ip.c
        net/loopback.c
        net/neighbor_cache.c
        net/network_task.c
        net/packet.c
        net/port.c
        net/destination_cache.c
        net/socket.c
        net/socket_syscalls.c
        net/tcp_socket.c
        net/tcp.c
        net/udp_socket.c
        net/udp.c
        net/umessage.c
        net/unix_socket.c
        proc/elf64.c
        proc/exec.c
        proc/pid.c
        proc/process.c
        proc/profile.c
        proc/stats.c
        proc/task.c
        proc/task_finalizer.c
        proc/user_mutex.c
        proc/wait_queue.c
        sched/task_sched.c
        time/clock.c
        time/timer.c
        util/bitset.c
        util/hash_map.c
        util/mutex.c
        util/random.c
        util/ring_buffer.c
        util/spinlock.c
        util/validators.c
    )

    add_executable(kernel ${SOURCES})

    target_compile_options(kernel PRIVATE -ffreestanding ${KERNEL_ARCH_CFLAGS})
    target_link_options(kernel PRIVATE -nostdlib ${KERNEL_ARCH_LDFLAGS})

    target_compile_definitions(kernel PRIVATE __is_kernel)

    target_link_libraries(kernel libk kernel_headers)

    install(TARGETS kernel RUNTIME DESTINATION boot)
endif()
