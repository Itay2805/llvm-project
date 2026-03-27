; RUN: llvm-mc -triple arc4 -show-encoding < %s | FileCheck %s

; ============================================================
; Load register+register (opcode 0x00): ld a, [b, c]
; Z/X bits encoded in lower instruction bits per spec
; ============================================================

; CHECK: ld	r1, [r2, r3] ; encoding: [0x00,0x06,0x21,0x00]
ld r1, [r2, r3]
; CHECK: ldb	r1, [r2, r3] ; encoding: [0x02,0x06,0x21,0x00]
ldb r1, [r2, r3]
; CHECK: ldw	r1, [r2, r3] ; encoding: [0x04,0x06,0x21,0x00]
ldw r1, [r2, r3]
; CHECK: ldbx	r1, [r2, r3] ; encoding: [0x03,0x06,0x21,0x00]
ldbx r1, [r2, r3]
; CHECK: ldwx	r1, [r2, r3] ; encoding: [0x05,0x06,0x21,0x00]
ldwx r1, [r2, r3]

; ============================================================
; Load register+shimm (opcode 0x01): ld a, [b, shimm]
; Z/X encoded in C field per spec
; ============================================================

; CHECK: ld	r1, [r2, 20] ; encoding: [0x14,0x00,0x21,0x08]
ld r1, [r2, 20]
; CHECK: ldb	r1, [r2, 10] ; encoding: [0x0a,0x04,0x21,0x08]
ldb r1, [r2, 10]

; ============================================================
; Load absolute (opcode 0x00 with B=C=62): ld a, [limm]
; ============================================================

; CHECK: ld	r1, [1000] ; encoding: [0x00,0x7c,0x3f,0x00,0xe8,0x03,0x00,0x00]
ld r1, [1000]

; ============================================================
; Store with offset: st c, [b, shimm]
; Per spec: A field encodes {Di,0,Aw,ZZ,0} flags
; ============================================================

; CHECK: st	r1, [r2, 0] ; encoding: [0x00,0x02,0x01,0x10]
st r1, [r2, 0]
; CHECK: st	r1, [r2, 10] ; encoding: [0x0a,0x02,0x01,0x10]
st r1, [r2, 10]
; CHECK: stb	r1, [r2, 5] ; encoding: [0x05,0x02,0x41,0x10]
stb r1, [r2, 5]
; CHECK: stw	r1, [r2, 8] ; encoding: [0x08,0x02,0x81,0x10]
stw r1, [r2, 8]

; ============================================================
; Store without offset: st c, [b]  (shorthand for offset=0)
; ============================================================

; CHECK: st	r1, [r2] ; encoding: [0x00,0x02,0x01,0x10]
st r1, [r2]
; CHECK: stb	r1, [r2] ; encoding: [0x00,0x02,0x41,0x10]
stb r1, [r2]
; CHECK: stw	r1, [r2] ; encoding: [0x00,0x02,0x81,0x10]
stw r1, [r2]

; ============================================================
; Store absolute: st c, [limm]
; ============================================================

; CHECK: st	r1, [1000] ; encoding: [0x00,0x02,0x1f,0x10,0xe8,0x03,0x00,0x00]
st r1, [1000]

; ============================================================
; LR (load from auxiliary register)
; Per spec: opcode 0x01, C[4]=1 (LR indicator)
; ============================================================

; CHECK: lr	r1, [r2] ; encoding: [0x00,0x20,0x21,0x08]
lr r1, [r2]
; CHECK: lr	r1, [256] ; encoding: [0x00,0x7c,0x3f,0x08,0x00,0x01,0x00,0x00]
lr r1, [0x100]

; ============================================================
; SR (store to auxiliary register)
; Per spec: opcode 0x02, A[4]=1 (SR indicator)
; ============================================================

; CHECK: sr	r1, [r2] ; encoding: [0x00,0x02,0x01,0x12]
sr r1, [r2]
; CHECK: sr	r1, [256] ; encoding: [0x00,0x02,0x1f,0x12,0x00,0x01,0x00,0x00]
sr r1, [0x100]
