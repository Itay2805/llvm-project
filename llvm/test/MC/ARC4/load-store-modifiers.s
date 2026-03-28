; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for load/store modifier encoding:
;   .x  = sign extend
;   .a  = address writeback
;   .di = cache bypass (direct/uncached)

; ============================================================
; Load opcode 0 (reg+reg): x=bit[0], w=bit[3], e=bit[5]
; ============================================================

; Baseline: no modifiers
; CHECK: ld	r0, [r1, r2]            ; encoding: [0x00,0x84,0x00,0x00]
ld r0, [r1, r2]

; .x (sign extend): bit 0 set
; CHECK: ld.x	r0, [r1, r2]            ; encoding: [0x01,0x84,0x00,0x00]
ld.x r0, [r1, r2]

; .a (writeback): bit 3 set
; CHECK: ld.a	r0, [r1, r2]            ; encoding: [0x08,0x84,0x00,0x00]
ld.a r0, [r1, r2]

; .di (cache bypass): bit 5 set
; CHECK: ld.di	r0, [r1, r2]            ; encoding: [0x20,0x84,0x00,0x00]
ld.di r0, [r1, r2]

; Combined .x.a.di: bits 0+3+5 = 0x29
; CHECK: ld.x.a.di	r0, [r1, r2]            ; encoding: [0x29,0x84,0x00,0x00]
ld.x.a.di r0, [r1, r2]

; Combined .x.a: bits 0+3 = 0x09
; CHECK: ld.x.a	r0, [r1, r2]            ; encoding: [0x09,0x84,0x00,0x00]
ld.x.a r0, [r1, r2]

; Combined .x.di: bits 0+5 = 0x21
; CHECK: ld.x.di	r0, [r1, r2]            ; encoding: [0x21,0x84,0x00,0x00]
ld.x.di r0, [r1, r2]

; Combined .a.di: bits 3+5 = 0x28
; CHECK: ld.a.di	r0, [r1, r2]            ; encoding: [0x28,0x84,0x00,0x00]
ld.a.di r0, [r1, r2]

; ============================================================
; Load opcode 0 (reg+limm): same bit layout, 8-byte encoding
; ============================================================

; .x on LD0_rl
; CHECK: ld.x	r0, [r1, 1000]          ; encoding: [0x01,0xfc,0x00,0x00,0xe8,0x03,0x00,0x00]
ld.x r0, [r1, 1000]

; .a on LD0_rl
; CHECK: ld.a	r0, [r1, 1000]          ; encoding: [0x08,0xfc,0x00,0x00,0xe8,0x03,0x00,0x00]
ld.a r0, [r1, 1000]

; .di on LD0_rl
; CHECK: ld.di	r0, [r1, 1000]          ; encoding: [0x20,0xfc,0x00,0x00,0xe8,0x03,0x00,0x00]
ld.di r0, [r1, 1000]

; ============================================================
; Load opcode 0 (limm+reg): NO .a, but .x and .di work
; ============================================================

; .x on LD0_lr
; CHECK: ld.x	r0, [1000, r1]          ; encoding: [0x01,0x02,0x1f,0x00,0xe8,0x03,0x00,0x00]
ld.x r0, [1000, r1]

; .di on LD0_lr
; CHECK: ld.di	r0, [1000, r1]          ; encoding: [0x20,0x02,0x1f,0x00,0xe8,0x03,0x00,0x00]
ld.di r0, [1000, r1]

; ============================================================
; Load opcode 1 (reg+shimm): X=bit[9], W=bit[12], E=bit[14]
; ============================================================

; Baseline
; CHECK: ld	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x08]
ld r0, [r1, 4]

; .x (sign extend): bit 9 (byte[1] bit 1)
; CHECK: ld.x	r0, [r1, 4]             ; encoding: [0x04,0x82,0x00,0x08]
ld.x r0, [r1, 4]

; .a (writeback): bit 12 (byte[1] bit 4)
; CHECK: ld.a	r0, [r1, 4]             ; encoding: [0x04,0x90,0x00,0x08]
ld.a r0, [r1, 4]

; .di (cache bypass): bit 14 (byte[1] bit 6)
; CHECK: ld.di	r0, [r1, 4]             ; encoding: [0x04,0xc0,0x00,0x08]
ld.di r0, [r1, 4]

; Combined .x.a.di: bits 9+12+14
; CHECK: ld.x.a.di	r0, [r1, 4]             ; encoding: [0x04,0xd2,0x00,0x08]
ld.x.a.di r0, [r1, 4]

