//===-- ARC4AsmBackend.cpp - ARC4 Asm Backend -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FixupKinds.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/EndianStream.h"

using namespace llvm;

namespace {

class ARC4AsmBackend : public MCAsmBackend {
  uint8_t OSABI;

public:
  ARC4AsmBackend(uint8_t OSABI)
      : MCAsmBackend(llvm::endianness::little), OSABI(OSABI) {}

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    const static MCFixupKindInfo Infos[ARC4::NumTargetFixupKinds] = {
        {"fixup_arc4_b26", 0, 24, 0},
        // B22_PCREL: 20-bit offset at bit position 7 (bits [26:7]).
        // "22-bit" refers to address range (20 encoding bits × 4 = 22-bit range).
        {"fixup_arc4_b22_pcrel", 7, 20, 0},
    };
    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);
    assert(unsigned(Kind - FirstTargetFixupKind) < ARC4::NumTargetFixupKinds &&
           "Invalid kind!");
    return Infos[Kind - FirstTargetFixupKind];
  }

  void applyFixup(const MCFragment &F, const MCFixup &Fixup,
                  const MCValue &Target, uint8_t *Data,
                  uint64_t Value, bool IsResolved) override {
    if (!IsResolved)
      Asm->getWriter().recordRelocation(F, Fixup, Target, Value);

    // Note: Data already points to the fixup location within the fragment
    // (MCAssembler sets Data = Contents.data() + Fixup.getOffset()), so we
    // write directly at Data without adding any offset.

    switch (Fixup.getKind()) {
    default:
      break;
    case FK_Data_1:
      *Data = Value;
      return;
    case FK_Data_2:
      support::endian::write<uint16_t>(Data, Value,
                                        llvm::endianness::little);
      return;
    case FK_Data_4:
      support::endian::write<uint32_t>(Data, Value,
                                        llvm::endianness::little);
      return;
    case ARC4::fixup_arc4_b26: {
      uint32_t CurVal = support::endian::read32le(Data);
      uint32_t Encoded = (Value >> 2) & 0x00ffffff;
      support::endian::write<uint32_t>(Data,
                                        (CurVal & 0xff000000) | Encoded,
                                        llvm::endianness::little);
      return;
    }
    case ARC4::fixup_arc4_b22_pcrel: {
      // 20-bit offset in bits [26:7], word-aligned.
      // ARC4 branch offset is relative to delay slot (PC+4), not the branch
      // instruction itself. Subtract 4 to adjust.
      uint32_t CurVal = support::endian::read32le(Data);
      int64_t Adjusted = static_cast<int64_t>(Value) - 4;
      uint32_t Encoded = ((static_cast<uint32_t>(Adjusted) >> 2) << 7) & 0x07ffff80;
      support::endian::write<uint32_t>(Data,
                                        (CurVal & ~0x07ffff80u) | Encoded,
                                        llvm::endianness::little);
      return;
    }
    }
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    uint64_t NumNops = Count / 4;
    for (uint64_t I = 0; I < NumNops; ++I)
      support::endian::write<uint32_t>(OS, 0x7fffffff, llvm::endianness::little);
    for (uint64_t I = 0; I < Count % 4; ++I)
      OS.write('\0');
    return true;
  }

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createARC4ELFObjectWriter(OSABI);
  }
};

} // end anonymous namespace

MCAsmBackend *llvm::createARC4AsmBackend(const Target &T,
                                         const MCSubtargetInfo &STI,
                                         const MCRegisterInfo &MRI,
                                         const MCTargetOptions &Options) {
  uint8_t OSABI =
      MCELFObjectTargetWriter::getOSABI(STI.getTargetTriple().getOS());
  return new ARC4AsmBackend(OSABI);
}
