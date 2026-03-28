//===-- ARC4InstrInfo.td - ARC4 Instruction Definitions ---*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "ARC4InstrFormats.td"

//===----------------------------------------------------------------------===//
// Operand types
//===----------------------------------------------------------------------===//

// Operand match classes — let the AsmMatcher prefer shimm over limm.
def Imm32AsmOperand : AsmOperandClass {
  let Name = "Imm32";
  let PredicateMethod = "isImm";        // any immediate matches
  let RenderMethod = "addImmOperands";
}
def Simm9AsmOperand : AsmOperandClass {
  let Name = "Simm9";
  let PredicateMethod = "isSimm9";
  let RenderMethod = "addImmOperands";
  let SuperClasses = [Imm32AsmOperand]; // simm9 IS-A imm32 (more specific)
}

// 9-bit signed immediate (shimm)
def simm9 : Operand<i32> {
  let PrintMethod = "printImm";
  let ParserMatchClass = Simm9AsmOperand;
}

// 32-bit immediate or label reference (limm)
def imm32 : Operand<i32> {
  let PrintMethod = "printImm";
  let ParserMatchClass = Imm32AsmOperand;
}

// 1-bit immediate (F flag, SetFlags)
def imm1 : Operand<i32> { let PrintMethod = "printImm"; }

// 2-bit delay-slot mode (NN)
def nn_imm : Operand<i32> { let PrintMethod = "printDelaySlot"; }

// 5-bit condition code (Q)
def cc_imm : Operand<i32> { let PrintMethod = "printCondCode"; }

// Discard-destination operand: matches literal 0 for "test" forms (op 0,b,c)
def DiscardAsmOperand : AsmOperandClass {
  let Name = "Discard";
  let PredicateMethod = "isImm0";
  let RenderMethod = "addImmOperands";
}
def discard : Operand<i32> {
  let PrintMethod = "printImm";
  let ParserMatchClass = DiscardAsmOperand;
}

//===----------------------------------------------------------------------===//
// ALU instructions (opcodes 0x08–0x0F)
// MCInst layouts:
//   _rrr: A(reg), B(reg), C(reg), F(imm1), NN(nn_imm), Q(cc_imm)
//   _rrs: A(reg), B(reg), D(simm9), SetFlags(imm1), NN(nn_imm)
//   _rrl: A(reg), B(reg), LImm(imm32), F(imm1), NN(nn_imm), Q(cc_imm)
//===----------------------------------------------------------------------===//

multiclass ARC4ALU<bits<5> Opc, string Mn> {
  // With result: op a, b, c/shimm/limm
  def _rrr : ARC4RegFmt<Opc,
      (outs GR:$A), (ins GR:$B, GR:$C, imm1:$F, nn_imm:$NN, cc_imm:$Q),
      Mn#"\t$A, $B, $C">;
  def _rrs : ARC4ShimmFmt<Opc,
      (outs GR:$A), (ins GR:$B, simm9:$D, imm1:$SetFlags, nn_imm:$NN),
      Mn#"\t$A, $B, $D">;
  def _rrl : ARC4LimmFmt<Opc,
      (outs GR:$A), (ins GR:$B, imm32:$LImm, imm1:$F, nn_imm:$NN, cc_imm:$Q),
      Mn#"\t$A, $B, $LImm">;

  // Discard result (test): op 0, b, c/shimm/limm
  // Per spec: A=63 (shimm no-set-flags) discards result.
  def _0rr : ARC4RegFmt<Opc,
      (outs), (ins discard:$Z, GR:$B, GR:$C, imm1:$F, nn_imm:$NN, cc_imm:$Q),
      Mn#"\t$Z, $B, $C">;
  def _0rs : ARC4ShimmFmt<Opc,
      (outs), (ins discard:$Z, GR:$B, simm9:$D, imm1:$SetFlags, nn_imm:$NN),
      Mn#"\t$Z, $B, $D">;
  def _0rl : ARC4LimmFmt<Opc,
      (outs), (ins discard:$Z, GR:$B, imm32:$LImm, imm1:$F, nn_imm:$NN, cc_imm:$Q),
      Mn#"\t$Z, $B, $LImm">;

  // Shimm as operand1: op a, shimm, c
  def _rsr : ARC4ShimmFmt<Opc,
      (outs GR:$A), (ins simm9:$D, GR:$C, imm1:$SetFlags, nn_imm:$NN),
      Mn#"\t$A, $D, $C"> {
    let TSFlags{17} = 1; // shimm-in-B flag for emitter
  }

  // Limm as operand1: op a, limm, c
  def _rlr : ARC4LimmFmt<Opc,
      (outs GR:$A), (ins imm32:$LImm, GR:$C, imm1:$F, nn_imm:$NN, cc_imm:$Q),
      Mn#"\t$A, $LImm, $C"> {
    let TSFlags{17} = 1; // limm-in-B flag for emitter
  }
}

