#include <kernel/hal/i686/gdt.h>

.section .text
.macro no_error_code_interrupt_handler n
.global interrupt_handler_\n
interrupt_handler_\n:
    # Push 0 error code and IRQ number
    push $0
    push $\n
    jmp _irq_entry_common
.size interrupt_handler_\n, . - interrupt_handler_\n
.endm

.macro interrupt_handler n
.global interrupt_handler_\n
interrupt_handler_\n:
    # Error code is already pushed, so just push IRQ number
    push $\n
    jmp _irq_entry_common
.size interrupt_handler_\n, . - interrupt_handler_\n
.endm

_irq_entry_common:
    # Swap error code with %eax
    xor 4(%esp), %eax
    xor %eax, 4(%esp)
    xor 4(%esp), %eax

    # Swap irq number with %ebx
    xor 0(%esp), %ebx
    xor %ebx, 0(%esp)
    xor 0(%esp), %ebx

    # Save %ecx on stack
    push %ecx

    # Check if the irq was from kernel or userspace
    # Load the old %cs value into %eax
    mov 16(%esp), %ecx
    and $3, %ecx
    jz _irq_entry_from_kernelspace

_irq_entry_from_userspace:
    # Save registers
    push %edx
    push %esi
    push %edi
    push %ebp

    pushw %gs
    pushw %fs
    pushw %es
    pushw %ds

    # Set %gs to the current processor, not the userspace thread
    mov $GS_PROCESSOR_SELECTOR, %cx
    mov %cx, %gs

    jmp _irq_entry_impl

_irq_entry_from_kernelspace:
    # FIXME: this could also switch the current stack out

    # Reserve stack space for saved %ss and %esp
    sub $8, %esp

    # Move 6 words currently on the stack over
    mov 8(%esp), %ecx
    mov %ecx, 0(%esp)
    mov 12(%esp), %ecx
    mov %ecx, 4(%esp)
    mov 16(%esp), %ecx
    mov %ecx, 8(%esp)
    mov 20(%esp), %ecx
    mov %ecx, 12(%esp)
    mov 24(%esp), %ecx
    mov %ecx, 16(%esp)
    mov 28(%esp), %ecx
    mov %ecx, 20(%esp)

    # Put %ss and %esp onto stack
    movl $DATA_SELECTOR, 28(%esp)
    lea 32(%esp), %ecx
    mov %ecx, 24(%esp)

    # Save registers
    push %edx
    push %esi
    push %edi
    push %ebp

    pushw %gs
    pushw %fs
    pushw %es
    pushw %ds

    # Fall through to handle IRQ

_irq_entry_impl:
    # Save pointer to task state
    mov %esp, %edi

    # Ensure stack is 16 byte aligned
    sub $12, %esp
    and $0xFFFFFFF0, %esp

    mov %eax, 8(%esp) # Arg 3: Errpr Code
    mov %edi, 4(%esp) # Arg 2: Task state
    mov %ebx, 0(%esp) # Arg 1: IRQ number

    call generic_irq_handler

    # Resume the task properly
    mov %edi, 0(%esp)
    call __run_task

__ll:
    jmp __ll
.size _irq_entry_common, . - _irq_entry_common

