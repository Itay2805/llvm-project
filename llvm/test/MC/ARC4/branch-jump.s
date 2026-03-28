; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Branch and jump instruction encoding tests

; --- Unconditional branch: positive offset ---
; CHECK: b	100                     ; encoding: [0x00,0x32,0x00,0x20]
b 100

; --- Unconditional branch: zero offset ---
; CHECK: b	0                       ; encoding: [0x00,0x00,0x00,0x20]
b 0

; --- Unconditional branch: negative offset ---
; CHECK: b	-4                      ; encoding: [0x00,0xfe,0xff,0x27]
b -4

; --- Register jump: bracket syntax ---
; CHECK: j	[r5]                    ; encoding: [0x00,0x80,0x02,0x38]
j [r5]

; --- Register jump: bare register (alias for bracket form) ---
; CHECK: j	[r5]                    ; encoding: [0x00,0x80,0x02,0x38]
j r5

; --- Jump and link: register (defaults to .jd) ---
; CHECK: jl	[r5]                    ; encoding: [0x40,0x82,0x02,0x38]
jl [r5]

; --- Jump and link: blink register (r31, defaults to .jd) ---
; CHECK: jl	[blink]                 ; encoding: [0x40,0x82,0x0f,0x38]
jl [r31]

; --- Flag: register operand ---
; CHECK: flag	r1                      ; encoding: [0x00,0x80,0xa0,0x1f]
flag r1

; --- Flag: shimm operand ---
; CHECK: flag	5                       ; encoding: [0x05,0x80,0xbf,0x1f]
flag 5

; --- Flag: limm operand (8-byte encoding) ---
; CHECK: flag	1000                    ; encoding: [0x00,0x00,0xbf,0x1f,0xe8,0x03,0x00,0x00]
flag 1000
