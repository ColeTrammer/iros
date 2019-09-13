.section .text
.global sys_sched_run_next
sys_sched_run_next:
    # Switches Stack And Then calls arch_sched_run_next

    movabsq $KERNEL_VM_STACK_START, %rsp
    callq arch_sched_run_next

    # Should Not Be Reached
    jmp __halt
