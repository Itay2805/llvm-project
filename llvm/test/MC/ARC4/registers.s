; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Named registers
; CHECK: add	sp, fp, gp              ; encoding: [0x00,0xb4,0x8d,0x43]
add sp, fp, gp

; CHECK: add	r0, blink, r1           ; encoding: [0x00,0x82,0x0f,0x40]
add r0, blink, r1

; Numbered aliases for named registers print as named registers
; CHECK: add	sp, fp, gp              ; encoding: [0x00,0xb4,0x8d,0x43]
add r28, r27, r26
