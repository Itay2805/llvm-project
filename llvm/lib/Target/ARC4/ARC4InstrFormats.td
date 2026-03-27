//===-- ARC4InstrFormats.td - ARC4 Instruction Formats ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// All instructions are 32 bits wide; limm instructions are 64 bits
// (instruction word + immediate word).
//
// TSFlags layout (used by ARC4MCCodeEmitter):
//   bits[4:0] = 5-bit ARC4 opcode (I field, Inst[31:27])
//   bits[6:5] = format: 0=RegFmt, 1=ShimmFmt, 2=LimmFmt, 3=BranchFmt
//
// MCInst operand layouts (ARC4AsmParser contract):
//
//   RegFmt:    A(reg), B(reg), C(reg), F(imm1), NN(imm2), Q(imm5)
//   ShimmFmt:  A(reg), B(reg), D(imm9), SetFlags(imm1), NN(imm2)
//              SetFlags=0 → C=63 (no-flag sentinel)
//              SetFlags=1 → C=61 (flag sentinel)
//   LimmFmt:   A(reg), B(reg), LImm(imm32/expr), F(imm1), NN(imm2), Q(imm5)
//              C is hardcoded to 62 (limm sentinel)
//   BranchFmt: Target(expr), NN(imm2), Q(imm5)
//
// "No-operand" RegFmt instructions (BRK/SLEEP/SWI/NOP) carry 0 MCInst operands;
// their full encoding is fixed in the Inst field.
//===----------------------------------------------------------------------===//

class ARC4Inst<bits<5> Opc, bits<2> Fmt, dag OutOps, dag InOps,
               string AsmStr, list<dag> Pat = []>
    : Instruction {
  field bits<32> Inst = 0;
  let Namespace      = "ARC4";
  let OutOperandList = OutOps;
  let InOperandList  = InOps;
  let AsmString      = AsmStr;
  let Pattern        = Pat;
  // limm instructions occupy 8 bytes (insn word + limm word)
  let Size           = !if(!eq(!cast<int>(Fmt), 2), 8, 4);
  // Embed opcode and format in TSFlags so the emitter can read them
  let TSFlags{4-0}   = Opc;
  let TSFlags{6-5}   = Fmt;
  // Pre-set opcode bits in Inst for reference / disassembler use
  let Inst{31-27}    = Opc;
}

// Convenience aliases for format codes (must be int-convertible in TSFlags)
class ARC4RegFmt<bits<5> Opc, dag OutOps, dag InOps, string AsmStr,
                 list<dag> Pat = []>
    : ARC4Inst<Opc, {0, 0}, OutOps, InOps, AsmStr, Pat>;

class ARC4ShimmFmt<bits<5> Opc, dag OutOps, dag InOps, string AsmStr,
                   list<dag> Pat = []>
    : ARC4Inst<Opc, {0, 1}, OutOps, InOps, AsmStr, Pat>;

class ARC4LimmFmt<bits<5> Opc, dag OutOps, dag InOps, string AsmStr,
                  list<dag> Pat = []>
    : ARC4Inst<Opc, {1, 0}, OutOps, InOps, AsmStr, Pat>;

class ARC4BranchFmt<bits<5> Opc, dag OutOps, dag InOps, string AsmStr,
                    list<dag> Pat = []>
    : ARC4Inst<Opc, {1, 1}, OutOps, InOps, AsmStr, Pat>;
