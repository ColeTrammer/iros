.section .text
.global __handle_double_fault
__handle_double_fault:
    call handle_double_fault