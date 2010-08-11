.syntax unified
.thumb
check_open:
    push {r4-r7, lr}
    mov r4, r0
    mov r5, r1
    mov r6, r2
    mov r7, r3

    ldr r2, k
    mov r3, sp
    subs r0, r3, r2
    mov sp, r0
    sub sp, #4

    str r2, [sp]

    mov r0, r5
    mov r2, sp
    mov r1, r2
    adds r1, r1, #4
    ldr r3, vn_getpath
    blx r3
    cmp r0, #0
    bne call_orig_check_open

    mov r0, sp
    adds r0, r0, #4
    adr r1, mobile
    movs r2, #20
    ldr r3, strncmp
    blx r3
    cmp r0, #0
    beq check_if_preferences_open
    movs r0, #0
    b check_open_return

check_if_preferences_open:
    mov r0, sp
    adds r0, r0, #4
    adr r1, preferences
    movs r2, #40
    ldr r3, strncmp
    blx r3
    cmp r0, #0
    beq check_open_return

call_orig_check_open:
    mov r0, r4
    mov r1, r5
    mov r2, r6
    mov r3, r7
    ldr r4, mpo_vnode_check_open
    blx r4

check_open_return:
    add sp, sp, #4
    add sp, sp, #0x180
    add sp, sp, #0x180
    add sp, sp, #0x100
    pop {r4-r7, pc}

.word 0xacce5
check_access:
    push {r4-r7, lr}
    mov r4, r0
    mov r5, r1
    mov r6, r2
    mov r7, r3

    ldr r2, k
    mov r3, sp
    subs r0, r3, r2
    mov sp, r0
    sub sp, #4

    str r2, [sp]

    mov r0, r5
    mov r2, sp
    adds r1, r2, #4
    ldr r3, vn_getpath
    blx r3
    cmp r0, #0
    bne call_orig_check_access

    mov r0, sp
    adds r0, r0, #4
    adr r1, mobile
    movs r2, #20
    ldr r3, strncmp
    blx r3
    cmp r0, #0
    beq check_if_preferences_access
    movs r0, #0
    b check_access_return

check_if_preferences_access:
    mov r0, sp
    adds r0, r0, #4
    adr r1, preferences
    movs r2, #40
    ldr r3, strncmp
    blx r3
    cmp r0, #0
    beq check_access_return

call_orig_check_access:
    mov r0, r4
    mov r1, r5
    mov r2, r6
    mov r3, r7
    ldr r4, mpo_vnode_check_access
    blx r4

check_access_return:
    add sp, sp, #4
    add sp, sp, #0x180
    add sp, sp, #0x180
    add sp, sp, #0x100
    pop {r4-r7, pc}

.align 2
mobile:
    .asciz "/private/var/mobile/"

.align 2
preferences:
    .asciz "/private/var/mobile/Library/Preferences/"

.align 2
k:                      .word 0x400
strncmp:                .word 0xaaaaaaaa
vn_getpath:             .word 0xbbbbbbbb
mpo_vnode_check_open:   .word 0xcccccccc
mpo_vnode_check_access: .word 0xdddddddd
