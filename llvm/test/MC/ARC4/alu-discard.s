; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; Tests for discard (result=0) variants.
;
; NOTE: Discard variants like "add 0, r1, r2" are defined in the instruction
; tablegen (ARC4InstrInfo.td) as _0rr, _0rs, _0sr, _0ss, _0rl, _0lr forms,
; but the assembler currently cannot match them because the literal "0" in the
; asm string is parsed as an immediate operand rather than a literal token.
;
; Only the "0ss" (discard + shimm+shimm) forms that get folded by the parser
; into a simpler form are potentially reachable, but even those currently fail.
;
; The discard forms ARE reachable through the compiler backend (pattern matching
; and instruction selection), just not from assembly syntax.
;
; For now, we test the related rss (shimm+shimm) form which works:

; CHECK: add	r0, 5, 5                ; encoding: [0x05,0xfe,0x1f,0x40]
add r0, 5, 5

; CHECK: sub	r0, 3, 3                ; encoding: [0x03,0xfe,0x1f,0x50]
sub r0, 3, 3

; CHECK: mov	r0, 7                   ; encoding: [0x07,0xfe,0x1f,0x60]
and r0, 7, 7

; CHECK: or	r0, 10, 10              ; encoding: [0x0a,0xfe,0x1f,0x68]
or r0, 10, 10
