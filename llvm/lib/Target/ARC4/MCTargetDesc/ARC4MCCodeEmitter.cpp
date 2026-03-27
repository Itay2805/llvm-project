//===-- ARC4MCCodeEmitter.cpp - ARC4 Instruction Encoding -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// MCInst operand layouts (must match ARC4AsmParser):
//
//   RegFmt    : A(reg), B(reg), C(reg), F(imm), NN(imm), Q(imm)
//   ShimmFmt  : A(reg), B(reg), D(imm9), SetFlags(imm), NN(imm)
//   LimmFmt   : A(reg), B(reg), LImm(imm32/expr), F(imm), NN(imm), Q(imm)
//   BranchFmt : Target(expr/imm), NN(imm), Q(imm)
//
//   Single-operand RegFmt (FLAG/ASR/...):
//               A(reg), B(reg), F(imm), Q(imm)    [C = sub-opcode from TSFlags]
//   Single-operand LimmFmt:
//               A(reg), LImm(imm32/expr), F(imm), Q(imm)
//   No-operand (BRK/SLEEP/SWI/NOP): 0 operands; encoding fixed in TSFlags.
//
//   Store RegFmt: C(reg), B(reg), A(reg), F(imm), NN(imm), Q(imm)
//   Store ShimmFmt: C(reg), B(reg), D(imm9), SetFlags(imm), NN(imm)
//   Store LimmFmt:  C(reg), LImm(imm32/expr), F(imm), NN(imm), Q(imm)
//
//   Jump RegFmt:  B(reg), F(imm), NN(imm), Q(imm)
//   Jump LimmFmt: LImm(imm32/expr), F(imm), NN(imm), Q(imm)
//
// TSFlags layout (ARC4InstrFormats.td / ARC4InstrInfo.td):
//   [4:0]  5-bit ARC4 opcode
//   [6:5]  format: 0=Reg, 1=Shimm, 2=Limm, 3=Branch
//   [11:7] single-operand sub-opcode (C field, opcode 0x03)
//   [12]   isJL  (jump-and-link: A=31)
//   [13]   BRK/SLEEP B field (0,1,2); for SWI uses [14:13]=2
//   [13:12] ZZ bits for load/store (00=32, 01=byte, 10=word)
//   [14]   X bit (sign-extend for loads)
//   [15]   isLR/isSR flag
//   [16]   isNOP
//===----------------------------------------------------------------------===//

#include "ARC4MCTargetDesc.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/EndianStream.h"

using namespace llvm;

#define DEBUG_TYPE "mccodeemitter"

// Fixup kind for 20-bit branch offset in L[21:2] field (bits[26:7]).
enum ARC4FixupKind {
  FK_ARC4_Branch20 = llvm::FirstTargetFixupKind,
};

namespace {

class ARC4MCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

public:
  ARC4MCCodeEmitter(const MCInstrInfo &MII, MCContext &C) : MCII(MII), Ctx(C) {}
  ARC4MCCodeEmitter(const ARC4MCCodeEmitter &) = delete;
  ARC4MCCodeEmitter &operator=(const ARC4MCCodeEmitter &) = delete;
  ~ARC4MCCodeEmitter() override = default;

  void encodeInstruction(const MCInst &MI, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

private:
  // Map MCRegister enum → 6-bit hardware encoding.
  // ARC4RegisterInfo.td defines r0..r63 with HWEncoding=0..63;
  // TableGen assigns enum values 1..64 in order, so enc = reg_enum - 1.
  static uint32_t regEnc(const MCOperand &MO) { return MO.getReg() - 1; }

  // Emit a 32-bit limm word (constant or fixup) and return it.
  void emitLimm(const MCOperand &Op, unsigned FixupOff,
                SmallVectorImpl<MCFixup> &Fixups,
                SmallVectorImpl<char> &CB) const;
};

} // namespace

