//===-- ARC4MCCodeEmitter.cpp - Convert ARC4 code to machine code ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FixupKinds.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/EndianStream.h"

#define DEBUG_TYPE "mccodeemitter"

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");

namespace llvm {

class ARC4MCCodeEmitter : public MCCodeEmitter {
  MCContext &Ctx;
  const MCInstrInfo &MCII;

public:
  ARC4MCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : Ctx(Ctx), MCII(MCII) {}

  uint64_t getBinaryCodeForInstr(const MCInst &Inst,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &Inst, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned getBranchTargetOpValue(const MCInst &Inst, unsigned OpIdx,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const;

  void encodeInstruction(const MCInst &Inst, SmallVectorImpl<char> &CB,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;
};

unsigned ARC4MCCodeEmitter::getMachineOpValue(const MCInst &Inst,
                                              const MCOperand &MO,
                                              SmallVectorImpl<MCFixup> &Fixups,
                                              const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  assert(MO.isExpr());
  Fixups.push_back(
      MCFixup::create(0, MO.getExpr(), MCFixupKind(FK_Data_4)));
  return 0;
}

unsigned ARC4MCCodeEmitter::getBranchTargetOpValue(
    const MCInst &Inst, unsigned OpIdx, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = Inst.getOperand(OpIdx);
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  // For symbolic targets, emit a fixup.
  assert(MO.isExpr());
  Fixups.push_back(
      MCFixup::create(0, MO.getExpr(), MCFixupKind(FK_Data_4)));
  return 0;
}

/// Check whether the instruction has a shimm operand by looking for sentinel
/// values 63 (SHIMM) or 61 (SHIMM_UPDATE) in the B or C register fields.
static void detectShimmFields(uint32_t Value, bool &BFieldIsShimm,
                              bool &CFieldIsShimm) {
  unsigned B = (Value >> 15) & 0x3F;
  unsigned C = (Value >> 9) & 0x3F;
  BFieldIsShimm = (B == 63 || B == 61);
  CFieldIsShimm = (C == 63 || C == 61);
}

/// Find the flag (f) operand value for shimm sentinel flipping.
/// Returns the f operand value, or 0 if no f operand exists.
/// The f operand is the last operand for shimm forms (rrs/rsr/rss, SOP rs),
/// since shimm forms have: visible operands..., f.
static int getShimmFlagValue(const MCInst &Inst, const MCInstrDesc &Desc) {
  unsigned Opc = Inst.getOpcode();
  unsigned NumOps = Inst.getNumOperands();
  unsigned ExpOps = Desc.getNumOperands();
  // Only check if all expected operands are present.
  if (NumOps < ExpOps || ExpOps == 0)
    return 0;

  // For ALU shimm forms (rrs/rsr/rss) and SOP rs, the f operand is the
  // last operand in the ins list. It's not wired to encoding bits, but
  // we read it here to decide whether to flip sentinel 63->61.
  switch (Opc) {
  // ALU3 shimm variants: last operand is f
  case ARC4::ADD_rrs: case ARC4::ADD_rsr: case ARC4::ADD_rss:
  case ARC4::ADD_0rs: case ARC4::ADD_0sr: case ARC4::ADD_0ss:
  case ARC4::ADC_rrs: case ARC4::ADC_rsr: case ARC4::ADC_rss:
  case ARC4::ADC_0rs: case ARC4::ADC_0sr: case ARC4::ADC_0ss:
  case ARC4::SUB_rrs: case ARC4::SUB_rsr: case ARC4::SUB_rss:
  case ARC4::SUB_0rs: case ARC4::SUB_0sr: case ARC4::SUB_0ss:
  case ARC4::SBC_rrs: case ARC4::SBC_rsr: case ARC4::SBC_rss:
  case ARC4::SBC_0rs: case ARC4::SBC_0sr: case ARC4::SBC_0ss:
  case ARC4::AND_rrs: case ARC4::AND_rsr: case ARC4::AND_rss:
  case ARC4::AND_0rs: case ARC4::AND_0sr: case ARC4::AND_0ss:
  case ARC4::OR_rrs:  case ARC4::OR_rsr:  case ARC4::OR_rss:
  case ARC4::OR_0rs:  case ARC4::OR_0sr:  case ARC4::OR_0ss:
  case ARC4::BIC_rrs: case ARC4::BIC_rsr: case ARC4::BIC_rss:
  case ARC4::BIC_0rs: case ARC4::BIC_0sr: case ARC4::BIC_0ss:
  case ARC4::XOR_rrs: case ARC4::XOR_rsr: case ARC4::XOR_rss:
  case ARC4::XOR_0rs: case ARC4::XOR_0sr: case ARC4::XOR_0ss:
  // SOP shimm variants
  case ARC4::ASL_rs:  case ARC4::ASL_0s:
  case ARC4::ASR_rs:  case ARC4::ASR_0s:
  case ARC4::LSR_rs:  case ARC4::LSR_0s:
  case ARC4::ROR_rs:  case ARC4::ROR_0s:
  case ARC4::RRC_rs:  case ARC4::RRC_0s:
  case ARC4::SEXB_rs: case ARC4::SEXB_0s:
  case ARC4::SEXW_rs: case ARC4::SEXW_0s:
  case ARC4::EXTB_rs: case ARC4::EXTB_0s:
  case ARC4::EXTW_rs: case ARC4::EXTW_0s:
    // f is the last operand.
    return Inst.getOperand(ExpOps - 1).getImm();
  default:
    return 0;
  }
}

void ARC4MCCodeEmitter::encodeInstruction(const MCInst &Inst,
                                           SmallVectorImpl<char> &CB,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  uint64_t Value = getBinaryCodeForInstr(Inst, Fixups, STI);
  ++MCNumEmitted;

  const MCInstrDesc &Desc = MCII.get(Inst.getOpcode());

  // Shimm sentinel flip: for shimm instructions where the f operand = 1,
  // change sentinel 63 -> 61 in the register field(s). This is the only
  // remaining post-hoc fixup because TableGen can't conditionally set bits.
  {
    int FlagVal = getShimmFlagValue(Inst, Desc);
    if (FlagVal != 0) {
      uint32_t V = static_cast<uint32_t>(Value);
      bool BShimm, CShimm;
      detectShimmFields(V, BShimm, CShimm);
      if (CShimm) {
        unsigned CField = (V >> 9) & 0x3F;
        if (CField == 63) {
          V &= ~(0x3FU << 9);
          V |= (61U << 9);
        }
      }
      if (BShimm) {
        unsigned BField = (V >> 15) & 0x3F;
        if (BField == 63) {
          V &= ~(0x3FU << 15);
          V |= (61U << 15);
        }
      }
      Value = V;
    }
  }

  // Emit the 32-bit instruction word (little-endian).
  support::endian::write<uint32_t>(CB, static_cast<uint32_t>(Value),
                                   llvm::endianness::little);

  // For 8-byte instructions (limm), emit the extra 32-bit limm word.
  if (Desc.getSize() == 8) {
    // The limm value is in an immediate operand. Find it among the operands.
    // Skip suffix operands (f, q, n) which are also immediates — the limm
    // is always among the first few operands (visible in AsmString).
    uint32_t LimmVal = 0;
    unsigned NumOps = Inst.getNumOperands();
    for (unsigned I = 0; I < NumOps; ++I) {
      const MCOperand &MO = Inst.getOperand(I);
      if (MO.isImm()) {
        LimmVal = static_cast<uint32_t>(MO.getImm());
        break;
      }
    }
    support::endian::write<uint32_t>(CB, LimmVal, llvm::endianness::little);
  }
}

MCCodeEmitter *createARC4MCCodeEmitter(const MCInstrInfo &MCII,
                                       MCContext &Ctx) {
  return new ARC4MCCodeEmitter(MCII, Ctx);
}

#include "ARC4GenMCCodeEmitter.inc"

} // namespace llvm
