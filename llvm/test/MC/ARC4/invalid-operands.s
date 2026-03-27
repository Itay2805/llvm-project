; RUN: not llvm-mc -triple arc4 < %s 2>&1 | FileCheck %s

; Invalid mnemonic
; CHECK: error: invalid instruction mnemonic
foo r1, r2, r3

; Invalid: store without brackets is not a valid instruction
; CHECK: error:
st r1, r2

; Invalid: byte store with wrong operands
; CHECK: error:
stb r1, r2, r3
