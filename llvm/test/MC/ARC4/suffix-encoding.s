; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for .f (flag), .q (condition code), delay slot, and rss form encoding

; ============================================================
; .f flag encoding
; ============================================================

; --- .f with rrr form: bit 8 set ---
; CHECK: add	r0, r1, r2              ; encoding: [0x00,0x85,0x00,0x40]
add.f r0, r1, r2

; --- Without .f: bit 8 clear (baseline) ---
; CHECK: add	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x40]
add r0, r1, r2

; --- .f with shimm (rrs form): sentinel 63->61 in C field ---
; CHECK: add	r0, r1, 5               ; encoding: [0x05,0xfa,0x00,0x40]
add.f r0, r1, 5

; --- Without .f shimm (baseline): sentinel 63 in C field ---
; CHECK: add	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x40]
add r0, r1, 5

; --- .f with rsr form: sentinel 63->61 in B field ---
; CHECK: add	r0, 5, r1               ; encoding: [0x05,0x82,0x1e,0x40]
add.f r0, 5, r1

; --- .f with sub ---
; CHECK: sub	r0, r1, r2              ; encoding: [0x00,0x85,0x00,0x50]
sub.f r0, r1, r2

; ============================================================
; Condition code encoding
; ============================================================

; --- .eq (cc=1) in bits [4:0] ---
; CHECK: add	r0, r1, r2              ; encoding: [0x01,0x84,0x00,0x40]
add.eq r0, r1, r2

; --- .ne (cc=2) ---
; CHECK: add	r0, r1, r2              ; encoding: [0x02,0x84,0x00,0x40]
add.ne r0, r1, r2

; --- .gt (cc=9) ---
; CHECK: add	r0, r1, r2              ; encoding: [0x09,0x84,0x00,0x40]
add.gt r0, r1, r2

; --- .ne on branch: bits [4:0]=2 ---
; CHECK: b	100                     ; encoding: [0x02,0x32,0x00,0x20]
b.ne 100

; --- .eq on branch ---
; CHECK: b	100                     ; encoding: [0x01,0x32,0x00,0x20]
b.eq 100

; ============================================================
; Delay slot encoding
; ============================================================

; --- .d (delay=1) on branch: bits [6:5]=01 ---
; CHECK: b	100                     ; encoding: [0x20,0x32,0x00,0x20]
b.d 100

; --- .jd (delay=2) on jump: bits [6:5]=10 ---
; CHECK: j	r5                      ; encoding: [0x40,0x80,0x02,0x38]
j.jd [r5]

; --- .nd (delay=0, default) on branch: bits [6:5]=00 ---
; CHECK: b	100                     ; encoding: [0x00,0x32,0x00,0x20]
b.nd 100

; --- .d on bl (branch and link) ---
; CHECK: bl	100                     ; encoding: [0x20,0x32,0x00,0x28]
bl.d 100

; ============================================================
; rss form: equal immediate operands
; ============================================================

; --- add r0, 5, 5 -> rss form ---
; CHECK: add	r0, 5, 5                ; encoding: [0x05,0xfe,0x1f,0x40]
add r0, 5, 5

; --- sub r0, 3, 3 -> rss form ---
; CHECK: sub	r0, 3, 3                ; encoding: [0x03,0xfe,0x1f,0x50]
sub r0, 3, 3

; ============================================================
; Combined suffixes
; ============================================================

; --- .eq.f: condition code AND flag ---
; CHECK: add	r0, r1, r2              ; encoding: [0x01,0x85,0x00,0x40]
add.eq.f r0, r1, r2

; --- .f.eq: order should not matter ---
; CHECK: add	r0, r1, r2              ; encoding: [0x01,0x85,0x00,0x40]
add.f.eq r0, r1, r2

; --- .ne.d on branch: condition AND delay ---
; CHECK: b	100                     ; encoding: [0x22,0x32,0x00,0x20]
b.ne.d 100

; --- .d.ne on branch: order should not matter ---
; CHECK: b	100                     ; encoding: [0x22,0x32,0x00,0x20]
b.d.ne 100
