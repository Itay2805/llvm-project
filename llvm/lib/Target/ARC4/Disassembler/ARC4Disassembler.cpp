//===- ARC4Disassembler.cpp - Disassembler for ARC4 -----------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the ARC4 (ARCtangent-A4) disassembler.
//
// ARC4 instructions are 32 bits (little-endian), optionally followed by
// a 32-bit Long Immediate (LIMM) word when any register field contains
// sentinel 62 (0b111110). The disassembler manually decodes the instruction
// word because the sentinel-based variant system creates overlapping bit
// patterns that the TableGen auto-generated decoder cannot resolve.
//
// Sentinel values in 6-bit register fields:
//   61 = SHIMM_UPDATE (short immediate with flag update)
//   62 = LIMM (long immediate, extra 32-bit word follows)
//   63 = SHIMM (short immediate without flag update)
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/ARC4MCTargetDesc.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"

#define DEBUG_TYPE "arc4-disassembler"

using namespace llvm;

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

class ARC4Disassembler : public MCDisassembler {
public:
  ARC4Disassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  ~ARC4Disassembler() override = default;

  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;
};

} // end anonymous namespace

static MCDisassembler *createARC4Disassembler(const Target & /*T*/,
                                              const MCSubtargetInfo &STI,
                                              MCContext &Ctx) {
  return new ARC4Disassembler(STI, Ctx);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeARC4Disassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheARC4Target(),
                                         createARC4Disassembler);
}

//===----------------------------------------------------------------------===//
// Helpers
//===----------------------------------------------------------------------===//

// clang-format off
static const MCPhysReg GPR32DecoderTable[] = {
  ARC4::R0,  ARC4::R1,  ARC4::R2,  ARC4::R3,
  ARC4::R4,  ARC4::R5,  ARC4::R6,  ARC4::R7,
  ARC4::R8,  ARC4::R9,  ARC4::R10, ARC4::R11,
  ARC4::R12, ARC4::R13, ARC4::R14, ARC4::R15,
  ARC4::R16, ARC4::R17, ARC4::R18, ARC4::R19,
  ARC4::R20, ARC4::R21, ARC4::R22, ARC4::R23,
  ARC4::R24, ARC4::R25, ARC4::GP,  ARC4::FP,
  ARC4::SP,  ARC4::ILINK1, ARC4::ILINK2, ARC4::BLINK,
};
// clang-format on

static void addReg(MCInst &Inst, unsigned RegNo) {
  assert(RegNo <= 31);
  Inst.addOperand(MCOperand::createReg(GPR32DecoderTable[RegNo]));
}

static void addImm(MCInst &Inst, int64_t Imm) {
  Inst.addOperand(MCOperand::createImm(Imm));
}

/// Read a 32-bit little-endian word from the byte stream.
static bool readWord(ArrayRef<uint8_t> Bytes, unsigned Offset, uint32_t &Word) {
  if (Bytes.size() < Offset + 4)
    return false;
  Word = static_cast<uint32_t>(Bytes[Offset + 0]) |
         (static_cast<uint32_t>(Bytes[Offset + 1]) << 8) |
         (static_cast<uint32_t>(Bytes[Offset + 2]) << 16) |
         (static_cast<uint32_t>(Bytes[Offset + 3]) << 24);
  return true;
}

/// Extract bit fields from the instruction word.
static unsigned getOpcode5(uint32_t W) { return (W >> 27) & 0x1F; }
static unsigned getFieldA(uint32_t W) { return (W >> 21) & 0x3F; }
static unsigned getFieldB(uint32_t W) { return (W >> 15) & 0x3F; }
static unsigned getFieldC(uint32_t W) { return (W >> 9) & 0x3F; }
static unsigned getFieldF(uint32_t W) { return (W >> 8) & 0x1; }
static unsigned getFieldQ(uint32_t W) { return W & 0x1F; }
static unsigned getFieldN(uint32_t W) { return (W >> 5) & 0x3; }
static int32_t getShimm9(uint32_t W) { return SignExtend32<9>(W & 0x1FF); }

