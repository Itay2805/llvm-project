; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for ALL delay slot modes on branches and jumps.
; Delay slot bits [6:5]: .nd=00, .d=01, .jd=10

; ============================================================
; Delay slots on branch (b)
; ============================================================

; CHECK: b	100                     ; encoding: [0x20,0x32,0x00,0x20]
b.d 100
; CHECK: b	100                     ; encoding: [0x00,0x32,0x00,0x20]
b.nd 100
; CHECK: b	100                     ; encoding: [0x40,0x32,0x00,0x20]
b.jd 100

; ============================================================
; Delay slots on branch-and-link (bl)
; ============================================================

; CHECK: bl	100                     ; encoding: [0x20,0x32,0x00,0x28]
bl.d 100
; CHECK: bl	100                     ; encoding: [0x00,0x32,0x00,0x28]
bl.nd 100

; ============================================================
; Delay slots on jump (j)
; ============================================================

; CHECK: j	r5                      ; encoding: [0x20,0x80,0x02,0x38]
j.d [r5]
; CHECK: j	r5                      ; encoding: [0x00,0x80,0x02,0x38]
j.nd [r5]
; CHECK: j	r5                      ; encoding: [0x40,0x80,0x02,0x38]
j.jd [r5]

; ============================================================
; Delay slots on jump-and-link (jl)
; ============================================================

; CHECK: jl	r5                      ; encoding: [0x20,0x82,0x02,0x38]
jl.d [r5]
; CHECK: jl	r5                      ; encoding: [0x00,0x82,0x02,0x38]
jl.nd [r5]
; CHECK: jl	r5                      ; encoding: [0x40,0x82,0x02,0x38]
jl.jd [r5]

; ============================================================
; Combined condition code + delay slot
; ============================================================

; CHECK: b	100                     ; encoding: [0x21,0x32,0x00,0x20]
b.eq.d 100
; CHECK: b	100                     ; encoding: [0x02,0x32,0x00,0x20]
b.ne.nd 100
; CHECK: j	r5                      ; encoding: [0x41,0x80,0x02,0x38]
j.eq.jd [r5]
; CHECK: jl	r5                      ; encoding: [0x22,0x82,0x02,0x38]
jl.ne.d [r5]

; ============================================================
; Default .jd for branch-and-link and jump-and-link
; When no delay slot suffix is specified, bl and jl default to .jd
; (bits[6:5]=10). Plain b and j still default to .nd (bits[6:5]=00).
; ============================================================

; bl without suffix defaults to .jd
; CHECK: bl	1000                    ; encoding: [0x40,0xf4,0x01,0x28]
bl 1000
; jl with limm without suffix defaults to .jd
; CHECK: jl	1000                    ; encoding: [0x40,0x02,0x1f,0x38,0xe8,0x03,0x00,0x00]
jl 1000
; jl with register without suffix defaults to .jd
; CHECK: jl	[r5]                    ; encoding: [0x40,0x82,0x02,0x38]
jl [r5]
; j without suffix still defaults to .nd (not a link instruction)
; CHECK: j	1000                    ; encoding: [0x00,0x00,0x1f,0x38,0xe8,0x03,0x00,0x00]
j 1000
; b without suffix still defaults to .nd
; CHECK: b	100                     ; encoding: [0x00,0x32,0x00,0x20]
b 100
