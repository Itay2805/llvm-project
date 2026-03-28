//===-- ARC4InstrInfo.td - ARC4 Instruction defs -----------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Full ARC4 (ARCtangent-A4) base instruction set definitions.
//
// Instruction encoding reference: binutils-2.15/opcodes/arc-opc.c
//
// Operand variant suffixes:
//   rrr  = reg, reg, reg         rrs  = reg, reg, shimm
//   rsr  = reg, shimm, reg       rss  = reg, shimm, shimm
//   rrl  = reg, reg, limm        rlr  = reg, limm, reg
//   0xx  = discard variant (A = sentinel 61, no destination register)
//
//===----------------------------------------------------------------------===//

include "ARC4InstrFormats.td"

//===----------------------------------------------------------------------===//
// Operand match classes
//===----------------------------------------------------------------------===//

def Limm32AsmOperand : AsmOperandClass {
  let Name = "Limm32";
  let RenderMethod = "addImmOperands";
  let PredicateMethod = "isImm";
}

def Simm9AsmOperand : AsmOperandClass {
  let Name = "Simm9";
  let RenderMethod = "addImmOperands";
  let PredicateMethod = "isSimm9";
  let SuperClasses = [Limm32AsmOperand];
}

def BrTarget20AsmOperand : AsmOperandClass {
  let Name = "BrTarget20";
  let RenderMethod = "addImmOperands";
  let PredicateMethod = "isImm";
}

//===----------------------------------------------------------------------===//
// Operand types
//===----------------------------------------------------------------------===//

// 9-bit signed immediate (shimm). Fits in bits [8:0].
def simm9 : Operand<i32> {
  let DecoderMethod = "decodeSimm9";
  let ParserMatchClass = Simm9AsmOperand;
}

// 32-bit long immediate (limm). Extra word after instruction.
def limm32 : Operand<i32> {
  let DecoderMethod = "decodeLimm32";
  let ParserMatchClass = Limm32AsmOperand;
}

// 20-bit branch target (PC-relative, encoded in bits [26:7]).
def brtarget20 : Operand<OtherVT> {
  let EncoderMethod = "getBranchTargetOpValue";
  let DecoderMethod = "decodeBranchTarget20";
  let ParserMatchClass = BrTarget20AsmOperand;
}

// 9-bit signed store offset.
def st_offset9 : Operand<i32> {
  let DecoderMethod = "decodeSimm9";
  let ParserMatchClass = Simm9AsmOperand;
}

// 9-bit signed load offset (opcode 1).
def ld_offset9 : Operand<i32> {
  let DecoderMethod = "decodeSimm9";
  let ParserMatchClass = Simm9AsmOperand;
}

//===----------------------------------------------------------------------===//
// NOP - fixed encoding 0x7fffffff
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in
def NOP : ARC4Inst<(outs), (ins), "nop", []> {
  let Inst{31-0} = 0x7fffffff;
}

//===----------------------------------------------------------------------===//
// 3-Operand ALU Instructions
//
// 12 variants per mnemonic:
//   rrr, rrs, rsr, rss, rrl, rlr  (result to register)
//   0rr, 0rs, 0sr, 0ss, 0rl, 0lr  (result discarded, A = sentinel 61)
//===----------------------------------------------------------------------===//

