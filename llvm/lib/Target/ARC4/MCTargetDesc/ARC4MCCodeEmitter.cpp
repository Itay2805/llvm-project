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
/// Sentinel flipping is needed for:
/// - Shimm forms (rrs/rsr/rss, SOP rs): flip B/C sentinel 63->61
/// - Discard forms (0rr/0rs/0sr/0ss/0rl/0lr, SOP 0r/0s/0l): flip A sentinel 63->61
///
/// For non-shimm non-discard forms, f is wired to bit[8] by TableGen
/// and no post-hoc flip is needed.
static int getSentinelFlagValue(const MCInst &Inst, const MCInstrDesc &Desc) {
  unsigned NumOps = Inst.getNumOperands();
  unsigned ExpOps = Desc.getNumOperands();
  if (NumOps < ExpOps || ExpOps == 0)
    return 0;

  // Check if any register field has sentinel 63 (SHIMM) that might need
  // flipping to 61 (SHIMM_UPDATE). This covers both shimm operands in
  // B/C fields AND discard sentinels in the A field.
  //
  // The f operand position varies by form but is always present as an
  // i32imm. For shimm-only forms it's the last operand; for non-shimm
  // discard forms it's typically at a known position. We use a simple
  // heuristic: check the encoded instruction for sentinel 63 in any
  // field. If found, read the f value from the instruction.
  //
  // Rather than enumerate all opcodes, we check the encoded word later.
  // Here we just need to return the f operand value. The f operand is
  // always the first i32imm after the visible operands for forms that
  // need sentinel flipping. Since the caller only uses this when a
  // sentinel 63 is detected, it's safe to always return the f value.

  // For all instruction forms with suffix operands, the f operand
  // position depends on the form. Find it by checking which operand
  // looks like f (value 0 or 1, appears after visible operands).
  // In our convention: for shimm forms, f is the last operand.
  // For non-shimm forms, f comes after visible ops but before q.
  //
  // Simplest correct approach: the f operand is always an immediate
  // with value 0 or 1. The last few operands are suffix operands
  // (f, q, n in some order). Find the f by checking operand values.
  // Actually, f is always at a specific position per instruction form.
  // But since we can't easily determine the form here, we check
  // whether the instruction HAS a sentinel 63 in A/B/C — if so,
  // the last operand (for shimm) or the f-position operand (for
  // discard non-shimm) tells us the flag value.

  // The f operand is the last operand for shimm forms (only suffix is f).
  // For non-shimm forms with f+q, f is at ExpOps-2 position.
  // For non-shimm forms with f+q+n, f is at ExpOps-3 position.
  // To simplify: just return the last operand if it's 0 or 1 (shimm forms),
  // or check the second-to-last / third-to-last for other forms.
  // Actually, the simplest: return 1 if ANY suffix operand is 1 and
  // represents f. But that requires knowing which one is f.

  // Pragmatic: enumerate the opcodes that need sentinel flipping.
  // This includes ALL forms where A, B, or C could be sentinel 63.
  unsigned Opc = Inst.getOpcode();
  switch (Opc) {
  // --- Shimm forms: f is the LAST operand ---
  // ALU3 shimm variants
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
  case ARC4::ASR_rs:  case ARC4::ASR_0s:
  case ARC4::LSR_rs:  case ARC4::LSR_0s:
  case ARC4::ROR_rs:  case ARC4::ROR_0s:
  case ARC4::RRC_rs:  case ARC4::RRC_0s:
  case ARC4::SEXB_rs: case ARC4::SEXB_0s:
  case ARC4::SEXW_rs: case ARC4::SEXW_0s:
  case ARC4::EXTB_rs: case ARC4::EXTB_0s:
  case ARC4::EXTW_rs: case ARC4::EXTW_0s:
    return Inst.getOperand(ExpOps - 1).getImm();

  // --- Non-shimm discard forms: A field has sentinel 63 ---
  // For these, f is wired to bit[8] by TableGen, BUT we also need
  // to flip A from 63->61. The f operand is at position ExpOps-2
  // (before q) for rrr/rrl/rlr forms.
  case ARC4::ADD_0rr: case ARC4::ADD_0rl: case ARC4::ADD_0lr:
  case ARC4::ADC_0rr: case ARC4::ADC_0rl: case ARC4::ADC_0lr:
  case ARC4::SUB_0rr: case ARC4::SUB_0rl: case ARC4::SUB_0lr:
  case ARC4::SBC_0rr: case ARC4::SBC_0rl: case ARC4::SBC_0lr:
  case ARC4::AND_0rr: case ARC4::AND_0rl: case ARC4::AND_0lr:
  case ARC4::OR_0rr:  case ARC4::OR_0rl:  case ARC4::OR_0lr:
  case ARC4::BIC_0rr: case ARC4::BIC_0rl: case ARC4::BIC_0lr:
  case ARC4::XOR_0rr: case ARC4::XOR_0rl: case ARC4::XOR_0lr:
  // SOP non-shimm discard: f is at ExpOps-2 (before q)
  case ARC4::ASR_0r:  case ARC4::ASR_0l:
  case ARC4::LSR_0r:  case ARC4::LSR_0l:
  case ARC4::ROR_0r:  case ARC4::ROR_0l:
  case ARC4::RRC_0r:  case ARC4::RRC_0l:
  case ARC4::SEXB_0r: case ARC4::SEXB_0l:
  case ARC4::SEXW_0r: case ARC4::SEXW_0l:
  case ARC4::EXTB_0r: case ARC4::EXTB_0l:
  case ARC4::EXTW_0r: case ARC4::EXTW_0l:
    // f is at ExpOps-2 (before q). ins order: ..., f, q
    return Inst.getOperand(ExpOps - 2).getImm();

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

  // For 8-byte instructions (limm), emit the extra 32-bit limm word.
  // The limm operand is always the first non-register operand in the MCInst
  // (suffix operands f/q/n come after the limm in operand order).
  if (Desc.getSize() == 8) {
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
