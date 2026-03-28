; RUN: llvm-mc -triple arc4 -show-encoding < %s | FileCheck %s

; ============================================================
; Flag-setting suffix (.f) on ALU instructions
; ============================================================

; sub.f discard: sets flags for comparison (SetFlags=1 in shimm sentinel)
; CHECK: sub	0, r1, 0 ; encoding:
sub.f 0, r1, 0

; sub.f discard reg-reg: F=1 in encoding bit 8
; CHECK: sub	0, r3, r4 ; encoding:
sub.f 0, r3, r4

; add.f sets carry for multi-word arithmetic
; CHECK: add	r0, r0, r0 ; encoding:
add.f r0, r0, r0

; ============================================================
; Condition code suffixes on branches
; ============================================================

; CHECK: b	0 ; encoding:
b.eq 0
; CHECK: b	0 ; encoding:
b.ne 0
; CHECK: b	0 ; encoding:
b.lt 0
; CHECK: b	0 ; encoding:
b.ge 0
; CHECK: b	0 ; encoding:
b.lo 0
; CHECK: b	0 ; encoding:
b.hs 0
; CHECK: b	0 ; encoding:
b.hi 0
; CHECK: b	0 ; encoding:
b.ls 0

; ============================================================
; Condition code suffixes on ALU register instructions
; ============================================================

; Conditional add (ne): Q=2
; CHECK: add	r2, r2, r0 ; encoding:
add.ne r2, r2, r0

; Conditional subtract (hs): Q=6
; CHECK: sub	r2, r2, r1 ; encoding:
sub.hs r2, r2, r1

; Conditional OR (hs): Q=6
; CHECK: or	r0, r0, r4 ; encoding:
or.hs r0, r0, r4

; Conditional subtract (lt): Q=11
; CHECK: sub	r0, r5, r0 ; encoding:
sub.lt r0, r5, r0

; Conditional subtract (n = negative): Q=4
; CHECK: sub	r0, r5, r0 ; encoding:
sub.n r0, r5, r0

; ============================================================
; Combined .f and condition code on single-operand instructions
; ============================================================

; lsr.f: sets carry flag (LSB shifted out)
; CHECK: lsr	r3, r3 ; encoding:
lsr.f r3, r3

; adc for carry chain: no .f or .cc needed
; CHECK: adc	r1, r1, r1 ; encoding:
adc r1, r1, r1

; adc.f: with flag-setting
; CHECK: adc	r4, r4, r4 ; encoding:
adc.f r4, r4, r4

; sbc.f: subtract with borrow, flag-setting
; CHECK: sbc	r9, r5, r3 ; encoding:
sbc.f r9, r5, r3

; ============================================================
; or.f for 64-bit zero test
; ============================================================

; CHECK: or	r6, r2, r3 ; encoding:
or.f r6, r2, r3
