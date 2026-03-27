; RUN: llvm-mc -triple arc4 -show-encoding < %s | FileCheck %s

; ============================================================
; Single-operand register form: op a, b
; ============================================================

; CHECK: asr	r1, r2 ; encoding: [0x00,0x02,0x21,0x18]
asr r1, r2
; CHECK: lsr	r3, r4 ; encoding: [0x00,0x04,0x62,0x18]
lsr r3, r4
; CHECK: ror	r5, r6 ; encoding: [0x00,0x06,0xa3,0x18]
ror r5, r6
; CHECK: rrc	r7, r8 ; encoding: [0x00,0x08,0xe4,0x18]
rrc r7, r8
; CHECK: sexb	r9, r10 ; encoding: [0x00,0x0a,0x25,0x19]
sexb r9, r10
; CHECK: sexw	r11, r12 ; encoding: [0x00,0x0c,0x66,0x19]
sexw r11, r12
; CHECK: extb	r13, r14 ; encoding: [0x00,0x0e,0xa7,0x19]
extb r13, r14
; CHECK: extw	r15, r16 ; encoding: [0x00,0x10,0xe8,0x19]
extw r15, r16

; ============================================================
; Single-operand shimm form: op a, shimm
; Per spec: B=shimm_sentinel, C=sub-opcode
; ============================================================

; CHECK: asr	r1, 42 ; encoding: [0x2a,0x82,0x3f,0x18]
asr r1, 42

; ============================================================
; Single-operand limm form: op a, limm
; Per spec: B=62 (limm), C=sub-opcode
; ============================================================

; CHECK: asr	r1, 1000 ; encoding: [0x00,0x02,0x3f,0x18,0xe8,0x03,0x00,0x00]
asr r1, 1000

; ============================================================
; Single-operand discard (test): op 0, b
; Per spec: A=63 discards result, used for flag testing
; ============================================================

; CHECK: asr	0, r1 ; encoding: [0x00,0x82,0xe0,0x1f]
asr 0, r1

; ============================================================
; FLAG instruction (special single-operand)
; ============================================================

; CHECK: flag	r1, r0 ; encoding: [0x00,0x00,0x20,0x18]
flag r1, r0
; CHECK: flag	r0, 1000 ; encoding: [0x00,0x00,0x1f,0x18,0xe8,0x03,0x00,0x00]
flag r0, 1000

; ============================================================
; No-operand instructions
; ============================================================

; CHECK: nop ; encoding: [0xff,0xff,0xff,0x7f]
nop
; CHECK: brk ; encoding: [0x00,0xfe,0xff,0x1f]
brk
; CHECK: sleep ; encoding: [0x01,0xfe,0xff,0x1f]
sleep
; CHECK: swi ; encoding: [0x02,0xfe,0xff,0x1f]
swi
