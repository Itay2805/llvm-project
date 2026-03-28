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
  // Non-shimm forms: f wired to bit[8], q wired to bits[4:0]
  def _rrr : ARC4InstALU3_rrr<opcode,
    (outs GPR32:$a), (ins GPR32:$b, GPR32:$c, i32imm:$f, i32imm:$q),
    mnemonic # "\t$a, $b, $c", []>;

  // Shimm forms: f not wired (MCCodeEmitter flips sentinel), no q (bits overlap)
  def _rrs : ARC4InstALU3_rrs<opcode,
    (outs GPR32:$a), (ins GPR32:$b, simm9:$shimm, i32imm:$f),
    mnemonic # "\t$a, $b, $shimm", []>;

  def _rsr : ARC4InstALU3_rsr<opcode,
    (outs GPR32:$a), (ins simm9:$shimm, GPR32:$c, i32imm:$f),
    mnemonic # "\t$a, $shimm, $c", []>;

  def _rss : ARC4InstALU3_rss<opcode,
    (outs GPR32:$a), (ins simm9:$shimm, i32imm:$f),
    mnemonic # "\t$a, $shimm, $shimm", []>;

  // Limm forms: f wired to bit[8], q wired to bits[4:0]
  def _rrl : ARC4InstALU3_rrl<opcode,
    (outs GPR32:$a), (ins GPR32:$b, limm32:$limm, i32imm:$f, i32imm:$q),
    mnemonic # "\t$a, $b, $limm", []>;

  def _rlr : ARC4InstALU3_rlr<opcode,
    (outs GPR32:$a), (ins limm32:$limm, GPR32:$c, i32imm:$f, i32imm:$q),
    mnemonic # "\t$a, $limm, $c", []>;

  // --- Discard variants (A = sentinel 61 = SHIMM_UPDATE) ---
  let hasSideEffects = 1 in {
    def _0rr : ARC4InstALU3_rrr<opcode,
      (outs), (ins GPR32:$b, GPR32:$c, i32imm:$f, i32imm:$q),
      mnemonic # "\t0, $b, $c", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
    }

    def _0rs : ARC4InstALU3_rrs<opcode,
      (outs), (ins GPR32:$b, simm9:$shimm, i32imm:$f),
      mnemonic # "\t0, $b, $shimm", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
    }

    def _0sr : ARC4InstALU3_rsr<opcode,
      (outs), (ins simm9:$shimm, GPR32:$c, i32imm:$f),
      mnemonic # "\t0, $shimm, $c", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
    }

    def _0ss : ARC4InstALU3_rss<opcode,
      (outs), (ins simm9:$shimm, i32imm:$f),
      mnemonic # "\t0, $shimm, $shimm", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
    }

    def _0rl : ARC4InstALU3_rrl<opcode,
      (outs), (ins GPR32:$b, limm32:$limm, i32imm:$f, i32imm:$q),
      mnemonic # "\t0, $b, $limm", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
    }

    def _0lr : ARC4InstALU3_rlr<opcode,
      (outs), (ins limm32:$limm, GPR32:$c, i32imm:$f, i32imm:$q),
      mnemonic # "\t0, $limm, $c", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
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
  // Non-shimm forms: f wired to bit[8], q wired to bits[4:0]
  def _rr : ARC4InstSOP_rr<subop,
    (outs GPR32:$a), (ins GPR32:$b, i32imm:$f, i32imm:$q),
    mnemonic # "\t$a, $b", []>;

  // Shimm form: f not wired (MCCodeEmitter flips sentinel), no q
  def _rs : ARC4InstSOP_rs<subop,
    (outs GPR32:$a), (ins simm9:$shimm, i32imm:$f),
    mnemonic # "\t$a, $shimm", []>;

  // Limm form: f wired to bit[8], q wired to bits[4:0]
  def _rl : ARC4InstSOP_rl<subop,
    (outs GPR32:$a), (ins limm32:$limm, i32imm:$f, i32imm:$q),
    mnemonic # "\t$a, $limm", []>;

  // --- Discard variants ---
  let hasSideEffects = 1 in {
    def _0r : ARC4InstSOP_rr<subop,
      (outs), (ins GPR32:$b, i32imm:$f, i32imm:$q),
      mnemonic # "\t0, $b", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
    }

    def _0s : ARC4InstSOP_rs<subop,
      (outs), (ins simm9:$shimm, i32imm:$f),
      mnemonic # "\t0, $shimm", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
    }

    def _0l : ARC4InstSOP_rl<subop,
      (outs), (ins limm32:$limm, i32imm:$f, i32imm:$q),
      mnemonic # "\t0, $limm", []> {
      let a = 0b111111;  // sentinel 63 (no flag); MCCodeEmitter flips to 61 for .f
    }
  }
}

