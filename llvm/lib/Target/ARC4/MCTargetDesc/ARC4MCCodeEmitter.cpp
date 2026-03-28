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
/// Returns true if shimm is present, and sets BFieldIsShimm / CFieldIsShimm.
static void detectShimmFields(uint32_t Value, bool &BFieldIsShimm,
                              bool &CFieldIsShimm) {
  unsigned B = (Value >> 15) & 0x3F;
  unsigned C = (Value >> 9) & 0x3F;
  BFieldIsShimm = (B == 63 || B == 61);
  CFieldIsShimm = (C == 63 || C == 61);
}

void ARC4MCCodeEmitter::encodeInstruction(const MCInst &Inst,
                                           SmallVectorImpl<char> &CB,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo &STI) const {
  uint64_t Value = getBinaryCodeForInstr(Inst, Fixups, STI);
  ++MCNumEmitted;

  const MCInstrDesc &Desc = MCII.get(Inst.getOpcode());
  unsigned NumDefOps = Desc.getNumOperands();
  unsigned NumInstOps = Inst.getNumOperands();

  // Check for trailing annotation operands (condition code, flag, delay slot).
  // These are appended by the AsmParser beyond the expected operand count.
  // Convention: 3 trailing imm operands = [cc, flag, delay].
  if (NumInstOps > NumDefOps && (NumInstOps - NumDefOps) == 3) {
    int CC = Inst.getOperand(NumDefOps).getImm();
    int Flag = Inst.getOperand(NumDefOps + 1).getImm();
    int Delay = Inst.getOperand(NumDefOps + 2).getImm();

    uint32_t V = static_cast<uint32_t>(Value);
    unsigned Opcode5 = (V >> 27) & 0x1F;

    // Apply condition code: bits [4:0].
    // Only for non-shimm instructions (shimm uses bits [8:0] for the value).
    if (CC != 0) {
      bool BShimm, CShimm;
      detectShimmFields(V, BShimm, CShimm);
      if (!BShimm && !CShimm) {
        // Safe to set condition code in bits [4:0].
        V = (V & ~0x1FU) | (CC & 0x1F);
      }
    }

    // Apply flag bit.
    if (Flag != 0) {
      bool BShimm, CShimm;
      detectShimmFields(V, BShimm, CShimm);
      if (BShimm || CShimm) {
        // Shimm form: change sentinel 63->61 in the shimm field(s).
        // Sentinel 63 = 0b111111, sentinel 61 = 0b111101.
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
      } else {
        // Non-shimm form: set bit 8.
        V |= (1U << 8);
      }
    }

    // Apply delay slot: bits [6:5]. Only for branch/jump opcodes (4-7).
    if (Delay != 0 && Opcode5 >= 4 && Opcode5 <= 7) {
      V = (V & ~(0x3U << 5)) | ((Delay & 0x3) << 5);
    }

    Value = V;
  }

  // Default delay slot for branch-and-link (BL) and jump-and-link with limm
  // (JL_l): when no explicit delay suffix was specified, use .jd (bits[6:5]=10)
  // instead of .nd (00). The architecture requires .jd for correct operation
  // of the delay slot on link instructions — the delay slot instruction must
  // execute only when the jump is taken.
  //
  // This applies when there are no trailing annotations (user wrote bare
  // "bl target" or "jl target" without any suffix). When annotations are
  // present, the user explicitly chose a delay mode and we respect it.
  {
    unsigned Opc = Inst.getOpcode();
    if (NumInstOps == NumDefOps) {
      // No trailing annotations — apply default .jd for link instructions.
      if (Opc == ARC4::BL || Opc == ARC4::BLcc ||
          Opc == ARC4::JL_l || Opc == ARC4::JL_r) {
        uint32_t V = static_cast<uint32_t>(Value);
        V = (V & ~(0x3U << 5)) | (0x2U << 5);  // .jd = 2
        Value = V;
      }
    }
  }

  // Emit the 32-bit instruction word (little-endian).
  support::endian::write<uint32_t>(CB, static_cast<uint32_t>(Value),
                                   llvm::endianness::little);

  // For 8-byte instructions (limm), emit the extra 32-bit limm word.
  if (Desc.getSize() == 8) {
    // The limm value is in an immediate operand. Find it (among the real
    // operands, not the trailing annotations).
    uint32_t LimmVal = 0;
    unsigned SearchEnd = (NumInstOps > NumDefOps) ? NumDefOps : NumInstOps;
    for (unsigned I = 0; I < SearchEnd; ++I) {
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
