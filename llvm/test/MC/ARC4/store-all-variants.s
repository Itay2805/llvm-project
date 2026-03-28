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

; reg, [limm] (store reg to absolute address, shimm offset = 0)
; CHECK: st	r0, [1000, 0]           ; encoding: [0x00,0x00,0x1f,0x10,0xe8,0x03,0x00,0x00]
st r0, [1000]

; shimm, [limm] (store shimm to absolute address, limm adjusted)
; "st 5, [1000]" encodes as st 5, [995, 5] where 995 = 1000 - 5
; CHECK: st	5, [995, 5]             ; encoding: [0x05,0x7e,0x1f,0x10,0xe3,0x03,0x00,0x00]
st 5, [1000]

; ============================================================
; Byte stores (stb)
; ============================================================

; CHECK: stb	r0, [r1]                ; encoding: [0x00,0x80,0x40,0x10]
stb r0, [r1]

; CHECK: stb	r0, [r1, 4]             ; encoding: [0x04,0x80,0x40,0x10]
stb r0, [r1, 4]

; stb reg, [limm]
; CHECK: stb	r0, [1000, 0]           ; encoding: [0x00,0x00,0x5f,0x10,0xe8,0x03,0x00,0x00]
stb r0, [1000]

; stb shimm, [limm]
; CHECK: stb	5, [995, 5]             ; encoding: [0x05,0x7e,0x5f,0x10,0xe3,0x03,0x00,0x00]
stb 5, [1000]

; ============================================================
; Halfword stores (stw)
; ============================================================

; CHECK: stw	r0, [r1]                ; encoding: [0x00,0x80,0x80,0x10]
stw r0, [r1]

; CHECK: stw	r0, [r1, 4]             ; encoding: [0x04,0x80,0x80,0x10]
stw r0, [r1, 4]

; stw reg, [limm]
; CHECK: stw	r0, [1000, 0]           ; encoding: [0x00,0x00,0x9f,0x10,0xe8,0x03,0x00,0x00]
stw r0, [1000]
