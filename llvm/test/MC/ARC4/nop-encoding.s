; RUN: llvm-mc -triple=arc4-unknown-elf -show-encoding %s | FileCheck %s

; CHECK: nop ; encoding: [0xff,0xff,0xff,0x7f]
nop