/// Is a 6-bit field value a shimm sentinel (61=SHIMM_UPDATE or 63=SHIMM)?
static bool isShimmSentinel(unsigned V) { return V == 61 || V == 63; }
/// Is a 6-bit field value the LIMM sentinel (62)?
static bool isLimmSentinel(unsigned V) { return V == 62; }

/// Determine if the instruction has a LIMM word (any reg field = 62).
static bool hasLimm(uint32_t W) {
  return isLimmSentinel(getFieldA(W)) || isLimmSentinel(getFieldB(W)) ||
         isLimmSentinel(getFieldC(W));
}

/// Is sentinel 61 (SHIMM_UPDATE)?  Means .f flag is set.
static bool isShimmUpdate(unsigned V) { return V == 61; }

//===----------------------------------------------------------------------===//
// ALU3 opcode table: maps 5-bit opcode to base instruction enum values.
// Indices: [opc - 8][variant]
// Variants: 0=rrr, 1=rrs, 2=rsr, 3=rss, 4=rrl, 5=rlr,
//           6=0rr, 7=0rs, 8=0sr, 9=0ss, 10=0rl, 11=0lr
//===----------------------------------------------------------------------===//

// clang-format off
struct ALU3OpcEntry {
  unsigned Rrr, Rrs, Rsr, Rss, Rrl, Rlr;
  unsigned Drr, Drs, Dsr, Dss, Drl, Dlr; // discard (0xx)
};

static const ALU3OpcEntry ALU3Table[] = {
  // ADD (opcode 8)
  { ARC4::ADD_rrr, ARC4::ADD_rrs, ARC4::ADD_rsr, ARC4::ADD_rss,
    ARC4::ADD_rrl, ARC4::ADD_rlr,
    ARC4::ADD_0rr, ARC4::ADD_0rs, ARC4::ADD_0sr, ARC4::ADD_0ss,
    ARC4::ADD_0rl, ARC4::ADD_0lr },
  // ADC (opcode 9)
  { ARC4::ADC_rrr, ARC4::ADC_rrs, ARC4::ADC_rsr, ARC4::ADC_rss,
    ARC4::ADC_rrl, ARC4::ADC_rlr,
    ARC4::ADC_0rr, ARC4::ADC_0rs, ARC4::ADC_0sr, ARC4::ADC_0ss,
    ARC4::ADC_0rl, ARC4::ADC_0lr },
  // SUB (opcode 10)
  { ARC4::SUB_rrr, ARC4::SUB_rrs, ARC4::SUB_rsr, ARC4::SUB_rss,
    ARC4::SUB_rrl, ARC4::SUB_rlr,
    ARC4::SUB_0rr, ARC4::SUB_0rs, ARC4::SUB_0sr, ARC4::SUB_0ss,
    ARC4::SUB_0rl, ARC4::SUB_0lr },
  // SBC (opcode 11)
  { ARC4::SBC_rrr, ARC4::SBC_rrs, ARC4::SBC_rsr, ARC4::SBC_rss,
    ARC4::SBC_rrl, ARC4::SBC_rlr,
    ARC4::SBC_0rr, ARC4::SBC_0rs, ARC4::SBC_0sr, ARC4::SBC_0ss,
    ARC4::SBC_0rl, ARC4::SBC_0lr },
  // AND (opcode 12)
  { ARC4::AND_rrr, ARC4::AND_rrs, ARC4::AND_rsr, ARC4::AND_rss,
    ARC4::AND_rrl, ARC4::AND_rlr,
    ARC4::AND_0rr, ARC4::AND_0rs, ARC4::AND_0sr, ARC4::AND_0ss,
    ARC4::AND_0rl, ARC4::AND_0lr },
  // OR (opcode 13)
  { ARC4::OR_rrr, ARC4::OR_rrs, ARC4::OR_rsr, ARC4::OR_rss,
    ARC4::OR_rrl, ARC4::OR_rlr,
    ARC4::OR_0rr, ARC4::OR_0rs, ARC4::OR_0sr, ARC4::OR_0ss,
    ARC4::OR_0rl, ARC4::OR_0lr },
  // BIC (opcode 14)
  { ARC4::BIC_rrr, ARC4::BIC_rrs, ARC4::BIC_rsr, ARC4::BIC_rss,
    ARC4::BIC_rrl, ARC4::BIC_rlr,
    ARC4::BIC_0rr, ARC4::BIC_0rs, ARC4::BIC_0sr, ARC4::BIC_0ss,
    ARC4::BIC_0rl, ARC4::BIC_0lr },
  // XOR (opcode 15)
  { ARC4::XOR_rrr, ARC4::XOR_rrs, ARC4::XOR_rsr, ARC4::XOR_rss,
    ARC4::XOR_rrl, ARC4::XOR_rlr,
    ARC4::XOR_0rr, ARC4::XOR_0rs, ARC4::XOR_0sr, ARC4::XOR_0ss,
    ARC4::XOR_0rl, ARC4::XOR_0lr },
};

