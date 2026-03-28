//===-- ARC4InstrFormats.td - ARC4 Instruction Formats -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 instruction formats. All instructions are 32-bit.
//
// Encoding reference: binutils-2.15/opcodes/arc-opc.c
//   I[31:27] = major opcode (5 bits)
//   A[26:21] = register A / destination (6 bits)
//   B[20:15] = register B / source 1 (6 bits)
//   C[14:9]  = register C / source 2 (6 bits)
//   [8:0]    = flags/shimm (9 bits)
//
//===----------------------------------------------------------------------===//

class ARC4Inst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  let Namespace = "ARC4";
  let Size = 4;

  field bits<32> Inst;
  field bits<32> SoftFail = 0;

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
}

// 3-operand ALU format: op a, b, c
class ARC4InstALU3<bits<5> opcode, dag outs, dag ins, string asmstr,
                   list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;
  bits<6> c;

  let Inst{31-27} = opcode;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = c;
  let Inst{8-0}   = 0;
}

// Single operand format (opcode 3, sub-op in C field)
class ARC4InstSOP<bits<6> subop, dag outs, dag ins, string asmstr,
                  list<dag> pattern>
    : ARC4Inst<outs, ins, asmstr, pattern> {
  bits<6> a;
  bits<6> b;

  let Inst{31-27} = 0b00011;
  let Inst{26-21} = a;
  let Inst{20-15} = b;
  let Inst{14-9}  = subop;
  let Inst{8-0}   = 0;
}
