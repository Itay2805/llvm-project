//===- ARC4.cpp -----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 (ARCtangent-A4) is a 32-bit RISC processor. This provides minimal
// linker support for static linking of ARC4 ELF objects.
//
// Supported relocations:
//   R_ARC_32       - absolute 32-bit
//   R_ARC_32_PCREL - PC-relative 32-bit
//
//===----------------------------------------------------------------------===//

#include "Symbols.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
class ARC4 final : public TargetInfo {
public:
  ARC4(Ctx &);
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

ARC4::ARC4(Ctx &ctx) : TargetInfo(ctx) {
  // BRK instruction as trap: 0x1FFFFE00 in LE = 00 FE FF 1F
  trapInstr = {0x00, 0xfe, 0xff, 0x1f};
  defaultMaxPageSize = 4096;
  defaultCommonPageSize = 4096;
}

RelExpr ARC4::getRelExpr(RelType type, const Symbol &s,
                         const uint8_t *loc) const {
  switch (type) {
  case R_ARC_32_PCREL:
    return R_PC;
  case R_ARC_32:
  default:
    return R_ABS;
  }
}

void ARC4::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  switch (rel.type) {
  case R_ARC_32:
    write32le(loc, val);
    break;
  case R_ARC_32_PCREL:
    write32le(loc, val);
    break;
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unrecognized relocation " << rel.type;
  }
}

void elf::setARC4TargetInfo(Ctx &ctx) { ctx.target.reset(new ARC4(ctx)); }