; ============================================================
; Load opcode 1 (limm): NO .a, but .x and .di work
; ============================================================

; .x on LD1_l
; CHECK: ld.x	r0, [1000]              ; encoding: [0x00,0x02,0x1f,0x08,0xe8,0x03,0x00,0x00]
ld.x r0, [1000]

; .di on LD1_l
; CHECK: ld.di	r0, [1000]              ; encoding: [0x00,0x40,0x1f,0x08,0xe8,0x03,0x00,0x00]
ld.di r0, [1000]

; ============================================================
; Byte/halfword loads with modifiers
; ============================================================

; ldb.x: sign extend on byte load (z=01 in bits 2:1, x=1 in bit 0)
; CHECK: ldb.x	r0, [r1, r2]            ; encoding: [0x03,0x84,0x00,0x00]
ldb.x r0, [r1, r2]

; ldw.x: sign extend on halfword load (z=10 in bits 2:1, x=1 in bit 0)
; CHECK: ldw.x	r0, [r1, r2]            ; encoding: [0x05,0x84,0x00,0x00]
ldw.x r0, [r1, r2]

; ldb.a: writeback on byte load
; CHECK: ldb.a	r0, [r1, r2]            ; encoding: [0x0a,0x84,0x00,0x00]
ldb.a r0, [r1, r2]

; ldw.di: cache bypass on halfword load
; CHECK: ldw.di	r0, [r1, r2]            ; encoding: [0x24,0x84,0x00,0x00]
ldw.di r0, [r1, r2]

; ============================================================
; Store modifiers: v=bit[24], D=bit[26]
; ============================================================

; Baseline: no modifiers
; CHECK: st	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x10]
st r0, [r1, 4]

; .a (writeback): bit 24 (byte[3] bit 0)
; CHECK: st.a	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x11]
st.a r0, [r1, 4]

; .di (cache bypass): bit 26 (byte[3] bit 2)
; CHECK: st.di	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x14]
st.di r0, [r1, 4]

; Combined .a.di
; CHECK: st.a.di	r0, [r1, 4]             ; encoding: [0x04,0x80,0x00,0x15]
st.a.di r0, [r1, 4]

; .di on byte store
; CHECK: stb.di	r0, [r1, 4]             ; encoding: [0x04,0x80,0x40,0x14]
stb.di r0, [r1, 4]

; .a on halfword store
; CHECK: stw.a	r0, [r1, 4]             ; encoding: [0x04,0x80,0x80,0x11]
stw.a r0, [r1, 4]

; ============================================================
; Store with limm value and modifiers
; ============================================================

; .di on st limm, [reg, shimm]
; CHECK: st.di	1000, [r1, 4]           ; encoding: [0x04,0xfc,0x00,0x14,0xe8,0x03,0x00,0x00]
st.di 1000, [r1, 4]

; .a on st limm, [reg, shimm]
; CHECK: st.a	1000, [r1, 4]           ; encoding: [0x04,0xfc,0x00,0x11,0xe8,0x03,0x00,0x00]
st.a 1000, [r1, 4]

; ============================================================
; Store shimm forms with modifiers
; ============================================================

; .di on st shimm, [reg, shimm] (srs form)
; CHECK: st.di	5, [r1, 5]              ; encoding: [0x05,0xfe,0x00,0x14]
st.di 5, [r1, 5]

; .a on st shimm, [reg, shimm] (srs form)
; CHECK: st.a	5, [r1, 5]              ; encoding: [0x05,0xfe,0x00,0x11]
st.a 5, [r1, 5]

; ============================================================
; Modifier combined with size suffix
; ============================================================

; ld.b.x => ldb.x
; CHECK: ldb.x	r0, [r1, r2]            ; encoding: [0x03,0x84,0x00,0x00]
ld.b.x r0, [r1, r2]

; st.w.di => stw.di
; CHECK: stw.di	r0, [r1, 4]             ; encoding: [0x04,0x80,0x80,0x14]
st.w.di r0, [r1, 4]

; ============================================================
; Store .di on non-register base forms
; ============================================================

; .di on st reg, [limm]
; CHECK: st.di	r0, [1000, 0]           ; encoding: [0x00,0x00,0x1f,0x14,0xe8,0x03,0x00,0x00]
st.di r0, [1000]

; .di on st reg, [shimm]
; CHECK: st.di	r0, [5]                 ; encoding: [0x05,0x80,0x1f,0x14]
st.di r0, [5, 5]