//===----------------------------------------------------------------------===//
// SOP opcode table: maps 6-bit subop to instruction enum values.
//===----------------------------------------------------------------------===//

struct SOPEntry {
  unsigned Rr, Rs, Rl;
  unsigned Dr, Ds, Dl; // discard (0x)
};

// Index by subop value.  Only subops 1-8 are defined.
static const SOPEntry SOPTable[] = {
  // subop 0 = reserved (flag instruction uses opcode 3 with different A field)
  { 0, 0, 0, 0, 0, 0 },
  // ASR (subop 1)
  { ARC4::ASR_rr, ARC4::ASR_rs, ARC4::ASR_rl,
    ARC4::ASR_0r, ARC4::ASR_0s, ARC4::ASR_0l },
  // LSR (subop 2)
  { ARC4::LSR_rr, ARC4::LSR_rs, ARC4::LSR_rl,
    ARC4::LSR_0r, ARC4::LSR_0s, ARC4::LSR_0l },
  // ROR (subop 3)
  { ARC4::ROR_rr, ARC4::ROR_rs, ARC4::ROR_rl,
    ARC4::ROR_0r, ARC4::ROR_0s, ARC4::ROR_0l },
  // RRC (subop 4)
  { ARC4::RRC_rr, ARC4::RRC_rs, ARC4::RRC_rl,
    ARC4::RRC_0r, ARC4::RRC_0s, ARC4::RRC_0l },
  // SEXB (subop 5)
  { ARC4::SEXB_rr, ARC4::SEXB_rs, ARC4::SEXB_rl,
    ARC4::SEXB_0r, ARC4::SEXB_0s, ARC4::SEXB_0l },
  // SEXW (subop 6)
  { ARC4::SEXW_rr, ARC4::SEXW_rs, ARC4::SEXW_rl,
    ARC4::SEXW_0r, ARC4::SEXW_0s, ARC4::SEXW_0l },
  // EXTB (subop 7)
  { ARC4::EXTB_rr, ARC4::EXTB_rs, ARC4::EXTB_rl,
    ARC4::EXTB_0r, ARC4::EXTB_0s, ARC4::EXTB_0l },
  // EXTW (subop 8)
  { ARC4::EXTW_rr, ARC4::EXTW_rs, ARC4::EXTW_rl,
    ARC4::EXTW_0r, ARC4::EXTW_0s, ARC4::EXTW_0l },
};

//===----------------------------------------------------------------------===//
// Load opcode table: maps size (Z field) to instruction enums.
//===----------------------------------------------------------------------===//

struct LD0Entry {
  unsigned Rr, Rl, Lr;
};

struct LD1Entry {
  unsigned Rs, Ss, L;
};

// Index by Z (size): 0=word, 1=byte, 2=halfword
static const LD0Entry LD0Table[] = {
  { ARC4::LD_rr,  ARC4::LD_rl,  ARC4::LD_lr  },
  { ARC4::LDB_rr, ARC4::LDB_rl, ARC4::LDB_lr },
  { ARC4::LDW_rr, ARC4::LDW_rl, ARC4::LDW_lr },
};

static const LD1Entry LD1Table[] = {
  { ARC4::LD_rs,  ARC4::LD_ss,  ARC4::LD_l  },
  { ARC4::LDB_rs, ARC4::LDB_ss, ARC4::LDB_l },
  { ARC4::LDW_rs, ARC4::LDW_ss, ARC4::LDW_l },
};

//===----------------------------------------------------------------------===//
// Store opcode table: maps size (Y field) to instruction enums.
//===----------------------------------------------------------------------===//

