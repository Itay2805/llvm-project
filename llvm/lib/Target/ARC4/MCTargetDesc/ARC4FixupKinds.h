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
  fixup_arc4_b26 = FirstTargetFixupKind,
  fixup_arc4_b22_pcrel,
  fixup_arc4_invalid,
  NumTargetFixupKinds = fixup_arc4_invalid - FirstTargetFixupKind
};

} // namespace ARC4
} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4FIXUPKINDS_H
