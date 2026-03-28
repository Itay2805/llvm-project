# REQUIRES: arc4
# RUN: llvm-mc -filetype=obj -triple=arc4-unknown-elf %s -o %t.o
# RUN: ld.lld %t.o -o %t --entry=__start
# RUN: llvm-readelf -h %t | FileCheck %s

# CHECK: Machine: ARC

  .text
  .globl __start
__start:
  nop