struct STEntry {
  unsigned Rrs, Srs, Rss, Sss, Rls, Lrs, Lls, Sls;
};

static const STEntry STTable[] = {
  { ARC4::ST_rrs,  ARC4::ST_srs,  ARC4::ST_rss,  ARC4::ST_sss,
    ARC4::ST_rls,  ARC4::ST_lrs,  ARC4::ST_lls,  ARC4::ST_sls  },
  { ARC4::STB_rrs, ARC4::STB_srs, ARC4::STB_rss, ARC4::STB_sss,
    ARC4::STB_rls, ARC4::STB_lrs, ARC4::STB_lls, ARC4::STB_sls },
  { ARC4::STW_rrs, ARC4::STW_srs, ARC4::STW_rss, ARC4::STW_sss,
    ARC4::STW_rls, ARC4::STW_lrs, ARC4::STW_lls, ARC4::STW_sls },
};
// clang-format on

//===----------------------------------------------------------------------===//
// Manual decoder: ALU3 (opcodes 8-15)
//===----------------------------------------------------------------------===//

static DecodeStatus decodeALU3(MCInst &Inst, uint32_t W, uint32_t Limm,
                               uint64_t Addr) {
  unsigned Opc5 = getOpcode5(W);
  if (Opc5 < 8 || Opc5 > 15)
    return MCDisassembler::Fail;

  const ALU3OpcEntry &E = ALU3Table[Opc5 - 8];
  unsigned A = getFieldA(W);
  unsigned B = getFieldB(W);
  unsigned C = getFieldC(W);

  bool AIsDiscard = isShimmSentinel(A) || isLimmSentinel(A);
  bool BIsShimm = isShimmSentinel(B);
  bool BIsLimm = isLimmSentinel(B);
  bool CIsShimm = isShimmSentinel(C);
  bool CIsLimm = isLimmSentinel(C);

  int32_t Shimm = getShimm9(W);
  unsigned F = getFieldF(W);
  unsigned Q = getFieldQ(W);

  // Determine the .f flag:
  // - For shimm forms (sentinel 61/63 in B or C): sentinel 61 means .f=1,
  //   63 means .f=0. The f bit is NOT in bit[8] (it's part of shimm).
  // - For non-shimm forms: f = bit[8].
  // - For discard forms with shimm: same sentinel logic as above.
  unsigned FVal = F;
  if (BIsShimm || CIsShimm) {
    // .f is encoded by sentinel choice: 61 = .f, 63 = no .f
    FVal = (isShimmUpdate(B) || isShimmUpdate(C)) ? 1 : 0;
  }

  // Discard forms: A is sentinel (61 or 63), no destination register.
  // The A sentinel also encodes .f for discard non-shimm forms.
  if (AIsDiscard) {
    if (BIsShimm && CIsShimm) {
      // 0ss: op 0, shimm, shimm (both B and C sentinel)
      Inst.setOpcode(E.Dss);
      addImm(Inst, Shimm); // shimm
      addImm(Inst, FVal);  // f
    } else if (CIsShimm) {
      // 0rs: op 0, b, shimm (C sentinel)
      Inst.setOpcode(E.Drs);
      addReg(Inst, B);     // b
      addImm(Inst, Shimm); // shimm
      addImm(Inst, FVal);  // f
    } else if (BIsShimm) {
      // 0sr: op 0, shimm, c (B sentinel)
      Inst.setOpcode(E.Dsr);
      addImm(Inst, Shimm); // shimm
      addReg(Inst, C);     // c
      addImm(Inst, FVal);  // f
    } else if (CIsLimm) {
      // 0rl: op 0, b, limm
      Inst.setOpcode(E.Drl);
      addReg(Inst, B);     // b
      addImm(Inst, Limm);  // limm
      addImm(Inst, FVal);  // f
      addImm(Inst, Q);     // q
    } else if (BIsLimm) {
      // 0lr: op 0, limm, c
      Inst.setOpcode(E.Dlr);
      addImm(Inst, Limm);  // limm
      addReg(Inst, C);     // c
      addImm(Inst, FVal);  // f
      addImm(Inst, Q);     // q
    } else {
      // 0rr: op 0, b, c
      Inst.setOpcode(E.Drr);
      addReg(Inst, B);     // b
      addReg(Inst, C);     // c
      addImm(Inst, FVal);  // f
      addImm(Inst, Q);     // q
    }
  } else {
    // Result-producing variants: A is a destination register.
    if (A > 31)
      return MCDisassembler::Fail;

    if (BIsShimm && CIsShimm) {
      // rss: op a, shimm, shimm
      Inst.setOpcode(E.Rss);
      addReg(Inst, A);     // a
      addImm(Inst, Shimm); // shimm
      addImm(Inst, FVal);  // f
    } else if (CIsShimm) {
      // rrs: op a, b, shimm
      Inst.setOpcode(E.Rrs);
      addReg(Inst, A);     // a
      addReg(Inst, B);     // b
      addImm(Inst, Shimm); // shimm
      addImm(Inst, FVal);  // f
    } else if (BIsShimm) {
      // rsr: op a, shimm, c
      Inst.setOpcode(E.Rsr);
      addReg(Inst, A);     // a
      addImm(Inst, Shimm); // shimm
      addReg(Inst, C);     // c
      addImm(Inst, FVal);  // f
    } else if (CIsLimm) {
      // rrl: op a, b, limm
      Inst.setOpcode(E.Rrl);
      addReg(Inst, A);     // a
      addReg(Inst, B);     // b
      addImm(Inst, Limm);  // limm
      addImm(Inst, FVal);  // f
      addImm(Inst, Q);     // q
    } else if (BIsLimm) {
      // rlr: op a, limm, c
      Inst.setOpcode(E.Rlr);
      addReg(Inst, A);     // a
      addImm(Inst, Limm);  // limm
      addReg(Inst, C);     // c
      addImm(Inst, FVal);  // f
      addImm(Inst, Q);     // q
    } else {
      // rrr: op a, b, c
      if (B > 31 || C > 31)
        return MCDisassembler::Fail;
      Inst.setOpcode(E.Rrr);
      addReg(Inst, A);     // a
      addReg(Inst, B);     // b
      addReg(Inst, C);     // c
      addImm(Inst, FVal);  // f
      addImm(Inst, Q);     // q
    }
  }
  return MCDisassembler::Success;
}

