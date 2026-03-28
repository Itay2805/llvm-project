; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for ALL load variants across word, byte, and halfword sizes.

; ============================================================
; Word loads (ld)
; ============================================================

; reg+reg
; CHECK: ld	r0, [r1, r2]            ; encoding: [0x00,0x84,0x00,0x00]
ld r0, [r1, r2]

; reg+shimm
; CHECK: ld	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x08]
ld r0, [r1, 4]

; reg+limm
; CHECK: ld	r0, [r1, 1000]          ; encoding: [0x00,0xfc,0x00,0x00,0xe8,0x03,0x00,0x00]
ld r0, [r1, 1000]

; limm+reg
; CHECK: ld	r0, [1000, r1]          ; encoding: [0x00,0x02,0x1f,0x00,0xe8,0x03,0x00,0x00]
ld r0, [1000, r1]

; limm absolute (single operand)
; CHECK: ld	r0, [1000]              ; encoding: [0x00,0x00,0x1f,0x08,0xe8,0x03,0x00,0x00]
ld r0, [1000]

; ============================================================
; Byte loads (ldb)
; ============================================================

; reg+reg
; CHECK: ldb	r0, [r1, r2]            ; encoding: [0x02,0x84,0x00,0x00]
ldb r0, [r1, r2]

; reg+shimm
; CHECK: ldb	r0, [r1, 4]             ; encoding: [0x04,0x84,0x00,0x08]
ldb r0, [r1, 4]

; reg+limm
; CHECK: ldb	r0, [r1, 1000]          ; encoding: [0x02,0xfc,0x00,0x00,0xe8,0x03,0x00,0x00]
ldb r0, [r1, 1000]

; limm+reg
; CHECK: ldb	r0, [1000, r1]          ; encoding: [0x02,0x02,0x1f,0x00,0xe8,0x03,0x00,0x00]
ldb r0, [1000, r1]

; limm absolute
; CHECK: ldb	r0, [1000]              ; encoding: [0x00,0x04,0x1f,0x08,0xe8,0x03,0x00,0x00]
ldb r0, [1000]

; ============================================================
; Halfword loads (ldw)
; ============================================================

; reg+reg
; CHECK: ldw	r0, [r1, r2]            ; encoding: [0x04,0x84,0x00,0x00]
ldw r0, [r1, r2]

; reg+shimm
; CHECK: ldw	r0, [r1, 4]             ; encoding: [0x04,0x88,0x00,0x08]
ldw r0, [r1, 4]

; reg+limm
; CHECK: ldw	r0, [r1, 1000]          ; encoding: [0x04,0xfc,0x00,0x00,0xe8,0x03,0x00,0x00]
ldw r0, [r1, 1000]

; limm+reg
; CHECK: ldw	r0, [1000, r1]          ; encoding: [0x04,0x02,0x1f,0x00,0xe8,0x03,0x00,0x00]
ldw r0, [1000, r1]

; limm absolute
; CHECK: ldw	r0, [1000]              ; encoding: [0x00,0x08,0x1f,0x08,0xe8,0x03,0x00,0x00]
ldw r0, [1000]

; ============================================================
; Load with zero offset (base register only)
; ============================================================

; CHECK: ld	r0, [r1]                ; encoding: [0x00,0x80,0x00,0x08]
ld r0, [r1]
; CHECK: ldb	r0, [r1]                ; encoding: [0x00,0x84,0x00,0x08]
ldb r0, [r1]
; CHECK: ldw	r0, [r1]                ; encoding: [0x00,0x88,0x00,0x08]
ldw r0, [r1]
