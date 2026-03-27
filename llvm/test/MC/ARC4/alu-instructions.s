; RUN: llvm-mc -triple arc4 -show-encoding < %s | FileCheck %s

; ============================================================
; ALU register-register-register: op a, b, c
; ============================================================

; CHECK: add	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x40]
add r1, r2, r3
; CHECK: adc	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x48]
adc r1, r2, r3
; CHECK: sub	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x50]
sub r1, r2, r3
; CHECK: sbc	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x58]
sbc r1, r2, r3
; CHECK: and	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x60]
and r1, r2, r3
; CHECK: or	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x68]
or r1, r2, r3
; CHECK: bic	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x70]
bic r1, r2, r3
; CHECK: xor	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x78]
xor r1, r2, r3

; ============================================================
; ALU register-register-shimm: op a, b, shimm
; shimm range: -256 to 255 (9-bit signed)
; ============================================================

; CHECK: add	r1, r2, 42 ; encoding: [0x2a,0x7e,0x21,0x40]
add r1, r2, 42
; CHECK: add	r1, r2, -1 ; encoding: [0xff,0x7f,0x21,0x40]
add r1, r2, -1
; CHECK: add	r1, r2, 255 ; encoding: [0xff,0x7e,0x21,0x40]
add r1, r2, 255
; CHECK: add	r1, r2, -256 ; encoding: [0x00,0x7f,0x21,0x40]
add r1, r2, -256

; ============================================================
; ALU register-register-limm: op a, b, limm
; Values outside shimm range automatically use limm (8-byte insn)
; ============================================================

; CHECK: add	r1, r2, 256 ; encoding: [0x00,0x7c,0x21,0x40,0x00,0x01,0x00,0x00]
add r1, r2, 256
; CHECK: add	r1, r2, 3735928559 ; encoding: [0x00,0x7c,0x21,0x40,0xef,0xbe,0xad,0xde]
add r1, r2, 0xDEADBEEF

; ============================================================
; ALU discard-destination (test mode): op 0, b, c/shimm/limm
; Per spec: A=63 discards result; used for flag-setting tests.
; ============================================================

; CHECK: add	0, r1, r2 ; encoding: [0x00,0x84,0xe0,0x47]
add 0, r1, r2
; CHECK: sub	0, r3, r4 ; encoding: [0x00,0x88,0xe1,0x57]
sub 0, r3, r4
; CHECK: add	0, r1, 42 ; encoding: [0x2a,0xfe,0xe0,0x47]
add 0, r1, 42
; CHECK: add	0, r1, 1000 ; encoding: [0x00,0xfc,0xe0,0x47,0xe8,0x03,0x00,0x00]
add 0, r1, 1000

; ============================================================
; ALU shimm-as-operand1: op a, shimm, c
; Per spec: B=shimm_sentinel, C=register
; ============================================================

; CHECK: add	r1, 42, r2 ; encoding: [0x2a,0x84,0x3f,0x40]
add r1, 42, r2

; ============================================================
; ALU limm-as-operand1: op a, limm, c
; Per spec: B=62 (limm), C=register
; ============================================================

; CHECK: add	r1, 1000, r2 ; encoding: [0x00,0x04,0x3f,0x40,0xe8,0x03,0x00,0x00]
add r1, 1000, r2