//===----------------------------------------------------------------------===//
// Manual decoder: SOP (opcode 3) and FLAG (opcode 3, A=61)
//===----------------------------------------------------------------------===//

static DecodeStatus decodeSOP(MCInst &Inst, uint32_t W, uint32_t Limm,
                              uint64_t Addr) {
  unsigned A = getFieldA(W);
  unsigned B = getFieldB(W);
  unsigned C = getFieldC(W);
  unsigned F = getFieldF(W);
  unsigned Q = getFieldQ(W);
  int32_t Shimm = getShimm9(W);

  // FLAG instruction: A = 61 (SHIMM_UPDATE sentinel), C = 0.
  if (A == 61 && C == 0) {
    if (isLimmSentinel(B)) {
      // flag limm
      Inst.setOpcode(ARC4::FLAG_l);
      addImm(Inst, Limm);  // limm
      addImm(Inst, Q);     // q
    } else if (isShimmSentinel(B)) {
      // flag shimm
      Inst.setOpcode(ARC4::FLAG_s);
      addImm(Inst, Shimm);  // shimm
    } else {
      // flag reg
      if (B > 31)
        return MCDisassembler::Fail;
      Inst.setOpcode(ARC4::FLAG_r);
      addReg(Inst, B);     // b
      addImm(Inst, Q);     // q
    }
    return MCDisassembler::Success;
  }

  // SOP instructions: subop in C[14:9] field.
  unsigned SubOp = C;
  if (SubOp == 0 || SubOp > 8)
    return MCDisassembler::Fail;

  const SOPEntry &E = SOPTable[SubOp];

  bool AIsDiscard = isShimmSentinel(A) || isLimmSentinel(A);
  bool BIsShimm = isShimmSentinel(B);
  bool BIsLimm = isLimmSentinel(B);

  unsigned FVal = F;
  if (BIsShimm)
    FVal = isShimmUpdate(B) ? 1 : 0;

  if (AIsDiscard) {
    // Discard variants: no destination register.
    if (BIsShimm) {
      Inst.setOpcode(E.Ds);
      addImm(Inst, Shimm);  // shimm
      addImm(Inst, FVal);   // f
    } else if (BIsLimm) {
      Inst.setOpcode(E.Dl);
      addImm(Inst, Limm);   // limm
      addImm(Inst, FVal);   // f
      addImm(Inst, Q);      // q
    } else {
      if (B > 31)
        return MCDisassembler::Fail;
      Inst.setOpcode(E.Dr);
      addReg(Inst, B);      // b
      addImm(Inst, FVal);   // f
      addImm(Inst, Q);      // q
    }
  } else {
    // Result-producing variants.
    if (A > 31)
      return MCDisassembler::Fail;

    if (BIsShimm) {
      Inst.setOpcode(E.Rs);
      addReg(Inst, A);      // a
      addImm(Inst, Shimm);  // shimm
      addImm(Inst, FVal);   // f
    } else if (BIsLimm) {
      Inst.setOpcode(E.Rl);
      addReg(Inst, A);      // a
      addImm(Inst, Limm);   // limm
      addImm(Inst, FVal);   // f
      addImm(Inst, Q);      // q
    } else {
      if (B > 31)
        return MCDisassembler::Fail;
      Inst.setOpcode(E.Rr);
      addReg(Inst, A);      // a
      addReg(Inst, B);      // b
      addImm(Inst, FVal);   // f
      addImm(Inst, Q);      // q
    }
  }
  return MCDisassembler::Success;
}

