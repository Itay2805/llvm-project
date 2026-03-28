; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; ALU3 instruction operand variants

; --- rrr: register × register → register ---
; CHECK: add	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x40]
add r0, r1, r2

; --- rrs: shimm in C ---
; CHECK: add	r0, r1, 5               ; encoding: [0x05,0xfe,0x00,0x40]
add r0, r1, 5

; --- rsr: shimm in B ---
; CHECK: add	r0, 5, r1               ; encoding: [0x05,0x82,0x1f,0x40]
add r0, 5, r1

; --- rrl: limm in C (8-byte encoding) ---
; CHECK: add	r0, r1, 1000            ; encoding: [0x00,0xfc,0x00,0x40,0xe8,0x03,0x00,0x00]
add r0, r1, 1000

; --- rlr: limm in B (8-byte encoding) ---
; CHECK: add	r0, 1000, r1            ; encoding: [0x00,0x02,0x1f,0x40,0xe8,0x03,0x00,0x00]
add r0, 1000, r1

; --- Verify different opcodes for each ALU3 mnemonic ---

; CHECK: sub	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x50]
sub r0, r1, r2

; CHECK: and	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x60]
and r0, r1, r2

; CHECK: or	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x68]
or r0, r1, r2

; CHECK: xor	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x78]
xor r0, r1, r2

; CHECK: bic	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x70]
bic r0, r1, r2

; CHECK: adc	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x48]
adc r0, r1, r2

; CHECK: sbc	r0, r1, r2              ; encoding: [0x00,0x84,0x00,0x58]
sbc r0, r1, r2

; --- Aliases ---

; mov a, b expands to: and a, b, b
; CHECK: mov	r0, r1                  ; encoding: [0x00,0x82,0x00,0x60]
mov r0, r1

; mov a, shimm expands to: and a, shimm, shimm (rss form)
; CHECK: mov	r0, 5                   ; encoding: [0x05,0xfe,0x1f,0x60]
mov r0, 5
