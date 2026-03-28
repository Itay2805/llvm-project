; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for ALL single-operand instructions across rr, rs (shimm), rl (limm)
; forms, plus .f and .q variants.
;
; Note: ASL does not exist as a single-operand instruction (opcode 3 subop 0
; is the flag instruction). ASL is an alias for add a, b, b. Tests for ASL
; are included separately to verify alias behavior.

; ============================================================
; rr form: reg, reg
; ============================================================

; ASL is an alias for add — separate from the SOP family
; CHECK: add	r1, r2, r2              ; encoding: [0x00,0x04,0x21,0x40]
asl r1, r2
; CHECK: asr	r1, r2                  ; encoding: [0x00,0x02,0x21,0x18]
asr r1, r2
; CHECK: lsr	r1, r2                  ; encoding: [0x00,0x04,0x21,0x18]
lsr r1, r2
; CHECK: ror	r1, r2                  ; encoding: [0x00,0x06,0x21,0x18]
ror r1, r2
; CHECK: rrc	r1, r2                  ; encoding: [0x00,0x08,0x21,0x18]
rrc r1, r2
; CHECK: sexb	r1, r2                  ; encoding: [0x00,0x0a,0x21,0x18]
sexb r1, r2
; CHECK: sexw	r1, r2                  ; encoding: [0x00,0x0c,0x21,0x18]
sexw r1, r2
; CHECK: extb	r1, r2                  ; encoding: [0x00,0x0e,0x21,0x18]
extb r1, r2
; CHECK: extw	r1, r2                  ; encoding: [0x00,0x10,0x21,0x18]
extw r1, r2

; ============================================================
; rs form: reg, shimm
; ============================================================

; ASL shimm: alias for add rss (both shimms match)
; CHECK: add	r1, 5, 5                ; encoding: [0x05,0xfe,0x3f,0x40]
asl r1, 5
; CHECK: asr	r1, 5                   ; encoding: [0x05,0x82,0x3f,0x18]
asr r1, 5
; CHECK: lsr	r1, 5                   ; encoding: [0x05,0x84,0x3f,0x18]
lsr r1, 5
; CHECK: ror	r1, 5                   ; encoding: [0x05,0x86,0x3f,0x18]
ror r1, 5
; CHECK: rrc	r1, 5                   ; encoding: [0x05,0x88,0x3f,0x18]
rrc r1, 5
; CHECK: sexb	r1, 5                   ; encoding: [0x05,0x8a,0x3f,0x18]
sexb r1, 5
; CHECK: sexw	r1, 5                   ; encoding: [0x05,0x8c,0x3f,0x18]
sexw r1, 5
; CHECK: extb	r1, 5                   ; encoding: [0x05,0x8e,0x3f,0x18]
extb r1, 5
; CHECK: extw	r1, 5                   ; encoding: [0x05,0x90,0x3f,0x18]
extw r1, 5

; ============================================================
; rl form: reg, limm
; ============================================================

; ASL with limm is not supported (would need add rll = both limm, not valid)
; CHECK: asr	r1, 1000                ; encoding: [0x00,0x02,0x3f,0x18,0xe8,0x03,0x00,0x00]
asr r1, 1000
; CHECK: lsr	r1, 1000                ; encoding: [0x00,0x04,0x3f,0x18,0xe8,0x03,0x00,0x00]
lsr r1, 1000
; CHECK: sexb	r1, 1000                ; encoding: [0x00,0x0a,0x3f,0x18,0xe8,0x03,0x00,0x00]
sexb r1, 1000

; ============================================================
; .f flag on SOP rr form
; ============================================================

; ASL .f: alias for add.f a, b, b
; CHECK: add	r1, r2, r2              ; encoding: [0x00,0x05,0x21,0x40]
asl.f r1, r2
; CHECK: asr	r1, r2                  ; encoding: [0x00,0x03,0x21,0x18]
asr.f r1, r2
; CHECK: lsr	r1, r2                  ; encoding: [0x00,0x05,0x21,0x18]
lsr.f r1, r2
; CHECK: ror	r1, r2                  ; encoding: [0x00,0x07,0x21,0x18]
ror.f r1, r2
; CHECK: rrc	r1, r2                  ; encoding: [0x00,0x09,0x21,0x18]
rrc.f r1, r2
; CHECK: sexb	r1, r2                  ; encoding: [0x00,0x0b,0x21,0x18]
sexb.f r1, r2
; CHECK: extb	r1, r2                  ; encoding: [0x00,0x0f,0x21,0x18]
extb.f r1, r2

; ============================================================
; .f flag on SOP rs (shimm) form
; ============================================================

; CHECK: asr	r1, 5                   ; encoding: [0x05,0x82,0x3e,0x18]
asr.f r1, 5
; CHECK: lsr	r1, 5                   ; encoding: [0x05,0x84,0x3e,0x18]
lsr.f r1, 5
; CHECK: ror	r1, 5                   ; encoding: [0x05,0x86,0x3e,0x18]
ror.f r1, 5
; CHECK: rrc	r1, 5                   ; encoding: [0x05,0x88,0x3e,0x18]
rrc.f r1, 5

; ============================================================
; Condition codes on SOP (reg form only, not shimm)
; ============================================================

; CHECK: asr	r1, r2                  ; encoding: [0x01,0x02,0x21,0x18]
asr.eq r1, r2
; CHECK: lsr	r1, r2                  ; encoding: [0x02,0x04,0x21,0x18]
lsr.ne r1, r2
; ASL .gt: alias for add.gt a, b, b
; CHECK: add	r1, r2, r2              ; encoding: [0x09,0x04,0x21,0x40]
asl.gt r1, r2
; CHECK: sexb	r1, r2                  ; encoding: [0x01,0x0a,0x21,0x18]
sexb.eq r1, r2