//===----------------------------------------------------------------------===//
// Manual decoder: Branch (opcodes 4, 5, 6)
//===----------------------------------------------------------------------===//

static DecodeStatus decodeBranch(MCInst &Inst, uint32_t W, uint64_t Addr) {
  unsigned Opc5 = getOpcode5(W);
  switch (Opc5) {
  case 4: Inst.setOpcode(ARC4::B); break;
  case 5: Inst.setOpcode(ARC4::BL); break;
  case 6: Inst.setOpcode(ARC4::LP_insn); break;
  default: return MCDisassembler::Fail;
  }

  // offset[26:7] = 20-bit signed offset; target = PC + 4 + (offset << 2)
  unsigned RawOffset = (W >> 7) & 0xFFFFF;
  int32_t Offset = SignExtend32<20>(RawOffset);
  int64_t Target = static_cast<int64_t>(Addr) + 4 +
                   (static_cast<int64_t>(Offset) << 2);
  addImm(Inst, Target);            // branch target
  addImm(Inst, getFieldQ(W));      // q (condition code)
  addImm(Inst, getFieldN(W));      // n (delay slot)

  return MCDisassembler::Success;
}

//===----------------------------------------------------------------------===//
// Manual decoder: Jump (opcode 7)
//===----------------------------------------------------------------------===//

