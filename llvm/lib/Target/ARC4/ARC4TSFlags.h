//===-- ARC4TSFlags.h - ARC4 TSFlags bit definitions ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Bit positions for the ARC4 TSFlags field.  These must stay in sync with
// the TSFlags{N} assignments in ARC4InstrFormats.td.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4TSFLAGS_H
#define LLVM_LIB_TARGET_ARC4_ARC4TSFLAGS_H

#include <cstdint>

namespace llvm {
namespace ARC4TSF {
enum : uint64_t {
  HasLimm   = 1 << 0, // 32-bit LIMM extra word follows instruction
  IsShimm   = 1 << 1, // shimm sentinel (61/63) in B or C field
  HasFQ     = 1 << 2, // has .f and .q suffix operands (non-shimm ALU/SOP)
  HasFOnly  = 1 << 3, // has .f suffix only, no .q (shimm ALU/SOP)
  HasQN     = 1 << 4, // has .q and .n suffix operands (branch)
  HasFQN    = 1 << 5, // has .f, .q, .n suffix operands (jump)
  HasLdMod3 = 1 << 6, // has 3 load modifiers (x, w/W, e/E)
  HasLdMod2 = 1 << 7, // has 2 load modifiers (x/X, e/E, no writeback)
  HasStMod2 = 1 << 8, // has 2 store modifiers (v, D)
  HasStMod1 = 1 << 9, // has 1 store modifier (D only)
};
} // namespace ARC4TSF
} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4TSFLAGS_H
