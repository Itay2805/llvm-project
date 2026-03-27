//===--- ARC4.h - Declare ARC4 target feature support -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_ARC4_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_ARC4_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY ARC4TargetInfo : public TargetInfo {
public:
  ARC4TargetInfo(const llvm::Triple &Triple, const TargetOptions &)
      : TargetInfo(Triple) {
    NoAsmVariants = true;
    LongLongAlign = 32;
    SuitableAlign = 32;
    DoubleAlign = LongDoubleAlign = 32;
    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    UseZeroLengthBitfieldAlignment = true;
    resetDataLayout();
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override {
    return {};
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  std::string_view getClobbers() const override { return ""; }

  ArrayRef<const char *> getGCCRegNames() const override {
    static const char *const GCCRegNames[] = {
        "r0",  "r1",  "r2",  "r3",  "r4",  "r5",     "r6",  "r7",
        "r8",  "r9",  "r10", "r11", "r12", "r13",    "r14", "r15",
        "r16", "r17", "r18", "r19", "r20", "r21",    "r22", "r23",
        "r24", "r25", "r26", "r27", "r28", "ilink1", "ilink2", "blink"};
    return llvm::ArrayRef(GCCRegNames);
  }

  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override {
    return {};
  }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    return false;
  }

  bool hasBitIntType() const override { return true; }

  bool isCLZForZeroUndef() const override { return false; }
};

} // namespace targets
} // namespace clang

#endif // LLVM_CLANG_LIB_BASIC_TARGETS_ARC4_H