static DecodeStatus decodeJump(MCInst &Inst, uint32_t W, uint32_t Limm,
                               uint64_t Addr) {
  unsigned B = getFieldB(W);
  unsigned Link = (W >> 9) & 1;
  unsigned F = getFieldF(W);
  unsigned Q = getFieldQ(W);
  unsigned N = getFieldN(W);

  if (isLimmSentinel(B)) {
    // Jump via LIMM. LIMM stores address >> 2, so multiply by 4.
    uint32_t Target = Limm << 2;
    Inst.setOpcode(Link ? ARC4::JL_l : ARC4::J_l);
    addImm(Inst, Target);  // target
    addImm(Inst, F);       // f
    addImm(Inst, Q);       // q
    addImm(Inst, N);       // n
  } else {
    // Jump via register.
    if (B > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(Link ? ARC4::JL_r : ARC4::J_r);
    addReg(Inst, B);       // b
    addImm(Inst, F);       // f
    addImm(Inst, Q);       // q
    addImm(Inst, N);       // n
  }
  return MCDisassembler::Success;
}

//===----------------------------------------------------------------------===//
// Manual decoder: Load opcode 0 (reg + reg/limm offset)
//===----------------------------------------------------------------------===//

static DecodeStatus decodeLD0(MCInst &Inst, uint32_t W, uint32_t Limm) {
  unsigned A = getFieldA(W);
  unsigned B = getFieldB(W);
  unsigned C = getFieldC(W);
  unsigned Z = (W >> 1) & 0x3;  // size bits [2:1]

  if (Z > 2)
    return MCDisassembler::Fail;
  if (A > 31)
    return MCDisassembler::Fail;

  const LD0Entry &E = LD0Table[Z];

  if (isLimmSentinel(C)) {
    // ld a, [b, limm]
    if (B > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Rl);
    addReg(Inst, A);     // a
    addReg(Inst, B);     // b
    addImm(Inst, Limm);  // limm
  } else if (isLimmSentinel(B)) {
    // ld a, [limm, c]
    if (C > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Lr);
    addReg(Inst, A);     // a
    addImm(Inst, Limm);  // limm
    addReg(Inst, C);     // c
  } else {
    // ld a, [b, c]
    if (B > 31 || C > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Rr);
    addReg(Inst, A);     // a
    addReg(Inst, B);     // b
    addReg(Inst, C);     // c
  }
  return MCDisassembler::Success;
}

//===----------------------------------------------------------------------===//
// Manual decoder: Load opcode 1 (reg + shimm offset)
//===----------------------------------------------------------------------===//

static DecodeStatus decodeLD1(MCInst &Inst, uint32_t W, uint32_t Limm) {
  unsigned A = getFieldA(W);
  unsigned B = getFieldB(W);
  unsigned Z = (W >> 10) & 0x3;  // size bits [11:10]
  int32_t Shimm = getShimm9(W);

  if (Z > 2)
    return MCDisassembler::Fail;
  if (A > 31)
    return MCDisassembler::Fail;

  const LD1Entry &E = LD1Table[Z];

  if (isLimmSentinel(B)) {
    // ld a, [limm] (shimm=0 implied)
    Inst.setOpcode(E.L);
    addReg(Inst, A);     // a
    addImm(Inst, Limm);  // limm
  } else if (isShimmSentinel(B)) {
    // ld a, [shimm, shimm] (effective addr = 2*shimm)
    Inst.setOpcode(E.Ss);
    addReg(Inst, A);     // a
    addImm(Inst, Shimm); // shimm (ld_ss_addr, printer multiplies by 2)
  } else {
    // ld a, [b, shimm]
    if (B > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Rs);
    addReg(Inst, A);     // a
    addReg(Inst, B);     // b
    addImm(Inst, Shimm); // shimm offset
  }
  return MCDisassembler::Success;
}

//===----------------------------------------------------------------------===//
// Manual decoder: Store opcode 2
//===----------------------------------------------------------------------===//

static DecodeStatus decodeST(MCInst &Inst, uint32_t W, uint32_t Limm) {
  unsigned B = getFieldB(W);
  unsigned C = getFieldC(W);
  unsigned Y = (W >> 22) & 0x3;  // size bits [23:22]
  int32_t Offset = getShimm9(W);

  if (Y > 2)
    return MCDisassembler::Fail;

  const STEntry &E = STTable[Y];

  bool BIsShimm = isShimmSentinel(B);
  bool BIsLimm = isLimmSentinel(B);
  bool CIsShimm = isShimmSentinel(C);
  bool CIsLimm = isLimmSentinel(C);

  if (BIsLimm && CIsLimm) {
    // st limm, [limm, offset]
    Inst.setOpcode(E.Lls);
    addImm(Inst, Limm);    // limm (both value and base)
    addImm(Inst, Offset);  // offset
  } else if (BIsLimm && CIsShimm) {
    // st shimm, [limm, offset]
    Inst.setOpcode(E.Sls);
    addImm(Inst, Limm);    // limm (base)
    addImm(Inst, Offset);  // offset (= shimm value)
  } else if (BIsLimm) {
    // st c, [limm, offset]
    if (C > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Rls);
    addReg(Inst, C);       // c (value)
    addImm(Inst, Limm);    // limm (base)
    addImm(Inst, Offset);  // offset
  } else if (CIsLimm) {
    // st limm, [b, offset]
    if (B > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Lrs);
    addImm(Inst, Limm);    // limm (value)
    addReg(Inst, B);       // b (base)
    addImm(Inst, Offset);  // offset
  } else if (BIsShimm && CIsShimm) {
    // st shimm, [shimm, offset]
    Inst.setOpcode(E.Sss);
    addImm(Inst, Offset);  // offset (= shimm value = base shimm)
  } else if (BIsShimm) {
    // st c, [shimm, offset]
    if (C > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Rss);
    addReg(Inst, C);       // c (value)
    addImm(Inst, Offset);  // offset
  } else if (CIsShimm) {
    // st shimm, [b, offset]
    if (B > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Srs);
    addImm(Inst, Offset);  // offset (= shimm value)
    addReg(Inst, B);       // b (base)
  } else {
    // st c, [b, offset]
    if (B > 31 || C > 31)
      return MCDisassembler::Fail;
    Inst.setOpcode(E.Rrs);
    addReg(Inst, C);       // c (value)
    addReg(Inst, B);       // b (base)
    addImm(Inst, Offset);  // offset
  }
  return MCDisassembler::Success;
}

//===----------------------------------------------------------------------===//
// Manual decoder: Special fixed-encoding instructions
//===----------------------------------------------------------------------===//

static DecodeStatus decodeSpecial(MCInst &Inst, uint32_t W) {
  switch (W) {
  case 0x7fffffff:
    Inst.setOpcode(ARC4::NOP);
    return MCDisassembler::Success;
  case 0x1ffffe00:
    Inst.setOpcode(ARC4::BRK);
    return MCDisassembler::Success;
  case 0x1ffffe01:
    Inst.setOpcode(ARC4::SLEEP);
    return MCDisassembler::Success;
  case 0x1ffffe02:
    Inst.setOpcode(ARC4::SWI);
    return MCDisassembler::Success;
  default:
    return MCDisassembler::Fail;
  }
}

//===----------------------------------------------------------------------===//
// Main decode entry point
//===----------------------------------------------------------------------===//

DecodeStatus
ARC4Disassembler::getInstruction(MCInst &Instr, uint64_t &Size,
                                 ArrayRef<uint8_t> Bytes, uint64_t Address,
                                 raw_ostream & /*CStream*/) const {
  uint32_t FirstWord;
  if (!readWord(Bytes, 0, FirstWord)) {
    Size = 0;
    return MCDisassembler::Fail;
  }

  // Check for LIMM word.
  uint32_t LimmWord = 0;
  bool HasLimmWord = hasLimm(FirstWord);
  if (HasLimmWord) {
    if (!readWord(Bytes, 4, LimmWord)) {
      Size = 4;
      return MCDisassembler::Fail;
    }
  }

  // Try special fixed-encoding instructions first (they may share opcode
  // bits with other instructions, so exact match is needed).
  DecodeStatus Result = decodeSpecial(Instr, FirstWord);
  if (Result == MCDisassembler::Success) {
    Size = 4;
    return Result;
  }

  // Dispatch by major opcode (bits [31:27]).
  unsigned Opc5 = getOpcode5(FirstWord);

  switch (Opc5) {
  case 0: // Load opcode 0 (reg+reg)
    Result = decodeLD0(Instr, FirstWord, LimmWord);
    break;
  case 1: // Load opcode 1 (reg+shimm)
    Result = decodeLD1(Instr, FirstWord, LimmWord);
    break;
  case 2: // Store
    Result = decodeST(Instr, FirstWord, LimmWord);
    break;
  case 3: // SOP / FLAG
    Result = decodeSOP(Instr, FirstWord, LimmWord, Address);
    break;
  case 4: // B
  case 5: // BL
  case 6: // LP
    Result = decodeBranch(Instr, FirstWord, Address);
    break;
  case 7: // Jump
    Result = decodeJump(Instr, FirstWord, LimmWord, Address);
    break;
  case 8: case 9: case 10: case 11:
  case 12: case 13: case 14: case 15: // ALU3
    Result = decodeALU3(Instr, FirstWord, LimmWord, Address);
    break;
  default:
    Result = MCDisassembler::Fail;
    break;
  }

  if (Result == MCDisassembler::Fail) {
    Size = 4;
    return MCDisassembler::Fail;
  }

  Size = HasLimmWord ? 8 : 4;
  return Result;
}
