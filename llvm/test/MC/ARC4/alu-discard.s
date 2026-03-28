; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for discard (result=0) variants.
; Without .f: A = sentinel 63 (SHIMM, no flag update)
; With .f:    A = sentinel 61 (SHIMM_UPDATE, flag update)

; ============================================================
; ALU3 discard: 0rr form (reg, reg) — no .f
; ============================================================

; CHECK: add	0, r1, r2               ; encoding: [0x00,0x84,0xe0,0x47]
add 0, r1, r2
; CHECK: sub	0, r1, r2               ; encoding: [0x00,0x84,0xe0,0x57]
sub 0, r1, r2
; CHECK: and	0, r1, r2               ; encoding: [0x00,0x84,0xe0,0x67]
and 0, r1, r2
; CHECK: or	0, r1, r2               ; encoding: [0x00,0x84,0xe0,0x6f]
or 0, r1, r2
; CHECK: xor	0, r1, r2               ; encoding: [0x00,0x84,0xe0,0x7f]
xor 0, r1, r2
; CHECK: bic	0, r1, r2               ; encoding: [0x00,0x84,0xe0,0x77]
bic 0, r1, r2
; CHECK: adc	0, r1, r2               ; encoding: [0x00,0x84,0xe0,0x4f]
adc 0, r1, r2
; CHECK: sbc	0, r1, r2               ; encoding: [0x00,0x84,0xe0,0x5f]
sbc 0, r1, r2

; ============================================================
; ALU3 discard with .f — A flips to sentinel 61
; ============================================================

; CHECK: add	0, r1, r2               ; encoding: [0x00,0x85,0xa0,0x47]
add.f 0, r1, r2
; sub.f 0, r3, r4 prints as "cmp r3, r4" because the cmp alias matches
; CHECK: cmp	r3, r4                  ; encoding: [0x00,0x89,0xa1,0x57]
sub.f 0, r3, r4

; ============================================================
; ALU3 discard: shimm variants
; ============================================================

; CHECK: add	0, r1, 5                ; encoding: [0x05,0xfe,0xe0,0x47]
add 0, r1, 5
; CHECK: add	0, 5, r1                ; encoding: [0x05,0x82,0xff,0x47]
add 0, 5, r1
; CHECK: add	0, 5, 5                 ; encoding: [0x05,0xfe,0xff,0x47]
add 0, 5, 5

; ============================================================
; ALU3 discard: limm variants
; ============================================================

; CHECK: add	0, r1, 1000             ; encoding: [0x00,0xfc,0xe0,0x47,0xe8,0x03,0x00,0x00]
add 0, r1, 1000
; CHECK: add	0, 1000, r1             ; encoding: [0x00,0x02,0xff,0x47,0xe8,0x03,0x00,0x00]
add 0, 1000, r1

; ============================================================
; SOP discard forms
; ============================================================

; CHECK: asr	0, r1                   ; encoding: [0x00,0x82,0xe0,0x1f]
asr 0, r1
; CHECK: asr	0, 5                    ; encoding: [0x05,0x82,0xff,0x1f]
asr 0, 5
; CHECK: asr	0, 1000                 ; encoding: [0x00,0x02,0xff,0x1f,0xe8,0x03,0x00,0x00]
asr 0, 1000

; SOP discard with .f
; CHECK: asr	0, r1                   ; encoding: [0x00,0x83,0xa0,0x1f]
asr.f 0, r1

; ============================================================
; rss form (non-discard, shimm+shimm): still works
; ============================================================

; CHECK: add	r0, 5, 5                ; encoding: [0x05,0xfe,0x1f,0x40]
add r0, 5, 5
; CHECK: sub	r0, 3, 3                ; encoding: [0x03,0xfe,0x1f,0x50]
sub r0, 3, 3
; CHECK: mov	r0, 7                   ; encoding: [0x07,0xfe,0x1f,0x60]
and r0, 7, 7
