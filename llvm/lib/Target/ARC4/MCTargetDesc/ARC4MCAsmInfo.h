//===-- ARC4MCAsmInfo.h - ARC4 asm properties -----------------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCASMINFO_H
#define LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class ARC4MCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit ARC4MCAsmInfo(const Triple &TheTriple,
                         const MCTargetOptions &Options);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4MCASMINFO_H
