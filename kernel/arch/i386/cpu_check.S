.section .text
.global supports_longmode
supports_longmode:
    # Push flags and put them in ecx
    pushf
    pop %ecx

    # Store flags
    mov %ecx, %edx

    # Flip bit 21
    xor $(1 << 21), %ecx

    # Load that value to flags
    push %ecx
    popf

    # Push flags and put them in ecx
    pushf
    pop %ecx

    # Compare it with the old flags
    cmp %edx, %ecx

    # Cpuid is available if they are different
    jnz cpuid_yes

    # Else print error message and halt
    push $cpuid_no_str
    call kprint
    jmp __halt

cpuid_yes:
    # Use cpuid to check for extended functions
    mov $0x80000000, %eax
    cpuid
    cmp $0x80000001, %eax

    # Extension if available if eax >= 0x80000001
    jge check_function_yes

    # Else print error message and halt
    push $check_function_no_str
    call kprint
    jmp __halt

check_function_yes:
    # Call function to check for long mode
    mov $0x80000001, %eax
    cpuid
    # If bit 29 is set, long mode is enabled
    test $(1 << 29), %edx
    jnz longmode_yes
    
    # Return false
    mov $0, %eax
    ret

longmode_yes:
    # Return true
    mov $1, %eax
    ret

.section .rodata
cpuid_no_str:
.asciz "Cpuid unsupported."
check_function_no_str:
.asciz "No function to check long mode."