defm ADD : ARC4ALU<0x08, "add">;
defm ADC : ARC4ALU<0x09, "adc">;
defm SUB : ARC4ALU<0x0A, "sub">;
defm SBC : ARC4ALU<0x0B, "sbc">;
defm AND : ARC4ALU<0x0C, "and">;
defm OR  : ARC4ALU<0x0D, "or">;
defm BIC : ARC4ALU<0x0E, "bic">;
defm XOR : ARC4ALU<0x0F, "xor">;

//===----------------------------------------------------------------------===//
// Single-operand instructions (opcode 0x03)
// MCInst layout:
//   _r: A(reg), B(reg), F(imm1), Q(cc_imm)   [C is fixed = sub-opcode]
//   _l: A(reg), LImm(imm32), F(imm1), Q(cc_imm)
// The C field (sub-opcode) is baked into the Inst encoding via let C = ...
// The emitter reads it from the pre-encoded Inst bits or a separate TSFlag.
// For simplicity, we store the sub-opcode in TSFlags[11:7].
//===----------------------------------------------------------------------===//

class ARC4SO_r<bits<5> SubOp, string Mn>
    : ARC4RegFmt<0x03,
      (outs GR:$A), (ins GR:$B, imm1:$F, cc_imm:$Q),
      Mn#"\t$A, $B"> {
  let TSFlags{11-7} = SubOp;
}
class ARC4SO_l<bits<5> SubOp, string Mn>
    : ARC4LimmFmt<0x03,
      (outs GR:$A), (ins imm32:$LImm, imm1:$F, cc_imm:$Q),
      Mn#"\t$A, $LImm"> {
  let TSFlags{11-7} = SubOp;
}

// Single-operand shimm form: dest = single_op(shimm)
class ARC4SO_s<bits<5> SubOp, string Mn>
    : ARC4ShimmFmt<0x03,
      (outs GR:$A), (ins simm9:$D, imm1:$SetFlags, nn_imm:$NN),
      Mn#"\t$A, $D"> {
  let TSFlags{11-7} = SubOp;
}

// Single-operand discard: 0 = single_op(b) — test, set flags
class ARC4SO_0r<bits<5> SubOp, string Mn>
    : ARC4RegFmt<0x03,
      (outs), (ins discard:$Z, GR:$B, imm1:$F, cc_imm:$Q),
      Mn#"\t$Z, $B"> {
  let TSFlags{11-7} = SubOp;
}

multiclass ARC4SO<bits<5> SubOp, string Mn> {
  def _r  : ARC4SO_r<SubOp, Mn>;
  def _l  : ARC4SO_l<SubOp, Mn>;
  def _s  : ARC4SO_s<SubOp, Mn>;
  def _0r : ARC4SO_0r<SubOp, Mn>;
}

defm FLAG : ARC4SO<0x00, "flag">;
defm ASR  : ARC4SO<0x01, "asr">;
defm LSR  : ARC4SO<0x02, "lsr">;
defm ROR  : ARC4SO<0x03, "ror">;
defm RRC  : ARC4SO<0x04, "rrc">;
defm SEXB : ARC4SO<0x05, "sexb">;
defm SEXW : ARC4SO<0x06, "sexw">;
defm EXTB : ARC4SO<0x07, "extb">;
defm EXTW : ARC4SO<0x08, "extw">;

