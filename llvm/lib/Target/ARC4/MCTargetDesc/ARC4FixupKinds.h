//===-- ARC4FixupKinds.h - ARC4 Fixup Entries -----------------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4FIXUPKINDS_H
#define LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace ARC4 {

enum Fixups {
  // 26-bit absolute branch target (R_ARC_B26), right-shift 2, bits [23:0]
  fixup_arc4_b26 = FirstTargetFixupKind,
  // 22-bit PC-relative branch (R_ARC_B22_PCREL), right-shift 2, bits [28:7]
  fixup_arc4_b22_pcrel,

  fixup_arc4_invalid,
  NumTargetFixupKinds = fixup_arc4_invalid - FirstTargetFixupKind
};

} // namespace ARC4
} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4FIXUPKINDS_H
