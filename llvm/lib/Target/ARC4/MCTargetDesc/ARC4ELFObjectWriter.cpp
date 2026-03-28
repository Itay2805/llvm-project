//===-- ARC4ELFObjectWriter.cpp - ARC4 ELF Writer -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FixupKinds.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class ARC4ELFObjectWriter : public MCELFObjectTargetWriter {
public:
  explicit ARC4ELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_ARC,
                                /*HasRelocationAddend=*/true) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    switch (Fixup.getKind()) {
    default:
      llvm_unreachable("Invalid fixup kind!");
    case FK_Data_4:
      return 4; // R_ARC_32
    case ARC4::fixup_arc4_b26:
      return 5; // R_ARC_B26
    case ARC4::fixup_arc4_b22_pcrel:
      return 6; // R_ARC_B22_PCREL
    }
  }
};

} // end anonymous namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createARC4ELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<ARC4ELFObjectWriter>(OSABI);
}
