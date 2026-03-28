; RUN: llc -march=arc4 < %s | FileCheck %s

; === Return values ===

define i32 @ret_const() {
; CHECK-LABEL: ret_const:
; CHECK: mov r0, 42
; CHECK: j [blink]
  ret i32 42
}

define i32 @ret_arg(i32 %a) {
; CHECK-LABEL: ret_arg:
; CHECK-NOT: mov
; CHECK: j [blink]
  ret i32 %a
}

; === Register-register ALU ===

define i32 @add_rr(i32 %a, i32 %b) {
; CHECK-LABEL: add_rr:
; CHECK: add r0, r0, r1
  %c = add i32 %a, %b
  ret i32 %c
}

define i32 @sub_rr(i32 %a, i32 %b) {
; CHECK-LABEL: sub_rr:
; CHECK: sub r0, r0, r1
  %c = sub i32 %a, %b
  ret i32 %c
}

define i32 @and_rr(i32 %a, i32 %b) {
; CHECK-LABEL: and_rr:
; CHECK: and r0, r0, r1
  %c = and i32 %a, %b
  ret i32 %c
}

define i32 @or_rr(i32 %a, i32 %b) {
; CHECK-LABEL: or_rr:
; CHECK: or r0, r0, r1
  %c = or i32 %a, %b
  ret i32 %c
}

define i32 @xor_rr(i32 %a, i32 %b) {
; CHECK-LABEL: xor_rr:
; CHECK: xor r0, r0, r1
  %c = xor i32 %a, %b
  ret i32 %c
}

; === ALU with small constant (shimm) ===

define i32 @add_imm(i32 %a) {
; CHECK-LABEL: add_imm:
; CHECK: add r0, r0, r{{[0-9]+}}
  %c = add i32 %a, 5
  ret i32 %c
}

define i32 @sub_imm(i32 %a) {
; CHECK-LABEL: sub_imm:
; CHECK: add r0, r0, r{{[0-9]+}}
  %c = sub i32 %a, 10
  ret i32 %c
}

define i32 @and_imm(i32 %a) {
; CHECK-LABEL: and_imm:
; CHECK: and r0, r0, r{{[0-9]+}}
  %c = and i32 %a, 255
  ret i32 %c
}

; === Chained operations ===

define i32 @chain_add_sub(i32 %a, i32 %b, i32 %c) {
; CHECK-LABEL: chain_add_sub:
; CHECK: add
; CHECK: sub r0
  %x = add i32 %a, %b
  %y = sub i32 %x, %c
  ret i32 %y
}

define i32 @triple_add(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK-LABEL: triple_add:
; CHECK: add
; CHECK: add
; CHECK: add r0
  %x = add i32 %a, %b
  %y = add i32 %c, %d
  %z = add i32 %x, %y
  ret i32 %z
}

; === Zero/sign extend ===

define i32 @zext_i8(i8 %a) {
; CHECK-LABEL: zext_i8:
; CHECK: and r0, r0, r{{[0-9]+}}
  %b = zext i8 %a to i32
  ret i32 %b
}

define i32 @zext_i16(i16 %a) {
; CHECK-LABEL: zext_i16:
  %b = zext i16 %a to i32
  ret i32 %b
}

; === Negate ===

define i32 @negate(i32 %a) {
; CHECK-LABEL: negate:
; CHECK: mov r{{[0-9]+}}, 0
; CHECK: sub r0, r{{[0-9]+}}, r0
  %c = sub i32 0, %a
  ret i32 %c
}

; === Identity ops (should be no-ops or moves) ===

define i32 @or_zero(i32 %a) {
; CHECK-LABEL: or_zero:
; CHECK: j [blink]
  %c = or i32 %a, 0
  ret i32 %c
}

define i32 @and_neg1(i32 %a) {
; CHECK-LABEL: and_neg1:
; CHECK: j [blink]
  %c = and i32 %a, -1
  ret i32 %c
}

define i32 @xor_zero(i32 %a) {
; CHECK-LABEL: xor_zero:
; CHECK: j [blink]
  %c = xor i32 %a, 0
  ret i32 %c
}
