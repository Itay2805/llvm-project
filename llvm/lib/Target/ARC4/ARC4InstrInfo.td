//===-- ARC4InstrInfo.td - ARC4 Instruction defs -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "ARC4InstrFormats.td"

// nop is encoded as 0x7fffffff
let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def NOP : ARC4Inst<(outs), (ins), "nop", []> {
  let Inst{31-0} = 0x7fffffff;
}
