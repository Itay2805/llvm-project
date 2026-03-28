; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Load and store instruction encoding tests
; Note: ldb/ldw/stb/stw mnemonics are not implemented in ARC4; only ld/st exist.

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

; --- Load: shimm base (absolute address load) ---
; CHECK: ld	r0, [100]               ; encoding: [0x64,0x80,0x1f,0x08]
ld r0, [100]

; --- Store: reg value, reg base, shimm offset ---
; CHECK: st	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x10]
st r0, [r1, 4]

; --- Store: [reg] alias for [reg, 0] ---
; CHECK: st	r0, [r1]                ; encoding: [0x00,0x80,0x00,0x10]
st r0, [r1]

; --- Store: shimm base (absolute address store) ---
; CHECK: st	r0, [100]               ; encoding: [0x64,0x80,0x1f,0x10]
st r0, [100]