// Note: ASL does NOT exist as a single-operand instruction (subop 0 is
// reserved for the flag instruction). ASL is an alias for add a, b, b.
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
  def B : ARC4InstBranch<0b00100,
    (outs), (ins brtarget20:$offset, i32imm:$q, i32imm:$n),
    "b\t$offset", []>;
}

let isCall = 1 in {
  def BL : ARC4InstBranch<0b00101,
    (outs), (ins brtarget20:$offset, i32imm:$q, i32imm:$n),
    "bl\t$offset", []>;
}

let hasSideEffects = 1 in {
  def LP_insn : ARC4InstBranch<0b00110,
    (outs), (ins brtarget20:$offset, i32imm:$q, i32imm:$n),
    "lp\t$offset", []>;
}

//===----------------------------------------------------------------------===//
// Jump Instructions  (opcode 7)
//===----------------------------------------------------------------------===//

let isBranch = 1, isTerminator = 1, isBarrier = 1, isIndirectBranch = 1 in {
  def J_r : ARC4InstJumpReg<0,
    (outs), (ins GPR32:$b, i32imm:$f, i32imm:$q, i32imm:$n),
    "j\t$b", []>;

  def J_l : ARC4InstJumpLimm<0,
    (outs), (ins limm32:$target, i32imm:$f, i32imm:$q, i32imm:$n),
    "j\t$target", []>;
}

let isCall = 1 in {
  def JL_r : ARC4InstJumpReg<1,
    (outs), (ins GPR32:$b, i32imm:$f, i32imm:$q, i32imm:$n),
    "jl\t$b", []>;

  def JL_l : ARC4InstJumpLimm<1,
    (outs), (ins limm32:$target, i32imm:$f, i32imm:$q, i32imm:$n),
    "jl\t$target", []>;
}

//===----------------------------------------------------------------------===//
// Load Instructions  (opcode 0 = reg offset, opcode 1 = shimm offset)
//
// 6 variants total:
//   LD0: rr (reg+reg), rl (reg+limm), lr (limm+reg)
//   LD1: rs (reg+shimm), ss (shimm+shimm), l (limm base, shimm=0)
//===----------------------------------------------------------------------===//

multiclass LoadAll<string mnemonic, bits<2> size> {
  let mayLoad = 1, hasSideEffects = 0 in {
    // --- Opcode 0: ld a, [b, c] ---
    def _rr : ARC4InstLD0_rr<
      (outs GPR32:$a), (ins GPR32:$b, GPR32:$c),
      mnemonic # "\t$a, [$b, $c]", []> {
      let e = 0;
      let w = 0;
      let z = size;
      let x = 0;
    }

    // --- Opcode 0: ld a, [b, limm] ---
    def _rl : ARC4InstLD0_rl<
      (outs GPR32:$a), (ins GPR32:$b, limm32:$limm),
      mnemonic # "\t$a, [$b, $limm]", []> {
      let e = 0;
      let w = 0;
      let z = size;
      let x = 0;
    }

    // --- Opcode 0: ld a, [limm, c] ---
    def _lr : ARC4InstLD0_lr<
      (outs GPR32:$a), (ins limm32:$limm, GPR32:$c),
      mnemonic # "\t$a, [$limm, $c]", []> {
      let e = 0;
      let z = size;
      let x = 0;
    }

    // --- Opcode 1: ld a, [b, shimm] ---
    def _rs : ARC4InstLD1_rs<
      (outs GPR32:$a), (ins GPR32:$b, ld_offset9:$shimm),
      mnemonic # "\t$a, [$b, $shimm]", []> {
      let E = 0;
      let W = 0;
      let Z = size;
      let X = 0;
    }

    // --- Opcode 1: ld a, [shimm, shimm] ---
    def _ss : ARC4InstLD1_ss<
      (outs GPR32:$a), (ins ld_offset9:$shimm),
      mnemonic # "\t$a, [$shimm]", []> {
      let E = 0;
      let Z = size;
      let X = 0;
    }

    // --- Opcode 1: ld a, [limm] ---
    def _l : ARC4InstLD1_l<
      (outs GPR32:$a), (ins limm32:$limm),
      mnemonic # "\t$a, [$limm]", []> {
      let E = 0;
      let Z = size;
      let X = 0;
    }
  }
}

