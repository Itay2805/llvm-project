//===- ARC4.cpp -----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 is a 32-bit embedded RISC processor (Argonaut RISC Core, generation 4).
// It uses the original ARC ELF machine type (EM_ARC = 45) and a compact
// little-endian instruction encoding.
//
// Relocation types are taken from binutils-2.15/include/elf/arc.h.
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

// ARC4 relocation types (from binutils-2.15/include/elf/arc.h).
enum {
  R_ARC_NONE     = 0,
  R_ARC_32       = 4, // 32-bit absolute
  R_ARC_B26      = 5, // 26-bit absolute branch: val>>2 stored in bits [23:0]
  R_ARC_B22_PCREL = 6, // 22-bit PC-relative branch: val>>2 stored in bits [28:7]
};

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
  // Trap instruction: NOP encoding 0x7fffffff in little-endian.
  trapInstr = {0xff, 0xff, 0xff, 0x7f};
  // Embedded target — no virtual memory, image starts at address 0.
  defaultImageBase = 0;
}

RelExpr ARC4::getRelExpr(RelType type, const Symbol &s,
                         const uint8_t *loc) const {
  switch (type) {
  case R_ARC_NONE:
    return R_NONE;
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
  case R_ARC_NONE:
    break;

  case R_ARC_32:
    checkIntUInt(ctx, loc, val, 32, rel);
    write32le(loc, val);
    break;

  case R_ARC_B26: {
    // 26-bit absolute branch.  The branch offset is val>>2, stored in
    // instruction bits [23:0].
    int64_t sval = static_cast<int64_t>(val);
    checkAlignment(ctx, loc, val, 4, rel);
    int64_t offset = sval >> 2;
    checkInt(ctx, loc, offset, 24, rel);
    uint32_t insn = read32le(loc);
    insn = (insn & ~0x00FFFFFFU) | (static_cast<uint32_t>(offset) & 0x00FFFFFFU);
    write32le(loc, insn);
    break;
  }

  case R_ARC_B22_PCREL: {
    // 22-bit PC-relative branch.  The branch offset is val>>2, stored in
    // instruction bits [28:7].
    int64_t sval = static_cast<int64_t>(val);
    checkAlignment(ctx, loc, val, 4, rel);
    int64_t offset = sval >> 2;
    checkInt(ctx, loc, offset, 22, rel);
    uint32_t insn = read32le(loc);
    insn = (insn & ~0x1FFFFF80U) |
           ((static_cast<uint32_t>(offset) & 0x003FFFFFU) << 7);
    write32le(loc, insn);
    break;
  }

  default:
    Err(ctx) << getErrorLoc(ctx, loc) << "unrecognized relocation " << rel.type;
  }
}

void elf::setARC4TargetInfo(Ctx &ctx) { ctx.target.reset(new ARC4(ctx)); }
