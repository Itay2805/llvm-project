; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for ALL store variants.

; ============================================================
; Word stores (st)
; ============================================================

; reg, [reg]
; CHECK: st	r0, [r1]                ; encoding: [0x00,0x80,0x00,0x10]
st r0, [r1]

; reg, [reg, shimm]
; CHECK: st	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x10]
st r0, [r1, 4]

; reg, [shimm, shimm] (both must match -- uses shimm encoding, 4 bytes)
; CHECK: st	r0, [5]                 ; encoding: [0x05,0x80,0x1f,0x10]
st r0, [5, 5]

; shimm, [reg, shimm] (source is shimm -- uses shimm encoding, 4 bytes)
; CHECK: st	5, [r1, 5]              ; encoding: [0x05,0xfe,0x00,0x10]
st 5, [r1, 5]

; limm, [reg, shimm] (source is limm)
; CHECK: st	1000, [r1, 4]           ; encoding: [0x04,0xfc,0x00,0x10,0xe8,0x03,0x00,0x00]
st 1000, [r1, 4]

; shimm, [reg, shimm] with different offset
; CHECK: st	5, [r1, 4]              ; encoding: [0x04,0xfc,0x00,0x10,0x05,0x00,0x00,0x00]
st 5, [r1, 4]

; ============================================================
; Byte stores (stb)
; ============================================================

; CHECK: stb	r0, [r1]                ; encoding: [0x00,0x80,0x40,0x10]
stb r0, [r1]

; CHECK: stb	r0, [r1, 4]             ; encoding: [0x04,0x80,0x40,0x10]
stb r0, [r1, 4]

; ============================================================
; Halfword stores (stw)
; ============================================================

; CHECK: stw	r0, [r1]                ; encoding: [0x00,0x80,0x80,0x10]
stw r0, [r1]

; CHECK: stw	r0, [r1, 4]             ; encoding: [0x04,0x80,0x80,0x10]
stw r0, [r1, 4]
