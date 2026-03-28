; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for discard (result=0) variants.
; The literal 0 destination means "discard result" (A = sentinel 61).

; ============================================================
; ALU3 discard: 0rr form (reg, reg)
; ============================================================

; CHECK: add	0, r1, r2               ; encoding: [0x00,0x84,0xa0,0x47]
add 0, r1, r2
; CHECK: sub	0, r1, r2               ; encoding: [0x00,0x84,0xa0,0x57]
sub 0, r1, r2
; CHECK: and	0, r1, r2               ; encoding: [0x00,0x84,0xa0,0x67]
and 0, r1, r2
; CHECK: or	0, r1, r2               ; encoding: [0x00,0x84,0xa0,0x6f]
or 0, r1, r2
; CHECK: xor	0, r1, r2               ; encoding: [0x00,0x84,0xa0,0x7f]
xor 0, r1, r2
; CHECK: bic	0, r1, r2               ; encoding: [0x00,0x84,0xa0,0x77]
bic 0, r1, r2
; CHECK: adc	0, r1, r2               ; encoding: [0x00,0x84,0xa0,0x4f]
adc 0, r1, r2
; CHECK: sbc	0, r1, r2               ; encoding: [0x00,0x84,0xa0,0x5f]
sbc 0, r1, r2

; ============================================================
; ALU3 discard: 0rs form (reg, shimm)
; ============================================================

; CHECK: add	0, r1, 5                ; encoding: [0x05,0xfe,0xa0,0x47]
add 0, r1, 5

; ============================================================
; ALU3 discard: 0sr form (shimm, reg)
; ============================================================

; CHECK: add	0, 5, r1                ; encoding: [0x05,0x82,0xbf,0x47]
add 0, 5, r1

; ============================================================
; ALU3 discard: 0rl form (reg, limm)
; ============================================================

; CHECK: add	0, r1, 1000             ; encoding: [0x00,0xfc,0xa0,0x47,0xe8,0x03,0x00,0x00]
add 0, r1, 1000

; ============================================================
; ALU3 discard: 0lr form (limm, reg)
; ============================================================

; CHECK: add	0, 1000, r1             ; encoding: [0x00,0x02,0xbf,0x47,0xe8,0x03,0x00,0x00]
add 0, 1000, r1

; ============================================================
; SOP discard: 0r form (reg)
; ============================================================

; CHECK: asr	0, r1                   ; encoding: [0x00,0x82,0xa0,0x1f]
asr 0, r1

; ============================================================
; SOP discard: 0s form (shimm)
; ============================================================

; CHECK: asr	0, 5                    ; encoding: [0x05,0x82,0xbf,0x1f]
asr 0, 5

; ============================================================
; SOP discard: 0l form (limm)
; ============================================================

; CHECK: asr	0, 1000                 ; encoding: [0x00,0x02,0xbf,0x1f,0xe8,0x03,0x00,0x00]
asr 0, 1000

; ============================================================
; rss form (non-discard, shimm+shimm): still works
; ============================================================

; CHECK: add	r0, 5, 5                ; encoding: [0x05,0xfe,0x1f,0x40]
add r0, 5, 5
; CHECK: sub	r0, 3, 3                ; encoding: [0x03,0xfe,0x1f,0x50]
sub r0, 3, 3
; CHECK: mov	r0, 7                   ; encoding: [0x07,0xfe,0x1f,0x60]
and r0, 7, 7
