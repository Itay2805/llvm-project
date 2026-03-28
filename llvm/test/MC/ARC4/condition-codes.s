; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Exhaustive test of ALL 16 condition codes on ALU, branch, and jump.

; ============================================================
; All 16 condition codes on add rrr form
; Condition code in bits [4:0]
; ============================================================

; CHECK: add	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x40]
add.al r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x01,0x84,0x00,0x40]
add.eq r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x02,0x84,0x00,0x40]
add.ne r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x03,0x84,0x00,0x40]
add.pl r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x04,0x84,0x00,0x40]
add.mi r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x05,0x84,0x00,0x40]
add.cs r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x06,0x84,0x00,0x40]
add.cc r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x07,0x84,0x00,0x40]
add.vs r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x08,0x84,0x00,0x40]
add.vc r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x09,0x84,0x00,0x40]
add.gt r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x0a,0x84,0x00,0x40]
add.ge r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x0b,0x84,0x00,0x40]
add.lt r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x0c,0x84,0x00,0x40]
add.le r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x0d,0x84,0x00,0x40]
add.hi r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x0e,0x84,0x00,0x40]
add.ls r0, r1, r2
; CHECK: add	r0, r1, r2              ; encoding: [0x0f,0x84,0x00,0x40]
add.pnz r0, r1, r2

; ============================================================
; All 16 condition codes on branch
; ============================================================

; CHECK: b	100                     ; encoding: [0x00,0x32,0x00,0x20]
b.al 100
; CHECK: b	100                     ; encoding: [0x01,0x32,0x00,0x20]
b.eq 100
; CHECK: b	100                     ; encoding: [0x02,0x32,0x00,0x20]
b.ne 100
; CHECK: b	100                     ; encoding: [0x03,0x32,0x00,0x20]
b.pl 100
; CHECK: b	100                     ; encoding: [0x04,0x32,0x00,0x20]
b.mi 100
; CHECK: b	100                     ; encoding: [0x05,0x32,0x00,0x20]
b.cs 100
; CHECK: b	100                     ; encoding: [0x06,0x32,0x00,0x20]
b.cc 100
; CHECK: b	100                     ; encoding: [0x07,0x32,0x00,0x20]
b.vs 100
; CHECK: b	100                     ; encoding: [0x08,0x32,0x00,0x20]
b.vc 100
; CHECK: b	100                     ; encoding: [0x09,0x32,0x00,0x20]
b.gt 100
; CHECK: b	100                     ; encoding: [0x0a,0x32,0x00,0x20]
b.ge 100
; CHECK: b	100                     ; encoding: [0x0b,0x32,0x00,0x20]
b.lt 100
; CHECK: b	100                     ; encoding: [0x0c,0x32,0x00,0x20]
b.le 100
; CHECK: b	100                     ; encoding: [0x0d,0x32,0x00,0x20]
b.hi 100
; CHECK: b	100                     ; encoding: [0x0e,0x32,0x00,0x20]
b.ls 100
; CHECK: b	100                     ; encoding: [0x0f,0x32,0x00,0x20]
b.pnz 100

; ============================================================
; Condition codes on jump and jump-and-link
; ============================================================

; CHECK: j	r5                      ; encoding: [0x01,0x80,0x02,0x38]
j.eq [r5]
; CHECK: j	r5                      ; encoding: [0x02,0x80,0x02,0x38]
j.ne [r5]
; CHECK: j	r5                      ; encoding: [0x09,0x80,0x02,0x38]
j.gt [r5]
; CHECK: j	r5                      ; encoding: [0x0b,0x80,0x02,0x38]
j.lt [r5]
; CHECK: jl	r5                      ; encoding: [0x01,0x82,0x02,0x38]
jl.eq [r5]
; CHECK: jl	r5                      ; encoding: [0x02,0x82,0x02,0x38]
jl.ne [r5]
