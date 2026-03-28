//===-- ARC4MCCodeEmitter.cpp - Convert ARC4 code to machine code ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FixupKinds.h"
#include "ARC4MCTargetDesc.h"
#include "ARC4TSFlags.h"
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

  // For expressions (symbol references), return 0 without creating a fixup.
  // Branch targets are handled by getBranchTargetOpValue with proper fixup kind.
  // Limm expressions will have their fixup created in encodeInstruction when
  // the limm word is emitted at offset 4.
  assert(MO.isExpr());
  return 0;
}

unsigned ARC4MCCodeEmitter::getBranchTargetOpValue(
    const MCInst &Inst, unsigned OpIdx, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = Inst.getOperand(OpIdx);
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());

  // For symbolic targets, emit a PC-relative fixup.
  // Branch offsets are word-aligned (lower 2 bits not stored).
  assert(MO.isExpr());
  Fixups.push_back(
      MCFixup::create(0, MO.getExpr(),
                      MCFixupKind(ARC4::fixup_arc4_b22_pcrel)));
  return 0;
}

/// Check whether the instruction has shimm sentinels in register fields.
/// Checks A (discard), B, and C fields for sentinel 63 (SHIMM) or 61
/// (SHIMM_UPDATE).
static void detectShimmFields(uint32_t Value, bool &AFieldIsShimm,
                              bool &BFieldIsShimm, bool &CFieldIsShimm) {
  unsigned A = (Value >> 21) & 0x3F;
  unsigned B = (Value >> 15) & 0x3F;
  unsigned C = (Value >> 9) & 0x3F;
  AFieldIsShimm = (A == 63 || A == 61);
  BFieldIsShimm = (B == 63 || B == 61);
  CFieldIsShimm = (C == 63 || C == 61);
}

/// Find the flag (f) operand value for sentinel flipping.
/// Returns the f operand value, or 0 if not applicable.
///
/// Sentinel flipping changes sentinel 63 (SHIMM, no flag update) to 61
/// (SHIMM_UPDATE) in register fields when the .f suffix is present.  This
/// covers shimm forms (B/C field) and discard forms (A field = 63).
///
/// TSFlags encode the suffix layout so we no longer need per-opcode cases:
///   HasFOnly  — shimm forms:          f is the last operand
///   HasFQ     — non-shimm ALU/SOP:    f is at ExpOps-2 (before q)
///   HasFQN    — jump forms:           f is at ExpOps-3 (before q, n)
static int getSentinelFlagValue(const MCInst &Inst, const MCInstrDesc &Desc) {
  unsigned ExpOps = Desc.getNumOperands();
  if (Inst.getNumOperands() < ExpOps || ExpOps == 0)
    return 0;

  uint64_t TSF = Desc.TSFlags;
  if (TSF & ARC4TSF::HasFOnly)
    return Inst.getOperand(ExpOps - 1).getImm();
  if (TSF & ARC4TSF::HasFQ)
    return Inst.getOperand(ExpOps - 2).getImm();
  if (TSF & ARC4TSF::HasFQN)
    return Inst.getOperand(ExpOps - 3).getImm();
  return 0;
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
    int FlagVal = getSentinelFlagValue(Inst, Desc);
    if (FlagVal != 0) {
      uint32_t V = static_cast<uint32_t>(Value);
      bool AShimm, BShimm, CShimm;
      detectShimmFields(V, AShimm, BShimm, CShimm);
      // Flip sentinel 63 -> 61 in any field that has shimm
      if (AShimm && ((V >> 21) & 0x3F) == 63) {
        V &= ~(0x3FU << 21);
        V |= (61U << 21);
      }
      if (BShimm && ((V >> 15) & 0x3F) == 63) {
        V &= ~(0x3FU << 15);
        V |= (61U << 15);
      }
      if (CShimm && ((V >> 9) & 0x3F) == 63) {
        V &= ~(0x3FU << 9);
        V |= (61U << 9);
      }
      Value = V;
    }
  }

  // Emit the 32-bit instruction word (little-endian).
  support::endian::write<uint32_t>(CB, static_cast<uint32_t>(Value),
                                   llvm::endianness::little);

  // For instructions with LIMM (HasLimm TSFlag), emit the extra 32-bit word.
  // The limm operand is always the first non-register operand in the MCInst
  // (suffix operands f/q/n come after the limm in operand order).
  if (Desc.TSFlags & 0x1) {
    bool Emitted = false;
    for (unsigned I = 0, E = Inst.getNumOperands(); I < E; ++I) {
      const MCOperand &MO = Inst.getOperand(I);
      if (MO.isReg())
        continue;
      if (MO.isExpr()) {
        // Symbol reference — emit fixup at offset 4 (limm word position).
        // For jump targets (j/jl limm), the limm stores address >> 2
        // (word-aligned, status register format). Use fixup_arc4_b26.
        // For data limm (ALU/load/store), full 32-bit value. Use FK_Data_4.
        unsigned Opc5 = (static_cast<uint32_t>(Value) >> 27) & 0x1F;
        MCFixupKind Kind = (Opc5 == 7) // opcode 7 = jump
            ? MCFixupKind(ARC4::fixup_arc4_b26)
            : MCFixupKind(FK_Data_4);
        Fixups.push_back(MCFixup::create(4, MO.getExpr(), Kind));
        support::endian::write<uint32_t>(CB, 0, llvm::endianness::little);
        Emitted = true;
        break;
      }
      if (MO.isImm()) {
        uint32_t LimmVal = static_cast<uint32_t>(MO.getImm());
        // For jump instructions (opcode 7), the limm stores address >> 2
        // (word-aligned, same format as status register — lower 2 bits
        // not included).
        unsigned Opc5 = (static_cast<uint32_t>(Value) >> 27) & 0x1F;
        if (Opc5 == 7)
          LimmVal >>= 2;
        support::endian::write<uint32_t>(CB, LimmVal,
                                          llvm::endianness::little);
        Emitted = true;
        break;
      }
    }
    if (!Emitted)
      support::endian::write<uint32_t>(CB, 0, llvm::endianness::little);
  }
}

MCCodeEmitter *createARC4MCCodeEmitter(const MCInstrInfo &MCII,
                                       MCContext &Ctx) {
  return new ARC4MCCodeEmitter(MCII, Ctx);
}

#include "ARC4GenMCCodeEmitter.inc"

} // namespace llvm