defm LD  : LoadAll<"ld",  0b00>;  // word
defm LDB : LoadAll<"ldb", 0b01>;  // byte
defm LDW : LoadAll<"ldw", 0b10>;  // halfword

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

multiclass StoreAll<string mnemonic, bits<2> size> {
  let mayStore = 1, hasSideEffects = 0 in {
    def _rrs : ARC4InstST_rrs<
      (outs), (ins GPR32:$c, GPR32:$b, st_offset9:$offset),
      mnemonic # "\t$c, [$b, $offset]", []> {
      let D = 0;
      let v = 0;
      let y = size;
    }

    def _srs : ARC4InstST_srs<
      (outs), (ins simm9:$offset, GPR32:$b),
      mnemonic # "\t$offset, [$b, $offset]", []> {
      let D = 0;
      let v = 0;
      let y = size;
    }

    def _rss : ARC4InstST_rss<
      (outs), (ins GPR32:$c, st_offset9:$offset),
      mnemonic # "\t$c, [$offset]", []> {
      let D = 0;
      let y = size;
    }

    def _sss : ARC4InstST_sss<
      (outs), (ins st_offset9:$offset),
      mnemonic # "\t$offset, [$offset]", []> {
      let D = 0;
      let y = size;
    }

    def _rls : ARC4InstST_rls<
      (outs), (ins GPR32:$c, limm32:$limm, st_offset9:$offset),
      mnemonic # "\t$c, [$limm, $offset]", []> {
      let D = 0;
      let y = size;
    }

    def _lrs : ARC4InstST_lrs<
      (outs), (ins limm32:$limm, GPR32:$b, st_offset9:$offset),
      mnemonic # "\t$limm, [$b, $offset]", []> {
      let D = 0;
      let v = 0;
      let y = size;
    }

    def _lls : ARC4InstST_lls<
      (outs), (ins limm32:$limm, st_offset9:$offset),
      mnemonic # "\t$limm, [$limm, $offset]", []> {
      let D = 0;
      let y = size;
    }

    def _sls : ARC4InstST_sls<
      (outs), (ins limm32:$limm, st_offset9:$offset),
      mnemonic # "\t$offset, [$limm, $offset]", []> {
      let D = 0;
      let y = size;
    }
  }
}

defm ST  : StoreAll<"st",  0b00>;  // word
defm STB : StoreAll<"stb", 0b01>;  // byte
defm STW : StoreAll<"stw", 0b10>;  // halfword

//===----------------------------------------------------------------------===//
// Flag Instruction  (opcode 3, A=61, C=0)
//===----------------------------------------------------------------------===//

