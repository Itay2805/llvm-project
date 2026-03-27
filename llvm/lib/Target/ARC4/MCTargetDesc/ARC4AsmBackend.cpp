//===-- ARC4AsmBackend.cpp - ARC4 Assembler Backend -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4MCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include <cstring>
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

// ---------------------------------------------------------------------------
// ELF object writer
// ---------------------------------------------------------------------------
namespace {

class ARC4ELFObjectWriter : public MCELFObjectTargetWriter {
public:
  explicit ARC4ELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_ARC,
                                /*HasRelocationAddend=*/true) {}
  ~ARC4ELFObjectWriter() override = default;

protected:
  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    // We only define one target fixup: the 20-bit branch offset.
    // All other fixups use generic FK_Data_N types.
    unsigned Kind = Fixup.getKind();
    if (IsPCRel)
      return ELF::R_ARC_32_PCREL; // closest ARC reloc for branch-range
    switch (Kind) {
    case FK_Data_4:
      return ELF::R_ARC_32;
    default:
      llvm_unreachable("Unhandled fixup kind");
    }
  }
};

} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createARC4ELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<ARC4ELFObjectWriter>(OSABI);
}

// ---------------------------------------------------------------------------
// Asm backend
// ---------------------------------------------------------------------------
namespace {

class ARC4AsmBackend : public MCAsmBackend {
  Triple::OSType OSType;

public:
  ARC4AsmBackend(Triple::OSType OST)
      : MCAsmBackend(llvm::endianness::little), OSType(OST) {}

  void applyFixup(const MCFragment &, const MCFixup &, const MCValue &,
                  uint8_t *Data, uint64_t Value, bool IsResolved) override;

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createARC4ELFObjectWriter(MCELFObjectTargetWriter::getOSABI(OSType));
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override;

  bool writeNopData(raw_ostream &OS, uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    // ARC4 NOP = XOR 0x1FF, 0x1FF, 0x1FF = 0x7FFFFFFF per spec (page 113)
    // In LE bytes: FF FF FF 7F
    if (Count % 4 != 0)
      return false;
    for (uint64_t I = 0; I < Count; I += 4)
      OS.write("\xff\xff\xff\x7f", 4);
    return true;
  }
};

} // namespace

MCFixupKindInfo ARC4AsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  // One target-specific fixup: 20-bit branch offset in bits[26:7]
  static const MCFixupKindInfo Infos[] = {
      // name               offset bits flags
      {"FK_ARC4_Branch20",  7,     20,  0},
  };
  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);
  assert(unsigned(Kind - FirstTargetFixupKind) <
             sizeof(Infos) / sizeof(Infos[0]) && "Invalid fixup kind");
  return Infos[Kind - FirstTargetFixupKind];
}

void ARC4AsmBackend::applyFixup(const MCFragment &F, const MCFixup &Fixup,
                                 const MCValue &Target, uint8_t *Data,
                                 uint64_t Value, bool IsResolved) {
  // Record a relocation for unresolved fixups (global/external symbols).
  // This must be called before any early returns so that the ELF writer
  // can emit the relocation entry.
  maybeAddReloc(F, Fixup, Target, Value, IsResolved);

  if (!IsResolved)
    return;

  MCFixupKind Kind = Fixup.getKind();
  if (Kind == FK_Data_4) {
    support::endian::write<uint32_t>(Data, (uint32_t)Value,
                                     llvm::endianness::little);
    return;
  }

  // FK_ARC4_Branch20: 20-bit PC-relative offset in bits[26:7]
  // The fixup expression already includes the -4 PC+4 adjustment
  // (baked into the MCExpr by the code emitter), so Value is the
  // final byte offset from the delay slot. Encode as word offset.
  if ((unsigned)Kind == FirstTargetFixupKind) {
    uint32_t L = (uint32_t)(((int64_t)Value >> 2) & 0xFFFFF);
    uint32_t Word;
    memcpy(&Word, Data, 4);
    Word = (Word & ~(0xFFFFFu << 7)) | (L << 7);
    support::endian::write<uint32_t>(Data, Word, llvm::endianness::little);
  }
}

MCAsmBackend *llvm::createARC4AsmBackend(const Target &T,
                                          const MCSubtargetInfo &STI,
                                          const MCRegisterInfo & /*MRI*/,
                                          const MCTargetOptions & /*Opts*/) {
  if (!STI.getTargetTriple().isOSBinFormatELF())
    llvm_unreachable("ARC4 only supports ELF");
  return new ARC4AsmBackend(STI.getTargetTriple().getOS());
}
