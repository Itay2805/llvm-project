; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Byte and halfword load/store size variant encoding tests

; --- Byte load: reg+reg (opcode 0, z=01) ---
; CHECK: ldb	r0, [r1, r2]            ; encoding: [0x02,0x84,0x00,0x00]
ldb r0, [r1, r2]

; --- Halfword load: reg+reg (opcode 0, z=10) ---
; CHECK: ldw	r0, [r1, r2]            ; encoding: [0x04,0x84,0x00,0x00]
ldw r0, [r1, r2]

; --- Byte load: reg+shimm (opcode 1, Z=01) ---
; CHECK: ldb	r0, [r1, 4]             ; encoding: [0x04,0x84,0x00,0x08]
ldb r0, [r1, 4]

; --- Halfword load: reg+shimm (opcode 1, Z=10) ---
; CHECK: ldw	r0, [r1, 4]             ; encoding: [0x04,0x88,0x00,0x08]
ldw r0, [r1, 4]

; --- Byte load: single reg alias (implicit offset 0) ---
; CHECK: ldb	r0, [r1]                ; encoding: [0x00,0x84,0x00,0x08]
ldb r0, [r1]

; --- Halfword load: single reg alias (implicit offset 0) ---
; CHECK: ldw	r0, [r1]                ; encoding: [0x00,0x88,0x00,0x08]
ldw r0, [r1]

; --- Byte store: reg+shimm (opcode 2, y=01) ---
; CHECK: stb	r0, [r1, 4]             ; encoding: [0x04,0x80,0x40,0x10]
stb r0, [r1, 4]

; --- Halfword store: reg+shimm (opcode 2, y=10) ---
; CHECK: stw	r0, [r1, 4]             ; encoding: [0x04,0x80,0x80,0x10]
stw r0, [r1, 4]

; --- Byte store: single reg alias ---
; CHECK: stb	r0, [r1]                ; encoding: [0x00,0x80,0x40,0x10]
stb r0, [r1]

; --- Halfword store: single reg alias ---
; CHECK: stw	r0, [r1]                ; encoding: [0x00,0x80,0x80,0x10]
stw r0, [r1]

; --- Word load (baseline, z=00) for comparison ---
; CHECK: ld	r0, [r1, r2]            ; encoding: [0x00,0x84,0x00,0x00]
ld r0, [r1, r2]

; --- Word store (baseline, y=00) for comparison ---
; CHECK: st	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x10]
st r0, [r1, 4]
