.section .text
.global handle_double_fault_entry
handle_double_fault_entry:
    call handle_double_fault

.global handle_page_fault_entry
handle_page_fault_entry:
    mov %cr2, %rdi
    call handle_page_fault