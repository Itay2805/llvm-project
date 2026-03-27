; RUN: llvm-mc -triple arc4 -show-encoding < %s | FileCheck %s

; ============================================================
; Shimm boundary values: 9-bit signed range is -256 to 255
; Values at boundary use shimm (4-byte), beyond use limm (8-byte)
; ============================================================

; CHECK: add	r1, r2, 255 ; encoding: [0xff,0x7e,0x21,0x40]
add r1, r2, 255
; CHECK: add	r1, r2, -256 ; encoding: [0x00,0x7f,0x21,0x40]
add r1, r2, -256

; 256 is out of shimm range, must use limm (8-byte instruction)
; CHECK: add	r1, r2, 256 ; encoding: [0x00,0x7c,0x21,0x40,0x00,0x01,0x00,0x00]
add r1, r2, 256

; ============================================================
; Special/named registers: ilink1(r29), ilink2(r30), blink(r31), lp_count(r60)
; ============================================================

; CHECK: add	r29, r1, r2 ; encoding: [0x00,0x84,0xa0,0x43]
add ilink1, r1, r2
; CHECK: add	r1, r31, r2 ; encoding: [0x00,0x84,0x2f,0x40]
add r1, blink, r2
; CHECK: add	lp_count, r1, r2 ; encoding: [0x00,0x84,0x80,0x47]
add lp_count, r1, r2

; ============================================================
; Case-insensitive register names
; ============================================================

; CHECK: add	r1, r2, r3 ; encoding: [0x00,0x06,0x21,0x40]
ADD R1, R2, R3
; CHECK: ld	r1, [r2, r3] ; encoding: [0x00,0x06,0x21,0x00]
LD R1, [R2, R3]

; ============================================================
; Negative shimm offsets in load/store
; ============================================================

; CHECK: st	r1, [r2, -8] ; encoding: [0xf8,0x03,0x01,0x10]
st r1, [r2, -8]
; CHECK: ld	r1, [r2, 0] ; encoding: [0x00,0x00,0x21,0x08]
ld r1, [r2, 0]

; ============================================================
; Single-operand discard and shimm: additional mnemonics
; ============================================================

; CHECK: lsr	0, r5 ; encoding: [0x00,0x84,0xe2,0x1f]
lsr 0, r5
; CHECK: sexb	0, r3 ; encoding: [0x00,0x8a,0xe1,0x1f]
sexb 0, r3
; CHECK: lsr	r1, -10 ; encoding: [0xf6,0x85,0x3f,0x18]
lsr r1, -10
; CHECK: extb	r1, 100 ; encoding: [0x64,0x8e,0x3f,0x18]
extb r1, 100

; ============================================================
; BL and LP with labels (PC-relative fixups)
; ============================================================

bl_target:
  nop
; CHECK: bl	bl_target ; encoding: [0bA0000000,A,A,0b00101AAA]
; CHECK-NEXT: ;   fixup A - offset: 0, value: bl_target-4, kind: FK_ARC4_Branch20
  bl bl_target

; CHECK: lp	lp_end ; encoding: [0bA0000000,A,A,0b00110AAA]
; CHECK-NEXT: ;   fixup A - offset: 0, value: lp_end-4, kind: FK_ARC4_Branch20
  lp lp_end
  nop
lp_end:
  nop

; ============================================================
; Function return pattern: j [blink]
; ============================================================

; CHECK: j	[r31] ; encoding: [0x00,0x80,0x0f,0x38]
j [r31]
