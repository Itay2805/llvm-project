; RUN: not llvm-mc -triple=arc4-unknown-elf %s 2>&1 | FileCheck %s

; Negative tests: instructions that must produce assembler errors.

; --- ALU with too few operands ---
; CHECK: error: invalid operand for instruction
add r0, r1

; --- ALU with too many operands ---
; CHECK: error: invalid operand for instruction
add r0, r1, r2, r3

; --- Load without memory brackets ---
; CHECK: error: invalid operand for instruction
ld r0, r1

; --- Store without source operand ---
; CHECK: error: invalid operand for instruction
st [r1, 4]

; --- Branch with memory operand (branch takes immediate, not register) ---
; CHECK: error: invalid operand for instruction
b [r5]

; --- Invalid register name ---
; CHECK: error: invalid operand for instruction
add r99, r1, r2

; --- nop with operand (nop takes no operands) ---
; CHECK: error: invalid operand for instruction
nop r0

; --- flag without operand ---
; CHECK: error: invalid operand for instruction
flag

; --- Load with three operands inside brackets ---
; CHECK: error: expected ']'
ld r0, [r1, r2, r3]

; --- Store reg, [limm] (single-limm address not supported for store) ---
; CHECK: error: invalid operand for instruction
st r0, [1000]

; --- Store reg+reg form (stores only support shimm/limm offset, not reg+reg) ---
; CHECK: error: invalid operand for instruction
st r0, [r1, r2]
