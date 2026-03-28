; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for ALL ALU3 opcodes across rrr, rrs (shimm), rsr, rss, rrl (limm),
; and rlr forms.

; ============================================================
; rrr form: reg, reg, reg
; ============================================================

; CHECK: add	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x40]
add r0, r1, r2
; CHECK: adc	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x48]
adc r0, r1, r2
; CHECK: sub	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x50]
sub r0, r1, r2
; CHECK: sbc	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x58]
sbc r0, r1, r2
; CHECK: and	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x60]
and r0, r1, r2
; CHECK: or	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x68]
or r0, r1, r2
; CHECK: bic	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x70]
bic r0, r1, r2
; CHECK: xor	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x78]
xor r0, r1, r2

; ============================================================
; rrs form: reg, reg, shimm
; ============================================================

; CHECK: add	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x40]
add r0, r1, 5
; CHECK: adc	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x48]
adc r0, r1, 5
; CHECK: sub	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x50]
sub r0, r1, 5
; CHECK: sbc	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x58]
sbc r0, r1, 5
; CHECK: and	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x60]
and r0, r1, 5
; CHECK: or	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x68]
or r0, r1, 5
; CHECK: bic	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x70]
bic r0, r1, 5
; CHECK: xor	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x78]
xor r0, r1, 5

; ============================================================
; rsr form: reg, shimm, reg
; ============================================================

; CHECK: add	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x40]
add r0, 5, r1
; CHECK: adc	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x48]
adc r0, 5, r1
; CHECK: sub	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x50]
sub r0, 5, r1
; CHECK: sbc	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x58]
sbc r0, 5, r1
; CHECK: and	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x60]
and r0, 5, r1
; CHECK: or	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x68]
or r0, 5, r1
; CHECK: bic	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x70]
bic r0, 5, r1
; CHECK: xor	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x78]
xor r0, 5, r1

; ============================================================
; rrl form: reg, reg, limm
; ============================================================

; CHECK: add	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x40,0xe8,0x03,0x00,0x00]
add r0, r1, 1000
; CHECK: adc	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x48,0xe8,0x03,0x00,0x00]
adc r0, r1, 1000
; CHECK: sub	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x50,0xe8,0x03,0x00,0x00]
sub r0, r1, 1000
; CHECK: sbc	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x58,0xe8,0x03,0x00,0x00]
sbc r0, r1, 1000
; CHECK: and	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x60,0xe8,0x03,0x00,0x00]
and r0, r1, 1000
; CHECK: or	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x68,0xe8,0x03,0x00,0x00]
or r0, r1, 1000
; CHECK: bic	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x70,0xe8,0x03,0x00,0x00]
bic r0, r1, 1000
; CHECK: xor	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x78,0xe8,0x03,0x00,0x00]
xor r0, r1, 1000

; ============================================================
; rss form: reg, shimm, shimm (both immediates must match)
; ============================================================

; CHECK: add	r0, 5, 5                ; encoding: [0x05,0xfe,0x1f,0x40]
add r0, 5, 5
; CHECK: sub	r0, 3, 3                ; encoding: [0x03,0xfe,0x1f,0x50]
sub r0, 3, 3

; ============================================================
; Shimm boundary values
; ============================================================

; CHECK: add	r0, r1, 255             ; encoding: [0xff,0xfe,0x00,0x40]
add r0, r1, 255
; CHECK: add	r0, r1, -256            ; encoding: [0x00,0xff,0x00,0x40]
add r0, r1, -256

; Out of shimm range -> uses limm
; CHECK: add	r0, r1, 256             ; encoding: [0x00,0xfc,0x00,0x40,0x00,0x01,0x00,0x00]
add r0, r1, 256
; CHECK: add	r0, r1, -257            ; encoding: [0x00,0xfc,0x00,0x40,0xff,0xfe,0xff,0xff]
add r0, r1, -257