// No-operand instructions (full encoding fixed, 0 MCInst operands)
// Per spec: A=63, B=63, C=63, D[8:0] discriminates (0=BRK, 1=SLEEP, 2=SWI).
// TSFlags[14:13] encodes the D value for the emitter.
def BRK   : ARC4RegFmt<0x03, (outs), (ins), "brk"> {
  let TSFlags{14-13} = 0; // D=0
}
def SLEEP : ARC4RegFmt<0x03, (outs), (ins), "sleep"> {
  let TSFlags{14-13} = 1; // D=1
}
def SWI   : ARC4RegFmt<0x03, (outs), (ins), "swi"> {
  let TSFlags{14-13} = 2; // D=2
}

//===----------------------------------------------------------------------===//
// Load instructions (opcodes 0x00, 0x01)
// The ZZ (size) and X (sign-extend) fields are encoded in C[5:4] for loads;
// we store them in TSFlags[13:12] so the emitter can pick them up.
//
// MCInst layouts (same as ALU _rrr/_rrs/_rrl):
//   _rrr: A, B, C_reg, F(unused=0), NN, Q(unused=0)
//   _rrs: A, B, D(shimm), SetFlags(0), NN
//   _abs: A, LImm, F(0), NN, Q(0)  [absolute: B=r62, C=r62]
//===----------------------------------------------------------------------===//

// ZZ/X bits in TSFlags[13:12]: 00=LD, 01=LDB, 10=LDW, 11=LDWX/LDBX
// Additional X bit in TSFlags[14]: 0=no sign-extend, 1=sign-extend
class ARC4Load_rrr<bits<2> ZZ, bit X, string Mn>
    : ARC4RegFmt<0x00,
      (outs GR:$A), (ins GR:$B, GR:$C),
      Mn#"\t$A, [$B, $C]"> {
  let TSFlags{13-12} = ZZ;
  let TSFlags{14}    = X;
}
class ARC4Load_rrs<bits<2> ZZ, bit X, string Mn>
    : ARC4ShimmFmt<0x00,
      (outs GR:$A), (ins GR:$B, simm9:$D),
      Mn#"\t$A, [$B, $D]"> {
  let TSFlags{13-12} = ZZ;
  let TSFlags{14}    = X;
  // SetFlags implicit=0, NN implicit=0
}
class ARC4Load_abs<bits<2> ZZ, bit X, string Mn>
    : ARC4LimmFmt<0x00,
      (outs GR:$A), (ins imm32:$LImm),
      Mn#"\t$A, [$LImm]"> {
  let TSFlags{13-12} = ZZ;
  let TSFlags{14}    = X;
  // F=0, NN=0, Q=0 implicit
}

// LDB/LDW use opcode 0x01 per spec; LD (32-bit) uses 0x00
// ZZ: 00=32-bit, 01=byte, 10=word
def LD_rrr   : ARC4Load_rrr<0b00, 0, "ld">;
def LDB_rrr  : ARC4RegFmt<0x00, (outs GR:$A), (ins GR:$B, GR:$C), "ldb\t$A, [$B, $C]"> {
  let TSFlags{13-12} = 0b01; let TSFlags{14} = 0;
}
def LDW_rrr  : ARC4RegFmt<0x00, (outs GR:$A), (ins GR:$B, GR:$C), "ldw\t$A, [$B, $C]"> {
  let TSFlags{13-12} = 0b10; let TSFlags{14} = 0;
}

// LD_rrs uses opcode 0x01 per spec (shimm load format)
def LD_rrs   : ARC4ShimmFmt<0x01, (outs GR:$A), (ins GR:$B, simm9:$D), "ld\t$A, [$B, $D]"> {
  let TSFlags{13-12} = 0b00; let TSFlags{14} = 0;
}
def LDB_rrs  : ARC4ShimmFmt<0x01, (outs GR:$A), (ins GR:$B, simm9:$D), "ldb\t$A, [$B, $D]"> {
  let TSFlags{13-12} = 0b01; let TSFlags{14} = 0;
}
def LDW_rrs  : ARC4ShimmFmt<0x01, (outs GR:$A), (ins GR:$B, simm9:$D), "ldw\t$A, [$B, $D]"> {
  let TSFlags{13-12} = 0b10; let TSFlags{14} = 0;
}