multiclass ALU3<bits<5> opcode, string mnemonic> {
  // --- Result-producing variants ---
  def _rrr : ARC4InstALU3_rrr<opcode,
    (outs GPR32:$a), (ins GPR32:$b, GPR32:$c),
    mnemonic # "\t$a, $b, $c", []>;

  def _rrs : ARC4InstALU3_rrs<opcode,
    (outs GPR32:$a), (ins GPR32:$b, simm9:$shimm),
    mnemonic # "\t$a, $b, $shimm", []>;

  def _rsr : ARC4InstALU3_rsr<opcode,
    (outs GPR32:$a), (ins simm9:$shimm, GPR32:$c),
    mnemonic # "\t$a, $shimm, $c", []>;

  def _rss : ARC4InstALU3_rss<opcode,
    (outs GPR32:$a), (ins simm9:$shimm),
    mnemonic # "\t$a, $shimm, $shimm", []>;

  def _rrl : ARC4InstALU3_rrl<opcode,
    (outs GPR32:$a), (ins GPR32:$b, limm32:$limm),
    mnemonic # "\t$a, $b, $limm", []>;

  def _rlr : ARC4InstALU3_rlr<opcode,
    (outs GPR32:$a), (ins limm32:$limm, GPR32:$c),
    mnemonic # "\t$a, $limm, $c", []>;

  // --- Discard variants (A = sentinel 61 = SHIMM_UPDATE) ---
  let hasSideEffects = 1 in {
    def _0rr : ARC4InstALU3_rrr<opcode,
      (outs), (ins GPR32:$b, GPR32:$c),
      mnemonic # "\t0, $b, $c", []> {
      let a = 0b111101;  // sentinel 61
    }

    def _0rs : ARC4InstALU3_rrs<opcode,
      (outs), (ins GPR32:$b, simm9:$shimm),
      mnemonic # "\t0, $b, $shimm", []> {
      let a = 0b111101;
    }

    def _0sr : ARC4InstALU3_rsr<opcode,
      (outs), (ins simm9:$shimm, GPR32:$c),
      mnemonic # "\t0, $shimm, $c", []> {
      let a = 0b111101;
    }

    def _0ss : ARC4InstALU3_rss<opcode,
      (outs), (ins simm9:$shimm),
      mnemonic # "\t0, $shimm, $shimm", []> {
      let a = 0b111101;
    }

    def _0rl : ARC4InstALU3_rrl<opcode,
      (outs), (ins GPR32:$b, limm32:$limm),
      mnemonic # "\t0, $b, $limm", []> {
      let a = 0b111101;
    }

    def _0lr : ARC4InstALU3_rlr<opcode,
      (outs), (ins limm32:$limm, GPR32:$c),
      mnemonic # "\t0, $limm, $c", []> {
      let a = 0b111101;
    }
  }
}

defm ADD : ALU3<0b01000, "add">;  // opcode 8
defm ADC : ALU3<0b01001, "adc">;  // opcode 9
defm SUB : ALU3<0b01010, "sub">;  // opcode 10
defm SBC : ALU3<0b01011, "sbc">;  // opcode 11
defm AND : ALU3<0b01100, "and">;  // opcode 12
defm OR  : ALU3<0b01101, "or">;   // opcode 13
defm BIC : ALU3<0b01110, "bic">;  // opcode 14
defm XOR : ALU3<0b01111, "xor">;  // opcode 15

//===----------------------------------------------------------------------===//
// Single-Operand Instructions  (opcode 3, sub-opcode in C field)
//
// 6 variants per mnemonic:
//   rr, rs, rl  (result to register)
//   0r, 0s, 0l  (result discarded, A = sentinel 61)
//===----------------------------------------------------------------------===//

multiclass SOP<bits<6> subop, string mnemonic> {
  def _rr : ARC4InstSOP_rr<subop,
    (outs GPR32:$a), (ins GPR32:$b),
    mnemonic # "\t$a, $b", []>;

  def _rs : ARC4InstSOP_rs<subop,
    (outs GPR32:$a), (ins simm9:$shimm),
    mnemonic # "\t$a, $shimm", []>;

  def _rl : ARC4InstSOP_rl<subop,
    (outs GPR32:$a), (ins limm32:$limm),
    mnemonic # "\t$a, $limm", []>;

  // --- Discard variants ---
  let hasSideEffects = 1 in {
    def _0r : ARC4InstSOP_rr<subop,
      (outs), (ins GPR32:$b),
      mnemonic # "\t0, $b", []> {
      let a = 0b111101;  // sentinel 61
    }

    def _0s : ARC4InstSOP_rs<subop,
      (outs), (ins simm9:$shimm),
      mnemonic # "\t0, $shimm", []> {
      let a = 0b111101;
    }

    def _0l : ARC4InstSOP_rl<subop,
      (outs), (ins limm32:$limm),
      mnemonic # "\t0, $limm", []> {
      let a = 0b111101;
    }
  }
}

defm ASL  : SOP<0b000000, "asl">;
defm ASR  : SOP<0b000001, "asr">;
defm LSR  : SOP<0b000010, "lsr">;
defm ROR  : SOP<0b000011, "ror">;
defm RRC  : SOP<0b000100, "rrc">;
defm SEXB : SOP<0b000101, "sexb">;
defm SEXW : SOP<0b000110, "sexw">;
defm EXTB : SOP<0b000111, "extb">;
defm EXTW : SOP<0b001000, "extw">;

//===----------------------------------------------------------------------===//
// Branch Instructions (b=opcode 4, bl=opcode 5, lp=opcode 6)
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1 in {
  let isBarrier = 1 in
  def B : ARC4InstBranch<0b00100,
    (outs), (ins brtarget20:$offset),
    "b\t$offset", []> {
    let n = 0;
    let q = 0;
  }

  def Bcc : ARC4InstBranch<0b00100,
    (outs), (ins brtarget20:$offset, i32imm:$q),
    "b\t$offset", []> {
    let n = 0;
  }
}

