; RUN: not llvm-mc -triple=arc4-unknown-elf %s 2>&1 | FileCheck %s

; Negative tests for load/store modifier validation.

; --- .x on store: sign-extend only valid on loads ---
; CHECK: error: sign-extend (.x) not allowed on store instructions
st.x r0, [r1, 4]

; --- .x on store shimm form ---
; CHECK: error: sign-extend (.x) not allowed on store instructions
st.x 5, [r1, 5]

; --- .a on limm base load (no writeback with limm base) ---
; CHECK: error: address writeback (.a) not allowed with non-register base
ld.a r0, [1000]

; --- .a on limm+reg load (LD0_lr has limm base) ---
; CHECK: error: address writeback (.a) not allowed with non-register base
ld.a r0, [1000, r1]

; --- .a on shimm+shimm load (LD1_ss has no register base) ---
; CHECK: error: address writeback (.a) not allowed with non-register base
ld.a r0, [100]

; --- .a on store with shimm base ---
; CHECK: error: address writeback (.a) not allowed with non-register base
st.a r0, [5, 5]

; --- .a on store to limm address ---
; CHECK: error: address writeback (.a) not allowed with non-register base
st.a r0, [1000]

; --- .x on ALU instruction ---
; CHECK: error: sign-extend (.x) only valid on load instructions
add.x r0, r1, r2

; --- .a on ALU instruction ---
; CHECK: error: address writeback (.a) only valid on load/store instructions
add.a r0, r1, r2

; --- .di on ALU instruction ---
; CHECK: error: cache bypass (.di) only valid on load/store instructions
add.di r0, r1, r2

; --- .x on branch ---
; CHECK: error: sign-extend (.x) only valid on load instructions
b.x 100

; --- .a on branch ---
; CHECK: error: address writeback (.a) only valid on load/store instructions
b.a 100
