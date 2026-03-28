; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Load and store instruction encoding tests (word size)
; See size-variants.s for ldb/ldw/stb/stw tests.

; --- Load: reg+reg base (opcode-0 form) ---
; CHECK: ld	r0, [r1, r2]            ; encoding: [0x00,0x84,0x00,0x00]
ld r0, [r1, r2]

; --- Load: reg+shimm offset (opcode-1 form) ---
; CHECK: ld	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x08]
ld r0, [r1, 4]

; --- Load: [reg] alias for [reg, 0] ---
; CHECK: ld	r0, [r1]                ; encoding: [0x00,0x80,0x00,0x08]
ld r0, [r1]

; --- Load: reg+limm offset (8-byte encoding) ---
; CHECK: ld	r0, [r1, 1000]          ; encoding: [0x00,0xfc,0x00,0x00,0xe8,0x03,0x00,0x00]
ld r0, [r1, 1000]

; --- Load: limm+reg base (8-byte encoding) ---
; CHECK: ld	r0, [1000, r1]          ; encoding: [0x00,0x02,0x1f,0x00,0xe8,0x03,0x00,0x00]
ld r0, [1000, r1]

; --- Load: absolute address 100 (shimm optimization: 100/2=50, 4 bytes) ---
; CHECK: ld	r0, [100]               ; encoding: [0x32,0x80,0x1f,0x08]
ld r0, [100]

; --- Store: reg value, reg base, shimm offset ---
; CHECK: st	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x10]
st r0, [r1, 4]

; --- Store: [reg] alias for [reg, 0] ---
; CHECK: st	r0, [r1]                ; encoding: [0x00,0x80,0x00,0x10]
st r0, [r1]

; --- Store: absolute address (limm form) ---
; CHECK: st	r0, [100, 0]            ; encoding: [0x00,0x00,0x1f,0x10,0x64,0x00,0x00,0x00]
st r0, [100]