let isCall = 1 in {
  let isBarrier = 1 in
  def BL : ARC4InstBranch<0b00101,
    (outs), (ins brtarget20:$offset),
    "bl\t$offset", []> {
    let n = 0;
    let q = 0;
  }

  def BLcc : ARC4InstBranch<0b00101,
    (outs), (ins brtarget20:$offset, i32imm:$q),
    "bl\t$offset", []> {
    let n = 0;
  }
}

let hasSideEffects = 1 in {
  def LP_insn : ARC4InstBranch<0b00110,
    (outs), (ins brtarget20:$offset),
    "lp\t$offset", []> {
    let n = 0;
    let q = 0;
  }

  def LP_cc : ARC4InstBranch<0b00110,
    (outs), (ins brtarget20:$offset, i32imm:$q),
    "lp\t$offset", []> {
    let n = 0;
  }
}

//===----------------------------------------------------------------------===//
// Jump Instructions  (opcode 7)
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
  def J_r : ARC4InstJumpReg<0,
    (outs), (ins GPR32:$b),
    "j\t$b", []> {
    let n = 0;
    let q = 0;
  }

  def J_l : ARC4InstJumpLimm<0,
    (outs), (ins limm32:$target),
    "j\t$target", []> {
    let n = 0;
    let q = 0;
  }
}

let isCall = 1 in {
  def JL_r : ARC4InstJumpReg<1,
    (outs), (ins GPR32:$b),
    "jl\t$b", []> {
    let n = 0;
    let q = 0;
  }

  def JL_l : ARC4InstJumpLimm<1,
    (outs), (ins limm32:$target),
    "jl\t$target", []> {
    let n = 0;
    let q = 0;
  }
}

//===----------------------------------------------------------------------===//
// Load Instructions  (opcode 0 = reg offset, opcode 1 = shimm offset)
//
// 6 variants total:
//   LD0: rr (reg+reg), rl (reg+limm), lr (limm+reg)
//   LD1: rs (reg+shimm), ss (shimm+shimm), l (limm base, shimm=0)
//===----------------------------------------------------------------------===//

let mayLoad = 1, hasSideEffects = 0 in {
  // --- Opcode 0: ld a, [b, c] ---
  def LD_rr : ARC4InstLD0_rr<
    (outs GPR32:$a), (ins GPR32:$b, GPR32:$c),
    "ld\t$a, [$b, $c]", []> {
    let e = 0;
    let w = 0;
    let z = 0;
    let x = 0;
  }

  // --- Opcode 0: ld a, [b, limm] ---
  def LD_rl : ARC4InstLD0_rl<
    (outs GPR32:$a), (ins GPR32:$b, limm32:$limm),
    "ld\t$a, [$b, $limm]", []> {
    let e = 0;
    let w = 0;
    let z = 0;
    let x = 0;
  }

  // --- Opcode 0: ld a, [limm, c] ---
  def LD_lr : ARC4InstLD0_lr<
    (outs GPR32:$a), (ins limm32:$limm, GPR32:$c),
    "ld\t$a, [$limm, $c]", []> {
    let e = 0;
    let z = 0;
    let x = 0;
  }

  // --- Opcode 1: ld a, [b, shimm] ---
  def LD_rs : ARC4InstLD1_rs<
    (outs GPR32:$a), (ins GPR32:$b, ld_offset9:$shimm),
    "ld\t$a, [$b, $shimm]", []> {
    let E = 0;
    let W = 0;
    let Z = 0;
    let X = 0;
  }

  // --- Opcode 1: ld a, [shimm, shimm] ---
  def LD_ss : ARC4InstLD1_ss<
    (outs GPR32:$a), (ins ld_offset9:$shimm),
    "ld\t$a, [$shimm]", []> {
    let E = 0;
    let Z = 0;
    let X = 0;
  }

  // --- Opcode 1: ld a, [limm] ---
  def LD_l : ARC4InstLD1_l<
    (outs GPR32:$a), (ins limm32:$limm),
    "ld\t$a, [$limm]", []> {
    let E = 0;
    let Z = 0;
    let X = 0;
  }
}

//===----------------------------------------------------------------------===//
// Store Instructions  (opcode 2)
//
// 7 variants:
//   rrs: st c, [b, offset]            (reg val, reg base, shimm offset)
//   srs: st shimm, [b, offset]        (shimm val, reg base, shimm offset)
//   rss: st c, [shimm, offset]        (reg val, shimm base, shimm offset)
//   sss: st shimm, [shimm, offset]    (all shimm)
//   rls: st c, [limm, offset]         (reg val, limm base, shimm offset)
//   lrs: st limm, [b, offset]         (limm val, reg base, shimm offset)
//   lls: st limm, [limm, offset]      (both limm, shimm offset)
//===----------------------------------------------------------------------===//

