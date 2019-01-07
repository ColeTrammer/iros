.section .text
.global handle_double_fault_entry
handle_double_fault_entry:
    call handle_double_fault
    iretq

.global handle_general_protection_fault_entry
handle_general_protection_fault_entry:
    movq (%rsp), %rdi
    call handle_general_protection_fault
    add $8, %rsp
    iretq

.global handle_page_fault_entry
handle_page_fault_entry:
    mov %cr2, %rdi
    movq (%rsp), %rsi
    call handle_page_fault
    add $8, %rsp
    iretq