// Absolute loads use opcode 0x00 (register form with B=C=62 limm)
def LD_abs   : ARC4Load_abs<0b00, 0, "ld">;
def LDB_abs  : ARC4LimmFmt<0x00, (outs GR:$A), (ins imm32:$LImm), "ldb\t$A, [$LImm]"> {
  let TSFlags{13-12} = 0b01; let TSFlags{14} = 0;
}
def LDW_abs  : ARC4LimmFmt<0x00, (outs GR:$A), (ins imm32:$LImm), "ldw\t$A, [$LImm]"> {
  let TSFlags{13-12} = 0b10; let TSFlags{14} = 0;
}

// Sign-extend register variants (opcode 0x00, Z/X encoded in lower bits)
def LDBX_rrr : ARC4RegFmt<0x00, (outs GR:$A), (ins GR:$B, GR:$C), "ldbx\t$A, [$B, $C]"> {
  let TSFlags{13-12} = 0b01; let TSFlags{14} = 1;
}
def LDWX_rrr : ARC4RegFmt<0x00, (outs GR:$A), (ins GR:$B, GR:$C), "ldwx\t$A, [$B, $C]"> {
  let TSFlags{13-12} = 0b10; let TSFlags{14} = 1;
}
// Sign-extend shimm variants (opcode 0x01, Z/X encoded in C field)
def LDBX_rrs : ARC4ShimmFmt<0x01, (outs GR:$A), (ins GR:$B, simm9:$D), "ldbx\t$A, [$B, $D]"> {
  let TSFlags{13-12} = 0b01; let TSFlags{14} = 1;
}
def LDWX_rrs : ARC4ShimmFmt<0x01, (outs GR:$A), (ins GR:$B, simm9:$D), "ldwx\t$A, [$B, $D]"> {
  let TSFlags{13-12} = 0b10; let TSFlags{14} = 1;
}

// LR – auxiliary register read (opcode 0x01, ZZ special)
def LR_r   : ARC4RegFmt<0x01, (outs GR:$A), (ins GR:$B), "lr\t$A, [$B]"> {
  let TSFlags{15} = 1; // isLR flag
}
def LR_abs : ARC4LimmFmt<0x01, (outs GR:$A), (ins imm32:$LImm), "lr\t$A, [$LImm]"> {
  let TSFlags{15} = 1;
}

//===----------------------------------------------------------------------===//
// Store instructions (opcode 0x02)
// MCInst layouts:
//   _rrr: C(reg), B(reg), A(reg)   [st c, [b, a]]
//   _rrs: C(reg), B(reg), D(simm9)
//   _abs: C(reg), LImm(imm32)
//===----------------------------------------------------------------------===//

// Per spec: stores always have shimm format. A field = {Di,0,Aw,ZZ,0} flags.
// C = source register, B = base register, D[8:0] = signed offset.
// No register-register store form exists in the ARC4 ISA.
// MCInst layout: C(reg), B(reg), D(simm9)  OR  C(reg), B(reg) [offset=0]
def ST_rrs  : ARC4ShimmFmt<0x02, (outs), (ins GR:$C, GR:$B, simm9:$D), "st\t$C, [$B, $D]">;
def ST_rb   : ARC4ShimmFmt<0x02, (outs), (ins GR:$C, GR:$B), "st\t$C, [$B]">;
def STB_rrs : ARC4ShimmFmt<0x02, (outs), (ins GR:$C, GR:$B, simm9:$D), "stb\t$C, [$B, $D]"> {
  let TSFlags{13-12} = 0b01;
}
def STB_rb  : ARC4ShimmFmt<0x02, (outs), (ins GR:$C, GR:$B), "stb\t$C, [$B]"> {
  let TSFlags{13-12} = 0b01;
}
def STW_rrs : ARC4ShimmFmt<0x02, (outs), (ins GR:$C, GR:$B, simm9:$D), "stw\t$C, [$B, $D]"> {
  let TSFlags{13-12} = 0b10;
}
def STW_rb  : ARC4ShimmFmt<0x02, (outs), (ins GR:$C, GR:$B), "stw\t$C, [$B]"> {
  let TSFlags{13-12} = 0b10;
}

