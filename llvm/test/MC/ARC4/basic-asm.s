; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; ALU register-register instructions
; add r0, r1, r2: I(8)|A(0)|B(1)|C(2) = 0x40008400
; CHECK: add	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x40]
add r0, r1, r2

; CHECK: sub	r3, r4, r5              ; encoding: [0x00,0x0a,0x62,0x50]
sub r3, r4, r5

; CHECK: and	r6, r7, r8              ; encoding: [0x00,0x90,0xc3,0x60]
and r6, r7, r8

; CHECK: or	r9, r10, r11            ; encoding: [0x00,0x16,0x25,0x69]
or r9, r10, r11

; CHECK: xor	r12, r13, r14           ; encoding: [0x00,0x9c,0x86,0x79]
xor r12, r13, r14

; Single-operand instructions
; CHECK: asr	r1, r2                  ; encoding: [0x00,0x02,0x21,0x18]
asr r1, r2

; CHECK: lsr	r3, r4                  ; encoding: [0x00,0x04,0x62,0x18]
lsr r3, r4

; CHECK: sexb	r5, r6                  ; encoding: [0x00,0x0a,0xa3,0x18]
sexb r5, r6

; CHECK: extb	r7, r8                  ; encoding: [0x00,0x0e,0xe4,0x18]
extb r7, r8

; Aliases
; mov expands to and a, b, b
; CHECK: mov	r0, r1                  ; encoding: [0x00,0x82,0x00,0x60]
mov r0, r1
