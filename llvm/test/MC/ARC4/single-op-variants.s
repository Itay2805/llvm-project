; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Single-operand (SOP) instruction variants

; --- asr: arithmetic shift right ---
; CHECK: asr	r1, r2                  ; encoding: [0x00,0x02,0x21,0x18]
asr r1, r2

; asr with shimm source
; CHECK: asr	r1, 5                   ; encoding: [0x05,0x82,0x3f,0x18]
asr r1, 5

; --- lsr: logical shift right ---
; CHECK: lsr	r1, r2                  ; encoding: [0x00,0x04,0x21,0x18]
lsr r1, r2

; --- ror: rotate right ---
; CHECK: ror	r1, r2                  ; encoding: [0x00,0x06,0x21,0x18]
ror r1, r2

; --- rrc: rotate right through carry ---
; CHECK: rrc	r1, r2                  ; encoding: [0x00,0x08,0x21,0x18]
rrc r1, r2

; --- sexb: sign-extend byte ---
; CHECK: sexb	r1, r2                  ; encoding: [0x00,0x0a,0x21,0x18]
sexb r1, r2

; --- sexw: sign-extend word ---
; CHECK: sexw	r1, r2                  ; encoding: [0x00,0x0c,0x21,0x18]
sexw r1, r2

; --- extb: zero-extend byte ---
; CHECK: extb	r1, r2                  ; encoding: [0x00,0x0e,0x21,0x18]
extb r1, r2

; --- extw: zero-extend halfword ---
; CHECK: extw	r1, r2                  ; encoding: [0x00,0x10,0x21,0x18]
extw r1, r2

; --- asl: arithmetic/logical shift left (SOP form) ---
; CHECK: asl	r1, r2                  ; encoding: [0x00,0x00,0x21,0x18]
asl r1, r2