// Absolute store: B=62 (limm for address), C = source register.
def ST_abs  : ARC4LimmFmt<0x02, (outs), (ins GR:$C, imm32:$LImm), "st\t$C, [$LImm]">;
def STB_abs : ARC4LimmFmt<0x02, (outs), (ins GR:$C, imm32:$LImm), "stb\t$C, [$LImm]"> {
  let TSFlags{13-12} = 0b01;
}
def STW_abs : ARC4LimmFmt<0x02, (outs), (ins GR:$C, imm32:$LImm), "stw\t$C, [$LImm]"> {
  let TSFlags{13-12} = 0b10;
}

// SR – auxiliary register write (opcode 0x02)
def SR_r   : ARC4RegFmt<0x02, (outs), (ins GR:$C, GR:$B), "sr\t$C, [$B]"> {
  let TSFlags{15} = 1; // isSR flag
}
def SR_abs : ARC4LimmFmt<0x02, (outs), (ins GR:$C, imm32:$LImm), "sr\t$C, [$LImm]"> {
  let TSFlags{15} = 1;
}

//===----------------------------------------------------------------------===//
// Control flow
//===----------------------------------------------------------------------===//

// Branch (0x04), BL (0x05), LP (0x06)
// MCInst: Target(imm32/expr), NN(nn_imm), Q(cc_imm)
def B  : ARC4BranchFmt<0x04, (outs), (ins imm32:$Target, nn_imm:$NN, cc_imm:$Q), "b\t$Target">;
def BL : ARC4BranchFmt<0x05, (outs), (ins imm32:$Target, nn_imm:$NN, cc_imm:$Q), "bl\t$Target">;
def LP : ARC4BranchFmt<0x06, (outs), (ins imm32:$Target, nn_imm:$NN, cc_imm:$Q), "lp\t$Target">;

// Jump (0x07, RegFmt)
// MCInst: B(reg), F(imm1), NN(nn_imm), Q(cc_imm)  [A=0, C=0]
def J_r  : ARC4RegFmt<0x07,
    (outs), (ins GR:$B, imm1:$F, nn_imm:$NN, cc_imm:$Q),
    "j\t[$B]">;
def J_l  : ARC4LimmFmt<0x07,
    (outs), (ins imm32:$LImm, imm1:$F, nn_imm:$NN, cc_imm:$Q),
    "j\t$LImm">;

// Jump-and-link (0x07, A=r31=blink)
def JL_r : ARC4RegFmt<0x07,
    (outs), (ins GR:$B, imm1:$F, nn_imm:$NN, cc_imm:$Q),
    "jl\t[$B]"> {
  let TSFlags{12} = 1; // isJL flag: emitter sets bit 9 per spec p98
}
def JL_l : ARC4LimmFmt<0x07,
    (outs), (ins imm32:$LImm, imm1:$F, nn_imm:$NN, cc_imm:$Q),
    "jl\t$LImm"> {
  let TSFlags{12} = 1;
}

//===----------------------------------------------------------------------===//
// NOP pseudo (XOR r0, r63, r63 with shimm=0)
// No MCInst operands; encoding is fully fixed.
//===----------------------------------------------------------------------===//
def NOP : ARC4ShimmFmt<0x0F, (outs), (ins), "nop"> {
  let TSFlags{16} = 1; // isNOP flag for emitter
}

//===----------------------------------------------------------------------===//
// Target-specific SDNodes for codegen
//===----------------------------------------------------------------------===//

def SDT_ARC4CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def SDT_ARC4CallSeqEnd   : SDCallSeqEnd<[SDTCisVT<0, i32>, SDTCisVT<1, i32>]>;
def CallSeqStart : SDNode<"ISD::CALLSEQ_START", SDT_ARC4CallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def CallSeqEnd   : SDNode<"ISD::CALLSEQ_END", SDT_ARC4CallSeqEnd,
                           [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def SDT_ARC4Call : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_ARC4Ret  : SDTypeProfile<0, -1, []>;
def SDT_ARC4Cmp  : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_ARC4BrCC : SDTypeProfile<0, -1, []>;
def SDT_ARC4Shift1 : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>]>;
def SDT_ARC4SelectCC : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>, SDTCisSameAs<1, 2>]>;