let hasSideEffects = 1 in {
  def FLAG_r : ARC4InstFlag_r<
    (outs), (ins GPR32:$b, i32imm:$q),
    "flag\t$b", []>;

  def FLAG_s : ARC4InstFlag_s<
    (outs), (ins simm9:$shimm),
    "flag\t$shimm", []>;

  def FLAG_l : ARC4InstFlag_l<
    (outs), (ins limm32:$limm, i32imm:$q),
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
def : InstAlias<"mov\t$a, $b", (AND_rrr GPR32:$a, GPR32:$b, GPR32:$b, 0, 0)>;

// mov a, shimm => and a, shimm, shimm  (rss form: both B and C are shimm sentinel)
def : InstAlias<"mov\t$a, $shimm", (AND_rss GPR32:$a, simm9:$shimm, 0)>;

// asl/lsl as add alias — ASL does not exist as a single-operand instruction
// (opcode 3 subop 0 is the flag instruction). Instead:
//   asl a, b    => add a, b, b  (shift left by 1 = double)
//   asl a, b, c => add a, b, c  (3-operand form)
def : InstAlias<"asl\t$a, $b", (ADD_rrr GPR32:$a, GPR32:$b, GPR32:$b, 0, 0)>;
def : InstAlias<"asl\t$a, $imm", (ADD_rss GPR32:$a, simm9:$imm, 0)>;
def : InstAlias<"asl\t$a, $b, $c", (ADD_rrr GPR32:$a, GPR32:$b, GPR32:$c, 0, 0)>;
def : InstAlias<"lsl\t$a, $b", (ADD_rrr GPR32:$a, GPR32:$b, GPR32:$b, 0, 0)>;
def : InstAlias<"lsl\t$a, $imm", (ADD_rss GPR32:$a, simm9:$imm, 0)>;
def : InstAlias<"lsl\t$a, $b, $c", (ADD_rrr GPR32:$a, GPR32:$b, GPR32:$c, 0, 0)>;

// rlc a, b => adc a, b, b  (rotate left through carry)
def : InstAlias<"rlc\t$a, $b", (ADC_rrr GPR32:$a, GPR32:$b, GPR32:$b, 0, 0)>;

// j [reg] / jl [reg] — bracket syntax for jump through register
def : InstAlias<"j\t[$b]", (J_r GPR32:$b, 0, 0, 0)>;
def : InstAlias<"jl\t[$b]", (JL_r GPR32:$b, 0, 0, 0)>;

// ld a, [b] => ld a, [b, 0]  — single-register load with implicit 0 offset
def : InstAlias<"ld\t$a, [$b]", (LD_rs GPR32:$a, GPR32:$b, 0)>;
def : InstAlias<"ldb\t$a, [$b]", (LDB_rs GPR32:$a, GPR32:$b, 0)>;
def : InstAlias<"ldw\t$a, [$b]", (LDW_rs GPR32:$a, GPR32:$b, 0)>;

// st c, [b] => st c, [b, 0]  — single-register store with implicit 0 offset
def : InstAlias<"st\t$c, [$b]", (ST_rrs GPR32:$c, GPR32:$b, 0)>;
def : InstAlias<"stb\t$c, [$b]", (STB_rrs GPR32:$c, GPR32:$b, 0)>;
def : InstAlias<"stw\t$c, [$b]", (STW_rrs GPR32:$c, GPR32:$b, 0)>;

// st c, [limm] => st c, [limm, 0]  — store reg to absolute address
// Priority 0: must not shadow the register alias above.
def : InstAlias<"st\t$c, [$addr]", (ST_rls GPR32:$c, limm32:$addr, 0), 0>;
def : InstAlias<"stb\t$c, [$addr]", (STB_rls GPR32:$c, limm32:$addr, 0), 0>;
def : InstAlias<"stw\t$c, [$addr]", (STW_rls GPR32:$c, limm32:$addr, 0), 0>;

// Hidden rss aliases: 2-operand form (after parser collapses duplicate imms).
// These exist because the AsmMatcher can't parse "$shimm, $shimm" (tied operand
// in AsmString). The parser detects "op reg, imm, imm" with equal imms and
// collapses to "op reg, imm"; these aliases let the matcher find the rss def.
// Priority 0 = never used for printing (the instruction's own AsmString handles that).
def : InstAlias<"add\t$a, $shimm", (ADD_rss GPR32:$a, simm9:$shimm, 0), 0>;
def : InstAlias<"adc\t$a, $shimm", (ADC_rss GPR32:$a, simm9:$shimm, 0), 0>;
def : InstAlias<"sub\t$a, $shimm", (SUB_rss GPR32:$a, simm9:$shimm, 0), 0>;
def : InstAlias<"sbc\t$a, $shimm", (SBC_rss GPR32:$a, simm9:$shimm, 0), 0>;
def : InstAlias<"and\t$a, $shimm", (AND_rss GPR32:$a, simm9:$shimm, 0), 0>;
def : InstAlias<"or\t$a, $shimm",  (OR_rss  GPR32:$a, simm9:$shimm, 0), 0>;
def : InstAlias<"bic\t$a, $shimm", (BIC_rss GPR32:$a, simm9:$shimm, 0), 0>;
def : InstAlias<"xor\t$a, $shimm", (XOR_rss GPR32:$a, simm9:$shimm, 0), 0>;

// Hidden 0ss aliases: discard + shimm+shimm form (after parser collapses
// duplicate imms AND converts leading 0 to a token).
// "add 0, 5, 5" -> parser collapses to [add, 0(token), 5(imm)] -> 0ss.
def : InstAlias<"add\t0, $shimm", (ADD_0ss simm9:$shimm, 0), 0>;
def : InstAlias<"adc\t0, $shimm", (ADC_0ss simm9:$shimm, 0), 0>;
def : InstAlias<"sub\t0, $shimm", (SUB_0ss simm9:$shimm, 0), 0>;
def : InstAlias<"sbc\t0, $shimm", (SBC_0ss simm9:$shimm, 0), 0>;
def : InstAlias<"and\t0, $shimm", (AND_0ss simm9:$shimm, 0), 0>;
def : InstAlias<"or\t0, $shimm",  (OR_0ss  simm9:$shimm, 0), 0>;
def : InstAlias<"bic\t0, $shimm", (BIC_0ss simm9:$shimm, 0), 0>;
def : InstAlias<"xor\t0, $shimm", (XOR_0ss simm9:$shimm, 0), 0>;

