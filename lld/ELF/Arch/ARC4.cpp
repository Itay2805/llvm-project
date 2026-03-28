//===- ARC4.cpp -----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 (ARCtangent-A4) is a 32-bit RISC processor.  This provides linker
// support for static linking of ARC4 ELF objects.
//
// Supported relocations:
//   R_ARC_32          - absolute 32-bit data
//   R_ARC_32_PCREL    - PC-relative 32-bit data
//   R_ARC_B26         - absolute 26-bit j/jl limm target (value>>2 in bits[23:0])
//   R_ARC_B22_PCREL   - 22-bit PC-relative branch (value>>2 in bits[26:7])
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
  case R_ARC_B22_PCREL:
    return R_PC;
  case R_ARC_32:
  case R_ARC_B26:
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
  case R_ARC_B26: {
    // j/jl limm: absolute 26-bit target.  The lowest 2 bits are always
    // zero (word-aligned) and not stored.  The upper 24 bits of (value>>2)
    // are installed into bits [23:0] of the limm word; bits [31:24] of the
    // existing limm (jump flags / reserved) are preserved.
    uint32_t word = read32le(loc);
    word = (word & 0xFF000000) | (((uint32_t)(val >> 2)) & 0x00FFFFFF);
    write32le(loc, word);
    break;
  }
  case R_ARC_B22_PCREL: {
    // b/bl/lp: 22-bit PC-relative branch.  The lowest 2 bits are not
    // stored.  The upper 20 bits of (value>>2) are installed into
    // instruction bits [26:7]; bits [31:27] (opcode) and [6:0] (NN/Q)
    // are preserved.
    uint32_t word = read32le(loc);
    uint32_t enc = ((uint32_t)((int64_t)val >> 2)) & 0xFFFFF;
    word = (word & 0xF800007F) | (enc << 7);
    write32le(loc, word);
    break;
  }
  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unrecognized relocation " << rel.type;
  }
}

void elf::setARC4TargetInfo(Ctx &ctx) { ctx.target.reset(new ARC4(ctx)); }
