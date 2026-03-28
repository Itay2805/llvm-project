; RUN: llvm-mc -triple=arc4-unknown-elf --filetype=obj %s -o %t.o
; RUN: llvm-objdump -d --triple=arc4-unknown-elf --no-show-raw-insn %t.o | \
; RUN:   FileCheck %s

; Assemble → disassemble round-trip test.
; Every instruction must disassemble back to its original mnemonic and operands.

  .text

; CHECK: add	r0, r1, r2
add r0, r1, r2
; CHECK: add.f	r0, r1, r2
add.f r0, r1, r2
; CHECK: add.eq	r0, r1, r2
add.eq r0, r1, r2
; CHECK: add.ne.f	r0, r1, r2
add.ne.f r0, r1, r2
; CHECK: sub	r0, r1, 5
sub r0, r1, 5
; CHECK: sub.f	r0, r1, 5
sub.f r0, r1, 5
; CHECK: add	r0, r1, 1000
add r0, r1, 1000
; CHECK: asr	r1, r2
asr r1, r2
; CHECK: asr.f	r1, r2
asr.f r1, r2
; CHECK: asr.eq	r1, r2
asr.eq r1, r2
; CHECK: mov	r0, r1
mov r0, r1
; CHECK: cmp	r1, r2
cmp r1, r2
; CHECK: rlc	r0, r1
rlc r0, r1
; CHECK: nop
nop
; CHECK: ld	r0, [r1, r2]
ld r0, [r1, r2]
; CHECK: ld.x	r0, [r1, r2]
ld.x r0, [r1, r2]
; CHECK: ld.a	r0, [r1, 4]
ld.a r0, [r1, 4]
; CHECK: ld.x.a.di	r0, [r1, 4]
ld.x.a.di r0, [r1, 4]
; CHECK: ldb	r0, [r1, r2]
ldb r0, [r1, r2]
; CHECK: ldw	r0, [r1, r2]
ldw r0, [r1, r2]
; CHECK: st	r0, [r1, 4]
st r0, [r1, 4]
; CHECK: st.a	r0, [r1, 4]
st.a r0, [r1, 4]
; CHECK: st.a.di	r0, [r1, 4]
st.a.di r0, [r1, 4]
; CHECK: stb	r0, [r1, 8]
stb r0, [r1, 8]
; CHECK: add	0, r1, r2
add 0, r1, r2
; CHECK: add.f	0, r1, r2
add.f 0, r1, r2
; CHECK: flag	r1
flag r1
; CHECK: j	[r5]
j [r5]
; CHECK: j	[blink]
j [blink]
; CHECK: jl	[r5]
jl [r5]