def ARC4call    : SDNode<"ARC4ISD::CALL", SDT_ARC4Call,
                         [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;
def ARC4retglue : SDNode<"ARC4ISD::RET_GLUE", SDT_ARC4Ret,
                         [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def ARC4cmp     : SDNode<"ARC4ISD::CMP", SDT_ARC4Cmp, [SDNPOutGlue]>;
def ARC4brcc    : SDNode<"ARC4ISD::BR_CC", SDT_ARC4BrCC,
                         [SDNPHasChain, SDNPInGlue]>;
def ARC4selectcc : SDNode<"ARC4ISD::SELECT_CC", SDT_ARC4SelectCC,
                          [SDNPInGlue]>;
def ARC4asr1 : SDNode<"ARC4ISD::ASR1", SDT_ARC4Shift1>;
def ARC4lsr1 : SDNode<"ARC4ISD::LSR1", SDT_ARC4Shift1>;

//===----------------------------------------------------------------------===//
// Codegen-only instructions
// These have clean operand lists for DAG pattern matching.
// MCInstLower translates them to the MC-layer forms (appending F/NN/Q=0).
//===----------------------------------------------------------------------===//

let isCodeGenOnly = 1 in {

// --- ALU register-register ---
class CG_ALU_rr<bits<5> Opc, string mn, SDPatternOperator op>
    : ARC4RegFmt<Opc, (outs GR:$dst), (ins GR:$src1, GR:$src2),
      mn#"\t$dst, $src1, $src2",
      [(set GR:$dst, (op GR:$src1, GR:$src2))]>;

def CG_ADDrr : CG_ALU_rr<0x08, "add", add>;
def CG_SUBrr : CG_ALU_rr<0x0A, "sub", sub>;
def CG_ANDrr : CG_ALU_rr<0x0C, "and", and>;
def CG_ORrr  : CG_ALU_rr<0x0D, "or",  or>;
def CG_XORrr : CG_ALU_rr<0x0F, "xor", xor>;

// --- ALU register-immediate (shimm, 9-bit signed) ---
class CG_ALU_ri<bits<5> Opc, string mn, SDPatternOperator op>
    : ARC4ShimmFmt<Opc, (outs GR:$dst), (ins GR:$src1, simm9:$imm),
      mn#"\t$dst, $src1, $imm",
      [(set GR:$dst, (op GR:$src1, (i32 imm:$imm)))]>;

def CG_ADDri : CG_ALU_ri<0x08, "add", add>;
def CG_SUBri : CG_ALU_ri<0x0A, "sub", sub>;
def CG_ANDri : CG_ALU_ri<0x0C, "and", and>;
def CG_ORri  : CG_ALU_ri<0x0D, "or",  or>;

// --- MOV register (copy via AND dst, src, src) ---
// Per spec page 108: MOV is the AND instruction
def CG_MOVrr : ARC4RegFmt<0x0C, (outs GR:$dst), (ins GR:$src),
    "mov\t$dst, $src", []>;

// --- MOV shimm (AND dst, shimm, shimm - shimms match) ---
// Per spec: MOV a,shimm encodes as AND with B=C=shimm_sentinel, D=value
def CG_MOVri : ARC4ShimmFmt<0x0C, (outs GR:$dst), (ins simm9:$imm),
    "mov\t$dst, $imm", []>;

// --- MOV limm (AND dst, limm - 32-bit constant) ---
def CG_MOVli : ARC4LimmFmt<0x0C, (outs GR:$dst), (ins imm32:$imm),
    "mov\t$dst, $imm", []>;

// --- Load word (32-bit) from [base + offset] ---
def CG_LDri : ARC4ShimmFmt<0x01, (outs GR:$dst), (ins GR:$base, simm9:$off),
    "ld\t$dst, [$base, $off]",
    [(set GR:$dst, (load (add GR:$base, (i32 imm:$off))))]> {
  let TSFlags{13-12} = 0b00;
  let TSFlags{14} = 0;
  let mayLoad = 1;
}

// --- Load word from [base + reg] ---
def CG_LDrr : ARC4RegFmt<0x00, (outs GR:$dst), (ins GR:$base, GR:$off),
    "ld\t$dst, [$base, $off]",
    [(set GR:$dst, (load (add GR:$base, GR:$off)))]> {
  let mayLoad = 1;
}

// --- Store word to [base + offset] ---
def CG_STri : ARC4ShimmFmt<0x02, (outs), (ins GR:$src, GR:$base, simm9:$off),
    "st\t$src, [$base, $off]",
    [(store GR:$src, (add GR:$base, (i32 imm:$off)))]> {
  let mayStore = 1;
}

// --- Compare (SUB with dest=discard, sets flags) ---
def CG_CMPrr : ARC4RegFmt<0x0A, (outs), (ins GR:$src1, GR:$src2),
    "sub.f\t0, $src1, $src2",
    [(ARC4cmp GR:$src1, GR:$src2)]> {
  let Defs = [STATUS];
}

def CG_CMPri : ARC4ShimmFmt<0x0A, (outs), (ins GR:$src1, simm9:$imm),
    "sub.f\t0, $src1, $imm", []> {
  let Defs = [STATUS];
}

// --- Conditional branch ---
def CG_BRcc : ARC4BranchFmt<0x04, (outs), (ins imm32:$target, imm1:$cc),
    "b\t$target", []> {
  let isBranch = 1;
  let isTerminator = 1;
  let Uses = [STATUS];
}

// --- Unconditional branch ---
def CG_BR : ARC4BranchFmt<0x04, (outs), (ins imm32:$target),
    "b\t$target", []> {
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

// --- Return (expands to j [blink]) ---
def CG_RET : ARC4Inst<0x07, {0, 0}, (outs), (ins), "j\t[r31]",
    [(ARC4retglue)]> {
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

// --- Call register ---
def CG_CALLr : ARC4RegFmt<0x07, (outs), (ins GR:$target),
    "jl\t[$target]", [(ARC4call GR:$target)]> {
  let isCall = 1;
  let TSFlags{12} = 1; // isJL
  let Defs = [R0, R1, R2, R3, R4, R5, R6, R7, R31];
}

// --- Call immediate (limm) ---
def CG_CALLi : ARC4LimmFmt<0x07, (outs), (ins imm32:$target),
    "jl\t$target", []> {
  let isCall = 1;
  let TSFlags{12} = 1;
  let Defs = [R0, R1, R2, R3, R4, R5, R6, R7, R31];
}

// --- Call frame pseudo-instructions ---
let Defs = [R28], Uses = [R28] in {
def ADJCALLSTACKDOWN : Pseudo<(outs), (ins i32imm:$amt, i32imm:$amt2),
    "# ADJCALLSTACKDOWN $amt",
    [(CallSeqStart timm:$amt, timm:$amt2)]>;
def ADJCALLSTACKUP   : Pseudo<(outs), (ins i32imm:$amt, i32imm:$amt2),
    "# ADJCALLSTACKUP $amt",
    [(CallSeqEnd timm:$amt, timm:$amt2)]>;
}

// --- SELECT_CC pseudo: expanded to a conditional branch diamond ---
// Operands: dst, trueVal, falseVal, condCode
// Uses glue (flags from CMP). Expanded by EmitInstrWithCustomInserter.
def CG_SELECT_CC : Pseudo<(outs GR:$dst),
    (ins GR:$truev, GR:$falsev, i32imm:$cc),
    "# SELECT_CC $dst, $truev, $falsev, $cc",
    [(set GR:$dst, (ARC4selectcc GR:$truev, GR:$falsev, (i32 imm:$cc)))]> {
  let usesCustomInserter = 1;
  let Uses = [STATUS];
}

// --- Shift-by-1 codegen instructions ---
// These match the ARC4ISD::ASR1/LSR1 target nodes emitted by LowerShift.
def CG_ASR : ARC4RegFmt<0x03, (outs GR:$dst), (ins GR:$src),
    "asr\t$dst, $src", [(set GR:$dst, (ARC4asr1 GR:$src))]> {
  let TSFlags{11-7} = 1; // ASR sub-opcode
}
def CG_LSR : ARC4RegFmt<0x03, (outs GR:$dst), (ins GR:$src),
    "lsr\t$dst, $src", [(set GR:$dst, (ARC4lsr1 GR:$src))]> {
  let TSFlags{11-7} = 2; // LSR sub-opcode
}

} // isCodeGenOnly = 1