void ARC4MCCodeEmitter::emitLimm(const MCOperand &Op, unsigned FixupOff,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  SmallVectorImpl<char> &CB) const {
  if (Op.isImm()) {
    support::endian::write<uint32_t>(CB, (uint32_t)Op.getImm(),
                                     llvm::endianness::little);
  } else {
    assert(Op.isExpr());
    Fixups.push_back(MCFixup::create(FixupOff, Op.getExpr(), FK_Data_4));
    support::endian::write<uint32_t>(CB, 0, llvm::endianness::little);
  }
}

void ARC4MCCodeEmitter::encodeInstruction(const MCInst &MI,
                                           SmallVectorImpl<char> &CB,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  unsigned Opc = MI.getOpcode();

  // Handle codegen-only MOV pseudo-instructions directly.
  // These bypass the normal format dispatch since they need special encoding.
  if (Opc == ARC4::CG_MOVri) {
    // MOV dst, shimm → AND dst, shimm, shimm (shimms match)
    // Encoding: I=0x0C(AND), A=dst, B=63(shimm-no-flag), C=63, D=value
    uint32_t A = regEnc(MI.getOperand(0));
    uint32_t D = (uint32_t)MI.getOperand(1).getImm() & 0x1FF;
    uint32_t Word = (0x0Cu << 27) | (A << 21) | (63u << 15) | (63u << 9) | D;
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  if (Opc == ARC4::CG_MOVli) {
    // MOV dst, limm → AND dst, limm (B=62, C=62)
    uint32_t A = regEnc(MI.getOperand(0));
    uint32_t Word = (0x0Cu << 27) | (A << 21) | (62u << 15) | (62u << 9);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    emitLimm(MI.getOperand(1), /*FixupOff=*/4, Fixups, CB);
    return;
  }

  // Handle codegen-only load/store (may have different operand order than MC forms)
  if (Opc == ARC4::CG_LDri) {
    // CG_LDri: dst(reg), base(reg), offset(imm) → LD shimm: A=dst, B=base, D=offset
    uint32_t A = regEnc(MI.getOperand(0));
    uint32_t B = regEnc(MI.getOperand(1));
    uint32_t D = (uint32_t)MI.getOperand(2).getImm() & 0x1FF;
    uint32_t Word = (0x01u << 27) | (A << 21) | (B << 15) | (D & 0x1FF);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  if (Opc == ARC4::CG_STri) {
    // CG_STri: src(reg), base(reg), offset(imm) → ST shimm: A=0(flags), B=base, C=src, D=offset
    uint32_t C = regEnc(MI.getOperand(0));
    uint32_t B = regEnc(MI.getOperand(1));
    uint32_t D = (uint32_t)MI.getOperand(2).getImm() & 0x1FF;
    uint32_t Word = (0x02u << 27) | (B << 15) | (C << 9) | (D & 0x1FF);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  if (Opc == ARC4::CG_LDrr) {
    // CG_LDrr: dst(reg), base(reg), off(reg)
    uint32_t A = regEnc(MI.getOperand(0));
    uint32_t B = regEnc(MI.getOperand(1));
    uint32_t Cv = regEnc(MI.getOperand(2));
    uint32_t Word = (0x00u << 27) | (A << 21) | (B << 15) | (Cv << 9);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  // Handle codegen-only call/compare/branch/return instructions.
  if (Opc == ARC4::CG_CALLi) {
    // JL [limm]: I=7, A=31(blink), B=62(limm), C=0
    uint32_t Word = (0x07u << 27) | (31u << 21) | (62u << 15);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    emitLimm(MI.getOperand(0), /*FixupOff=*/4, Fixups, CB);
    return;
  }
  if (Opc == ARC4::CG_CALLr) {
    // JL [reg]: I=7, A=31(blink), B=reg, C=0
    uint32_t B = regEnc(MI.getOperand(0));
    uint32_t Word = (0x07u << 27) | (31u << 21) | (B << 15);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  // Shift-by-1: ASR/LSR (single operand, opcode 0x03)
  if (Opc == ARC4::CG_ASR) {
    uint32_t A = regEnc(MI.getOperand(0)); // dst
    uint32_t B = regEnc(MI.getOperand(1)); // src
    // I=0x03, C=1(ASR sub-opcode)
    uint32_t Word = (0x03u << 27) | (A << 21) | (B << 15) | (1u << 9);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  if (Opc == ARC4::CG_LSR) {
    uint32_t A = regEnc(MI.getOperand(0));
    uint32_t B = regEnc(MI.getOperand(1));
    // I=0x03, C=2(LSR sub-opcode)
    uint32_t Word = (0x03u << 27) | (A << 21) | (B << 15) | (2u << 9);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  if (Opc == ARC4::CG_CMPrr) {
    // SUB.F 0, src1, src2: I=0xA, A=63(discard), F=1
    uint32_t B = regEnc(MI.getOperand(0));
    uint32_t C = regEnc(MI.getOperand(1));
    uint32_t Word = (0x0Au << 27) | (63u << 21) | (B << 15) | (C << 9) | (1u << 8);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  if (Opc == ARC4::CG_CMPri) {
    // SUB.F 0, src, shimm: I=0xA, A=63, C=61(shimm+flags), D=value
    uint32_t B = regEnc(MI.getOperand(0));
    uint32_t D = (uint32_t)MI.getOperand(1).getImm() & 0x1FF;
    uint32_t Word = (0x0Au << 27) | (63u << 21) | (B << 15) | (61u << 9) | D;
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  if (Opc == ARC4::CG_RET) {
    // J [blink]: I=7, A=0, B=31, C=0
    uint32_t Word = (0x07u << 27) | (31u << 15);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  // Skip any pseudo instructions that somehow reach the encoder
  if (MCII.get(Opc).isPseudo())
    return;

  const MCInstrDesc &Desc = MCII.get(Opc);
  uint64_t TSF = Desc.TSFlags;

  uint32_t Arc4Op = TSF & 0x1F;         // 5-bit ARC4 instruction opcode
  uint32_t Fmt    = (TSF >> 5) & 0x3;   // format: 0=Reg, 1=Shimm, 2=Limm, 3=Branch

  uint32_t Word = Arc4Op << 27;

  if (Fmt == 3) {
    // ---- Branch format ----
    // MCInst: Target(0), NN(1), Q(2)
    uint32_t NN = (uint32_t)MI.getOperand(1).getImm() & 0x3;
    uint32_t Q  = (uint32_t)MI.getOperand(2).getImm() & 0x1F;
    Word |= (NN << 5) | Q;

    const MCOperand &TgtOp = MI.getOperand(0);
    if (TgtOp.isImm()) {
      int64_t Offset = TgtOp.getImm(); // byte offset from next PC
      uint32_t L = (uint32_t)((Offset >> 2) & 0xFFFFF);
      Word |= (L << 7);
    } else {
      // Create a PC-relative fixup. For locally-resolved fixups, applyFixup
      // subtracts 4 for the ARC4 PC+4 convention. For linker-emitted
      // relocations, we bake the -4 into the expression so the addend is
      // correct: the linker computes S + A - P and we need S - P - 4.
      const MCExpr *Adjusted = MCBinaryExpr::createSub(
          TgtOp.getExpr(),
          MCConstantExpr::create(4, Ctx),
          Ctx);
      Fixups.push_back(MCFixup::create(
          0, Adjusted, MCFixupKind(FK_ARC4_Branch20),
          /*PCRel=*/true));
    }
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  if (Fmt == 2) {
    // ---- Long-immediate format ----
    // C field = 62 (limm sentinel) always.
    Word |= (62u << 9);

    bool isNOP  = (TSF >> 16) & 1;
    bool isJL   = (TSF >> 12) & 1;
    bool isAuxFlag = (TSF >> 15) & 1;
    bool isLR   = isAuxFlag && (Arc4Op == 0x01);
    bool isSR   = isAuxFlag && (Arc4Op == 0x02);
    bool isSO   = (Arc4Op == 0x03) && !(isLR || isSR);

    if (isNOP) {
      // NOP: XOR r0, r63, r63 — shimm form with D=0
      // Actually NOP is in ShimmFmt, but handle the Limm branch gracefully
      support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
      return;
    }

    // Determine operand positions based on instruction type.
    // Jump limm:  LImm(0), F(1), NN(2), Q(3)
    // SO limm:    A(0), LImm(1), F(2), Q(3)
    // Store limm: C(0), LImm(1), F(2), NN(3), Q(4)
    // Load abs:   A(0), LImm(1), [F/NN/Q follow if present]
    // Generic:    A(0), B(1), LImm(2), F(3), NN(4), Q(5)

    unsigned NumOuts = Desc.getNumDefs();
    bool isJump  = (Arc4Op == 0x07);
    bool isStore = (Arc4Op == 0x02) && NumOuts == 0 && !isSR;
    bool isLoad  = (Arc4Op == 0x00 || Arc4Op == 0x01) && NumOuts > 0 && !isLR;

    uint32_t A = 0, B = 0, F = 0, NN = 0, Q = 0;
    unsigned LImmIdx = 2; // default: A(0), B(1), LImm(2)

    if (isJump) {
      // J_l / JL_l: LImm(0), F(1), NN(2), Q(3)
      // B=62 (limm target), C=0
      LImmIdx = 0;
      B = 62;
      Word &= ~((uint32_t)0x3F << 9); // clear C=62 set above
      // C=0 for jump
      F  = (uint32_t)MI.getOperand(1).getImm() & 0x1;
      NN = (uint32_t)MI.getOperand(2).getImm() & 0x3;
      Q  = (uint32_t)MI.getOperand(3).getImm() & 0x1F;
      if (isJL) A = 31; // blink
    } else if (isSO) {
      // SO_l: A(0), LImm(1), F(2), Q(3)
      // Per spec: B=62 (limm source), C=sub-opcode
      A = regEnc(MI.getOperand(0));
      LImmIdx = 1;
      F = (uint32_t)MI.getOperand(2).getImm() & 0x1;
      Q = (uint32_t)MI.getOperand(3).getImm() & 0x1F;
      uint32_t SubOp = (TSF >> 7) & 0x1F;
      B = 62; // limm source
      Word &= ~((uint32_t)0x3F << 9); // clear C=62 set above
      Word |= (SubOp << 9);           // C = sub-opcode
    } else if (isStore || isSR) {
      // ST_abs/SR_abs: C(0), LImm(1)
      // Per spec: B=62 (limm address), C = source register
      // For ST: A = {Di=0, 0, Aw=0, ZZ[1:0], 0}
      // For SR: A = {0, 1, 0, 0, 0, 0} = 16
      uint32_t C = regEnc(MI.getOperand(0));
      LImmIdx = 1;
      Word &= ~((uint32_t)0x3F << 9); // clear C=62 set above
      Word |= (C << 9);               // C = source register
      B = 62;                          // B = limm sentinel for address
      if (isSR) {
        A = 16; // A[4]=1 → SR indicator
      } else {
        uint32_t ZZ = (TSF >> 12) & 0x3;
        A = (ZZ << 1); // A = {0, 0, 0, ZZ[1:0], 0}
      }
      Word |= (A << 21) | (B << 15);
    } else if (isLoad || isLR) {
      // LD_abs/LR_abs: A(0), LImm(1)
      // Per spec: B=62, C=62 (both limm), ZZ/X in lower bits
      A = regEnc(MI.getOperand(0));
      LImmIdx = 1;
      B = 62;
      Word &= ~((uint32_t)0x7FFF << 9); // clear B and C fields
      Word |= (B << 15) | (62u << 9);   // B=62, C=62
    } else {
      bool isLimmInB = (TSF >> 17) & 1;
      bool isDiscardLimm = (Desc.getNumDefs() == 0) && MI.getNumOperands() > 0 &&
                           MI.getOperand(0).isImm() && !isStore && !isSR;

      if (isLimmInB) {
        // Limm-in-B: A(reg0), LImm(imm1), C(reg2), F(3), NN(4), Q(5)
        // Encode: A=dest, B=62(limm), C=register
        A = regEnc(MI.getOperand(0));
        LImmIdx = 1;
        uint32_t C = regEnc(MI.getOperand(2));
        Word &= ~((uint32_t)0x3F << 9); // clear default C=62
        Word |= (C << 9);               // C = actual register
        B = 62;                          // B = limm
        if (MI.getNumOperands() > 3) F  = MI.getOperand(3).getImm() & 0x1;
        if (MI.getNumOperands() > 4) NN = MI.getOperand(4).getImm() & 0x3;
        if (MI.getNumOperands() > 5) Q  = MI.getOperand(5).getImm() & 0x1F;
      } else if (isDiscardLimm) {
        // Discard limm: Z(imm0=0), B(reg1), LImm(2), F(3), NN(4), Q(5)
        A = 63u; // discard
        B = regEnc(MI.getOperand(1));
        LImmIdx = 2;
        if (MI.getNumOperands() > 3) F  = MI.getOperand(3).getImm() & 0x1;
        if (MI.getNumOperands() > 4) NN = MI.getOperand(4).getImm() & 0x3;
        if (MI.getNumOperands() > 5) Q  = MI.getOperand(5).getImm() & 0x1F;
      } else {
        // Generic: A(reg0), B(reg1), LImm(2), F(3), NN(4), Q(5)
        A  = regEnc(MI.getOperand(0));
        B  = regEnc(MI.getOperand(1));
        if (MI.getNumOperands() > 3) F  = MI.getOperand(3).getImm() & 0x1;
        if (MI.getNumOperands() > 4) NN = MI.getOperand(4).getImm() & 0x3;
        if (MI.getNumOperands() > 5) Q  = MI.getOperand(5).getImm() & 0x1F;
      }
    }

    // For loads: ZZ/X go in lower bits (same position as register form)
    if (isLoad) {
      uint32_t ZZ = (TSF >> 12) & 0x3;
      uint32_t X  = (TSF >> 14) & 0x1;
      Word |= (A << 21) | (B << 15) | (ZZ << 1) | X;
    } else {
      Word |= (A << 21) | (B << 15) | (F << 8) | (NN << 5) | Q;
    }

    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    // Emit the limm word (4 bytes after the instruction word)
    emitLimm(MI.getOperand(LImmIdx), /*FixupOff=*/4, Fixups, CB);
    return;
  }

  if (Fmt == 1) {
    // ---- Short-immediate format ----

    bool isNOP = (TSF >> 16) & 1;
    if (isNOP) {
      // NOP = XOR 0x1FF, 0x1FF, 0x1FF → all bits set except bit 31.
      // Per spec: 0x7FFFFFFF
      Word = 0x7FFFFFFFu;
      support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
      return;
    }

    bool isStore = (Arc4Op == 0x02) && (Desc.getNumDefs() == 0);
    bool isLoad  = (Arc4Op == 0x00 || Arc4Op == 0x01) && (Desc.getNumDefs() > 0);

    if (isStore) {
      // ST_rrs: C(0), B(1), D(2)  OR  ST_rb: C(0), B(1) [no offset]
      // Per spec: A field = {Di,0,Aw,ZZ[1:0],0} flags, C = source reg, D = offset
      uint32_t Csrc = regEnc(MI.getOperand(0));
      uint32_t B    = regEnc(MI.getOperand(1));
      uint32_t D    = 0;
      if (MI.getNumOperands() > 2 && MI.getOperand(2).isImm())
        D = (uint32_t)MI.getOperand(2).getImm() & 0x1FF;
      uint32_t ZZ   = (TSF >> 12) & 0x3;
      uint32_t Aenc = (ZZ << 1);
      Word |= (Aenc << 21) | (B << 15) | (Csrc << 9) | (D & 0x1FF);
      support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
      return;
    }

    if (isLoad) {
      // Shimm load (opcode 0x01): A(0), B(1), D(2)
      // Per spec: C field = {Di, 0(LD)/1(LR), Aw, ZZ[1:0], X}
      uint32_t A  = regEnc(MI.getOperand(0));
      uint32_t B  = regEnc(MI.getOperand(1));
      uint32_t D  = (uint32_t)MI.getOperand(2).getImm() & 0x1FF;
      uint32_t ZZ = (TSF >> 12) & 0x3;
      uint32_t X  = (TSF >> 14) & 0x1;
      // C = {Di=0, type=0, Aw=0, ZZ[1], ZZ[0], X}
      uint32_t Cenc = (ZZ << 1) | X;
      Word |= (A << 21) | (B << 15) | (Cenc << 9) | (D & 0x1FF);
      support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
      return;
    }

    // Single-operand shimm (opcode 0x03): A(0), D(1), SetFlags(2), NN(3)
    bool isSO_shimm = (Arc4Op == 0x03) && (Desc.getNumDefs() > 0);
    if (isSO_shimm && MI.getNumOperands() >= 2 && MI.getOperand(1).isImm() &&
        !MI.getOperand(0).isImm()) {
      uint32_t A  = regEnc(MI.getOperand(0));
      uint32_t D  = (uint32_t)MI.getOperand(1).getImm() & 0x1FF;
      uint32_t SetFlags = 0;
      if (MI.getNumOperands() > 2) SetFlags = MI.getOperand(2).getImm() & 0x1;
      uint32_t SubOp = (TSF >> 7) & 0x1F;
      uint32_t Bsent = SetFlags ? 61u : 63u;
      // Per spec: C = sub-opcode always for opcode 0x03, B = shimm sentinel
      Word |= (A << 21) | (Bsent << 15) | (SubOp << 9) | (D & 0x1FF);
      support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
      return;
    }

    // Determine if this is a discard-dest form (first operand is imm 0)
    bool isDiscard = (Desc.getNumDefs() == 0) && MI.getNumOperands() > 0 &&
                     MI.getOperand(0).isImm();
    // Determine if shimm is in operand1 (B position) via TSFlags{17}
    bool isShimmInB = (TSF >> 17) & 1;

    // Generic ALU shimm forms:
    //   Normal:     A(reg0), B(reg1), D(imm2), SetFlags(imm3), NN(imm4)
    //   Discard:    Z(imm0), B(reg1), D(imm2), SetFlags(imm3), NN(imm4)
    //   ShimmInB:   A(reg0), D(imm1), C(reg2), SetFlags(imm3), NN(imm4)
    //   SO discard: Z(imm0), B(reg1), F(imm2), Q(imm3)
    {
      uint32_t A, B, D, SetFlags;
      uint32_t Csent;

      if (isDiscard && Arc4Op == 0x03) {
        // Single-operand discard: Z(imm0), B(reg1), F(imm2), Q(imm3)
        A = 63u; // discard
        B = regEnc(MI.getOperand(1));
        uint32_t SubOp = (TSF >> 7) & 0x1F;
        uint32_t F = 0, Q = 0;
        if (MI.getNumOperands() > 2) F = MI.getOperand(2).getImm() & 0x1;
        if (MI.getNumOperands() > 3) Q = MI.getOperand(3).getImm() & 0x1F;
        // This is actually a register form encoding, not shimm
        Word = (Arc4Op << 27) | (A << 21) | (B << 15) | (SubOp << 9) | (F << 8) | Q;
        support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
        return;
      }

      if (isDiscard) {
        // ALU discard: Z(imm0=0), B(reg1), D(imm2), SetFlags(imm3)
        A = 63u; // discard dest
        B = regEnc(MI.getOperand(1));
        D = (uint32_t)MI.getOperand(2).getImm() & 0x1FF;
        SetFlags = 0;
        if (MI.getNumOperands() > 3) SetFlags = MI.getOperand(3).getImm() & 0x1;
      } else if (isShimmInB) {
        // Shimm-in-B: A(reg0), D(imm1), C(reg2), SetFlags(imm3)
        // Encode: A=reg, B=shimm_sentinel, C=reg, D=shimm_value
        A = regEnc(MI.getOperand(0));
        D = (uint32_t)MI.getOperand(1).getImm() & 0x1FF;
        uint32_t C = regEnc(MI.getOperand(2));
        SetFlags = 0;
        if (MI.getNumOperands() > 3) SetFlags = MI.getOperand(3).getImm() & 0x1;
        Csent = SetFlags ? 61u : 63u;
        // B = shimm sentinel, C = actual register
        Word |= (A << 21) | (Csent << 15) | (C << 9) | (D & 0x1FF);
        support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
        return;
      } else {
        // Normal: A(reg0), B(reg1), D(imm2), SetFlags(imm3)
        A = regEnc(MI.getOperand(0));
        B = regEnc(MI.getOperand(1));
        D = (uint32_t)MI.getOperand(2).getImm() & 0x1FF;
        SetFlags = 0;
        if (MI.getNumOperands() > 3) SetFlags = MI.getOperand(3).getImm() & 0x1;
      }

      Csent = SetFlags ? 61u : 63u;
      Word |= (A << 21) | (B << 15) | (Csent << 9) | (D & 0x1FF);
      support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
      return;
    }
  }

  // ---- Register format (Fmt == 0) ----
  bool isNOP  = (TSF >> 16) & 1;
  bool isJL   = (TSF >> 12) & 1;
  bool isAuxFlag = (TSF >> 15) & 1;
  bool isLR   = isAuxFlag && (Arc4Op == 0x01); // LR uses opcode 0x01
  bool isSR   = isAuxFlag && (Arc4Op == 0x02); // SR uses opcode 0x02
  bool isSO   = (Arc4Op == 0x03);
  bool isJump = (Arc4Op == 0x07);
  bool isLoad = (Arc4Op == 0x00 || Arc4Op == 0x01) && (Desc.getNumDefs() > 0) && !isLR;

  if (isNOP) {
    // NOP = XOR 0x1FF, 0x1FF, 0x1FF → 0x7FFFFFFF per spec
    Word = 0x7FFFFFFFu;
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  // No-operand instructions: BRK / SLEEP / SWI
  // Per spec: A=63, B=63, C=63, D[8:0] = discriminator (0=BRK, 1=SLEEP, 2=SWI)
  if (MI.getNumOperands() == 0) {
    uint32_t Dval = (TSF >> 13) & 0x3; // 0=BRK,1=SLEEP,2=SWI
    Word |= (63u << 21)      // A=63
         |  (63u << 15)      // B=63
         |  (63u << 9)       // C=63
         |  (Dval & 0x1FF);  // D = discriminator
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  // Single-operand instructions (opcode 0x03, has operands)
  if (isSO && !isLR && !isSR) {
    uint32_t SubOp = (TSF >> 7) & 0x1F; // C = sub-opcode

    // Check if this is a discard form: first operand is imm (the '0')
    if (MI.getOperand(0).isImm()) {
      // SO_0r: Z(imm0), B(reg1), F(imm2), Q(imm3) — discard result
      uint32_t A = 63u; // discard
      uint32_t B = regEnc(MI.getOperand(1));
      uint32_t F = 0, Q = 0;
      if (MI.getNumOperands() > 2) F = MI.getOperand(2).getImm() & 0x1;
      if (MI.getNumOperands() > 3) Q = MI.getOperand(3).getImm() & 0x1F;
      Word |= (A << 21) | (B << 15) | (SubOp << 9) | (F << 8) | Q;
      support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
      return;
    }

    // SO_r: A(reg0), B(reg1), F(imm2), Q(imm3)
    uint32_t A  = regEnc(MI.getOperand(0));
    uint32_t B  = regEnc(MI.getOperand(1));
    uint32_t F  = (uint32_t)MI.getOperand(2).getImm() & 0x1;
    uint32_t Q  = (uint32_t)MI.getOperand(3).getImm() & 0x1F;
    Word |= (A << 21) | (B << 15) | (SubOp << 9) | (F << 8) | Q;
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  // Jump register: B(0), F(1), NN(2), Q(3)
  if (isJump) {
    uint32_t A  = isJL ? 31u : 0u;
    uint32_t B  = regEnc(MI.getOperand(0));
    uint32_t F  = (uint32_t)MI.getOperand(1).getImm() & 0x1;
    uint32_t NN = (uint32_t)MI.getOperand(2).getImm() & 0x3;
    uint32_t Q  = (uint32_t)MI.getOperand(3).getImm() & 0x1F;
    Word |= (A << 21) | (B << 15) | (0u << 9) | (F << 8) | (NN << 5) | Q;
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  // LR register form: A(0), B(1)
  // Per spec (page 102): opcode 0x01, C[4]=1 (bit 13), rest of C/D = reserved
  if (isLR) {
    uint32_t A = regEnc(MI.getOperand(0));
    uint32_t B = regEnc(MI.getOperand(1));
    // C field: bit 13 = 1 (LR indicator), rest = 0 → C = 0b010000 = 16
    Word |= (A << 21) | (B << 15) | (16u << 9);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }
  // SR register form: C(0), B(1)
  // Per spec: opcode 0x02, A[4]=1 (bit 25 = SR indicator)
  if (isSR) {
    uint32_t C = regEnc(MI.getOperand(0));
    uint32_t B = regEnc(MI.getOperand(1));
    // A field: bit 25 = 1 (SR indicator) → A = 0b010000 = 16
    Word |= (16u << 21) | (B << 15) | (C << 9);
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  // Load register form (opcode 0x00): A(0), B(1), C(2)
  // Per spec: lower bits encode {Di(5), ?(4), Aw(3), ZZ[1:0](2:1), X(0)}
  if (isLoad) {
    uint32_t A  = regEnc(MI.getOperand(0));
    uint32_t B  = regEnc(MI.getOperand(1));
    uint32_t C  = regEnc(MI.getOperand(2));
    uint32_t ZZ = (TSF >> 12) & 0x3;
    uint32_t X  = (TSF >> 14) & 0x1;
    // Encode Z in bits 2:1, X in bit 0 (Di=0, Aw=0 for now)
    Word |= (A << 21) | (B << 15) | (C << 9) | (ZZ << 1) | X;
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
    return;
  }

  // Generic ALU register form.
  // Normal:  A(reg0), B(reg1), C(reg2), F(imm3), NN(imm4), Q(imm5)
  // Discard: Z(imm0=0), B(reg1), C(reg2), F(imm3), NN(imm4), Q(imm5)
  {
    uint32_t A, B, C;
    unsigned Idx = 0;
    if (MI.getOperand(0).isImm()) {
      // Discard form: first operand is literal 0, encode A=63
      A = 63u;
      Idx = 1;
    } else {
      A = regEnc(MI.getOperand(0));
      Idx = 1;
    }
    B = regEnc(MI.getOperand(Idx++));
    C = regEnc(MI.getOperand(Idx++));
    uint32_t F  = (Idx < MI.getNumOperands()) ? MI.getOperand(Idx++).getImm() & 0x1 : 0;
    uint32_t NN = (Idx < MI.getNumOperands()) ? MI.getOperand(Idx++).getImm() & 0x3 : 0;
    uint32_t Q  = (Idx < MI.getNumOperands()) ? MI.getOperand(Idx++).getImm() & 0x1F : 0;
    Word |= (A << 21) | (B << 15) | (C << 9) | (F << 8) | (NN << 5) | Q;
    support::endian::write<uint32_t>(CB, Word, llvm::endianness::little);
  }
}

MCCodeEmitter *llvm::createARC4MCCodeEmitter(const MCInstrInfo &MII,
                                              MCContext &Ctx) {
  return new ARC4MCCodeEmitter(MII, Ctx);
}