no_error_code_interrupt_handler 0
no_error_code_interrupt_handler 1
no_error_code_interrupt_handler 2
no_error_code_interrupt_handler 3
no_error_code_interrupt_handler 4
no_error_code_interrupt_handler 5
no_error_code_interrupt_handler 6
interrupt_handler 7
no_error_code_interrupt_handler 8
no_error_code_interrupt_handler 9
interrupt_handler 10
interrupt_handler 11
interrupt_handler 12
interrupt_handler 13
interrupt_handler 14
no_error_code_interrupt_handler 15
no_error_code_interrupt_handler 16
interrupt_handler 17
no_error_code_interrupt_handler 18
no_error_code_interrupt_handler 19
no_error_code_interrupt_handler 20
no_error_code_interrupt_handler 21
no_error_code_interrupt_handler 22
no_error_code_interrupt_handler 23
no_error_code_interrupt_handler 24
no_error_code_interrupt_handler 25
no_error_code_interrupt_handler 26
no_error_code_interrupt_handler 27
no_error_code_interrupt_handler 28
no_error_code_interrupt_handler 29
no_error_code_interrupt_handler 30
no_error_code_interrupt_handler 31
no_error_code_interrupt_handler 32
no_error_code_interrupt_handler 33
no_error_code_interrupt_handler 34
no_error_code_interrupt_handler 35
no_error_code_interrupt_handler 36
no_error_code_interrupt_handler 37
no_error_code_interrupt_handler 38
no_error_code_interrupt_handler 39
no_error_code_interrupt_handler 40
no_error_code_interrupt_handler 41
no_error_code_interrupt_handler 42
no_error_code_interrupt_handler 43
no_error_code_interrupt_handler 44
no_error_code_interrupt_handler 45
no_error_code_interrupt_handler 46
no_error_code_interrupt_handler 47
no_error_code_interrupt_handler 48
no_error_code_interrupt_handler 49
no_error_code_interrupt_handler 50
no_error_code_interrupt_handler 51
no_error_code_interrupt_handler 52
no_error_code_interrupt_handler 53
no_error_code_interrupt_handler 54
no_error_code_interrupt_handler 55
no_error_code_interrupt_handler 56
no_error_code_interrupt_handler 57
no_error_code_interrupt_handler 58
no_error_code_interrupt_handler 59
no_error_code_interrupt_handler 60
no_error_code_interrupt_handler 61
no_error_code_interrupt_handler 62
no_error_code_interrupt_handler 63
no_error_code_interrupt_handler 64
no_error_code_interrupt_handler 65
no_error_code_interrupt_handler 66
no_error_code_interrupt_handler 67
no_error_code_interrupt_handler 68
no_error_code_interrupt_handler 69
no_error_code_interrupt_handler 70
no_error_code_interrupt_handler 71
no_error_code_interrupt_handler 72
no_error_code_interrupt_handler 73
no_error_code_interrupt_handler 74
no_error_code_interrupt_handler 75
no_error_code_interrupt_handler 76
no_error_code_interrupt_handler 77
no_error_code_interrupt_handler 78
no_error_code_interrupt_handler 79
no_error_code_interrupt_handler 80
no_error_code_interrupt_handler 81
no_error_code_interrupt_handler 82
no_error_code_interrupt_handler 83
no_error_code_interrupt_handler 84
no_error_code_interrupt_handler 85
no_error_code_interrupt_handler 86
no_error_code_interrupt_handler 87
no_error_code_interrupt_handler 88
no_error_code_interrupt_handler 89
no_error_code_interrupt_handler 90
no_error_code_interrupt_handler 91
no_error_code_interrupt_handler 92
no_error_code_interrupt_handler 93
no_error_code_interrupt_handler 94
no_error_code_interrupt_handler 95
no_error_code_interrupt_handler 96
no_error_code_interrupt_handler 97
no_error_code_interrupt_handler 98
no_error_code_interrupt_handler 99
no_error_code_interrupt_handler 100
no_error_code_interrupt_handler 101
no_error_code_interrupt_handler 102
no_error_code_interrupt_handler 103
no_error_code_interrupt_handler 104
no_error_code_interrupt_handler 105
no_error_code_interrupt_handler 106
no_error_code_interrupt_handler 107
no_error_code_interrupt_handler 108
no_error_code_interrupt_handler 109
no_error_code_interrupt_handler 110
no_error_code_interrupt_handler 111
no_error_code_interrupt_handler 112
no_error_code_interrupt_handler 113
no_error_code_interrupt_handler 114
no_error_code_interrupt_handler 115
no_error_code_interrupt_handler 116
no_error_code_interrupt_handler 117
no_error_code_interrupt_handler 118
no_error_code_interrupt_handler 119
no_error_code_interrupt_handler 120
no_error_code_interrupt_handler 121
no_error_code_interrupt_handler 122
no_error_code_interrupt_handler 123
no_error_code_interrupt_handler 124
no_error_code_interrupt_handler 125
no_error_code_interrupt_handler 126
no_error_code_interrupt_handler 127
no_error_code_interrupt_handler 128
no_error_code_interrupt_handler 129
no_error_code_interrupt_handler 130
no_error_code_interrupt_handler 131
no_error_code_interrupt_handler 132
no_error_code_interrupt_handler 133
no_error_code_interrupt_handler 134
no_error_code_interrupt_handler 135
no_error_code_interrupt_handler 136
no_error_code_interrupt_handler 137
no_error_code_interrupt_handler 138
no_error_code_interrupt_handler 139
no_error_code_interrupt_handler 140
no_error_code_interrupt_handler 141
no_error_code_interrupt_handler 142
no_error_code_interrupt_handler 143
no_error_code_interrupt_handler 144
no_error_code_interrupt_handler 145
no_error_code_interrupt_handler 146
no_error_code_interrupt_handler 147
no_error_code_interrupt_handler 148
no_error_code_interrupt_handler 149
no_error_code_interrupt_handler 150
no_error_code_interrupt_handler 151
no_error_code_interrupt_handler 152
no_error_code_interrupt_handler 153
no_error_code_interrupt_handler 154
no_error_code_interrupt_handler 155
no_error_code_interrupt_handler 156
no_error_code_interrupt_handler 157
no_error_code_interrupt_handler 158
no_error_code_interrupt_handler 159
no_error_code_interrupt_handler 160
no_error_code_interrupt_handler 161
no_error_code_interrupt_handler 162
no_error_code_interrupt_handler 163
no_error_code_interrupt_handler 164
no_error_code_interrupt_handler 165
no_error_code_interrupt_handler 166
no_error_code_interrupt_handler 167
no_error_code_interrupt_handler 168
no_error_code_interrupt_handler 169
no_error_code_interrupt_handler 170
no_error_code_interrupt_handler 171
no_error_code_interrupt_handler 172
no_error_code_interrupt_handler 173
no_error_code_interrupt_handler 174
no_error_code_interrupt_handler 175
no_error_code_interrupt_handler 176
no_error_code_interrupt_handler 177
no_error_code_interrupt_handler 178
no_error_code_interrupt_handler 179
no_error_code_interrupt_handler 180
no_error_code_interrupt_handler 181
no_error_code_interrupt_handler 182
no_error_code_interrupt_handler 183
no_error_code_interrupt_handler 184
no_error_code_interrupt_handler 185
no_error_code_interrupt_handler 186
no_error_code_interrupt_handler 187
no_error_code_interrupt_handler 188
no_error_code_interrupt_handler 189
no_error_code_interrupt_handler 190
no_error_code_interrupt_handler 191
no_error_code_interrupt_handler 192
no_error_code_interrupt_handler 193
no_error_code_interrupt_handler 194
no_error_code_interrupt_handler 195
no_error_code_interrupt_handler 196
no_error_code_interrupt_handler 197
no_error_code_interrupt_handler 198
no_error_code_interrupt_handler 199
no_error_code_interrupt_handler 200
no_error_code_interrupt_handler 201
no_error_code_interrupt_handler 202
no_error_code_interrupt_handler 203
no_error_code_interrupt_handler 204
no_error_code_interrupt_handler 205
no_error_code_interrupt_handler 206
no_error_code_interrupt_handler 207
no_error_code_interrupt_handler 208
no_error_code_interrupt_handler 209
no_error_code_interrupt_handler 210
no_error_code_interrupt_handler 211
no_error_code_interrupt_handler 212
no_error_code_interrupt_handler 213
no_error_code_interrupt_handler 214
no_error_code_interrupt_handler 215
no_error_code_interrupt_handler 216
no_error_code_interrupt_handler 217
no_error_code_interrupt_handler 218
no_error_code_interrupt_handler 219
no_error_code_interrupt_handler 220
no_error_code_interrupt_handler 221
no_error_code_interrupt_handler 222
no_error_code_interrupt_handler 223
no_error_code_interrupt_handler 224
no_error_code_interrupt_handler 225
no_error_code_interrupt_handler 226
no_error_code_interrupt_handler 227
no_error_code_interrupt_handler 228
no_error_code_interrupt_handler 229
no_error_code_interrupt_handler 230
no_error_code_interrupt_handler 231
no_error_code_interrupt_handler 232
no_error_code_interrupt_handler 233
no_error_code_interrupt_handler 234
no_error_code_interrupt_handler 235
no_error_code_interrupt_handler 236
no_error_code_interrupt_handler 237
no_error_code_interrupt_handler 238
no_error_code_interrupt_handler 239
no_error_code_interrupt_handler 240
no_error_code_interrupt_handler 241
no_error_code_interrupt_handler 242
no_error_code_interrupt_handler 243
no_error_code_interrupt_handler 244
no_error_code_interrupt_handler 245
no_error_code_interrupt_handler 246
no_error_code_interrupt_handler 247
no_error_code_interrupt_handler 248
no_error_code_interrupt_handler 249
no_error_code_interrupt_handler 250
no_error_code_interrupt_handler 251
no_error_code_interrupt_handler 252
no_error_code_interrupt_handler 253
no_error_code_interrupt_handler 254
no_error_code_interrupt_handler 255