let mayStore = 1, hasSideEffects = 0 in {
  def ST_rrs : ARC4InstST_rrs<
    (outs), (ins GPR32:$c, GPR32:$b, st_offset9:$offset),
    "st\t$c, [$b, $offset]", []> {
    let D = 0;
    let v = 0;
    let y = 0;
  }

  def ST_srs : ARC4InstST_srs<
    (outs), (ins simm9:$offset, GPR32:$b),
    "st\t$offset, [$b, $offset]", []> {
    let D = 0;
    let v = 0;
    let y = 0;
  }

  def ST_rss : ARC4InstST_rss<
    (outs), (ins GPR32:$c, st_offset9:$offset),
    "st\t$c, [$offset]", []> {
    let D = 0;
    let y = 0;
  }

  def ST_sss : ARC4InstST_sss<
    (outs), (ins st_offset9:$offset),
    "st\t$offset, [$offset]", []> {
    let D = 0;
    let y = 0;
  }

  def ST_rls : ARC4InstST_rls<
    (outs), (ins GPR32:$c, limm32:$limm, st_offset9:$offset),
    "st\t$c, [$limm, $offset]", []> {
    let D = 0;
    let y = 0;
  }

  def ST_lrs : ARC4InstST_lrs<
    (outs), (ins limm32:$limm, GPR32:$b, st_offset9:$offset),
    "st\t$limm, [$b, $offset]", []> {
    let D = 0;
    let v = 0;
    let y = 0;
  }

  def ST_lls : ARC4InstST_lls<
    (outs), (ins limm32:$limm, st_offset9:$offset),
    "st\t$limm, [$limm, $offset]", []> {
    let D = 0;
    let y = 0;
  }
}

//===----------------------------------------------------------------------===//
// Flag Instruction  (opcode 3, A=61, C=0)
//===----------------------------------------------------------------------===//

let hasSideEffects = 1 in {
  def FLAG_r : ARC4InstFlag_r<
    (outs), (ins GPR32:$b),
    "flag\t$b", []>;

  def FLAG_s : ARC4InstFlag_s<
    (outs), (ins simm9:$shimm),
    "flag\t$shimm", []>;

  def FLAG_l : ARC4InstFlag_l<
    (outs), (ins limm32:$limm),
    "flag\t$limm", []>;
}

//===----------------------------------------------------------------------===//
// Special fixed-encoding instructions
//===----------------------------------------------------------------------===//

let hasSideEffects = 1 in {
  def BRK : ARC4Inst<(outs), (ins), "brk", []> {
    let Inst{31-0} = 0x1ffffe00;
  }

  def SLEEP : ARC4Inst<(outs), (ins), "sleep", []> {
    let Inst{31-0} = 0x1ffffe01;
  }

  def SWI : ARC4Inst<(outs), (ins), "swi", []> {
    let Inst{31-0} = 0x1ffffe02;
  }
}

//===----------------------------------------------------------------------===//
// Aliases
//===----------------------------------------------------------------------===//

// mov a, b => and a, b, b
def : InstAlias<"mov\t$a, $b", (AND_rrr GPR32:$a, GPR32:$b, GPR32:$b)>;

// mov a, shimm => and a, shimm, shimm  (rss form: both B and C are shimm sentinel)
def : InstAlias<"mov\t$a, $shimm", (AND_rss GPR32:$a, simm9:$shimm)>;

// asl as add alias (asl a, b => add a, b, b)
def : InstAlias<"asl\t$a, $b, $c", (ADD_rrr GPR32:$a, GPR32:$b, GPR32:$c)>;
def : InstAlias<"lsl\t$a, $b, $c", (ADD_rrr GPR32:$a, GPR32:$b, GPR32:$c)>;

// rlc a, b => adc a, b, b  (rotate left through carry)
def : InstAlias<"rlc\t$a, $b", (ADC_rrr GPR32:$a, GPR32:$b, GPR32:$b)>;

// j [reg] / jl [reg] — bracket syntax for jump through register
def : InstAlias<"j\t[$b]", (J_r GPR32:$b)>;
def : InstAlias<"jl\t[$b]", (JL_r GPR32:$b)>;

// ld a, [b] => ld a, [b, 0]  — single-register load with implicit 0 offset
def : InstAlias<"ld\t$a, [$b]", (LD_rs GPR32:$a, GPR32:$b, 0)>;

// st c, [b] => st c, [b, 0]  — single-register store with implicit 0 offset
def : InstAlias<"st\t$c, [$b]", (ST_rrs GPR32:$c, GPR32:$b, 0)>;
