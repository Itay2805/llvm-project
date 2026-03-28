//===--- ARC4.cpp - Implement ARC4 target feature support -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements ARC4 TargetInfo objects.
//
//===----------------------------------------------------------------------===//

#include "ARC4.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

const char *const ARC4TargetInfo::GCCRegNames[] = {
    "r0",    "r1",    "r2",    "r3",    "r4",    "r5",    "r6",    "r7",
    "r8",    "r9",    "r10",   "r11",   "r12",   "r13",   "r14",   "r15",
    "r16",   "r17",   "r18",   "r19",   "r20",   "r21",   "r22",   "r23",
    "r24",   "r25",   "gp",    "fp",    "sp",    "ilink1","ilink2","blink"
};

ArrayRef<const char *> ARC4TargetInfo::getGCCRegNames() const {
  return llvm::ArrayRef(GCCRegNames);
}

const TargetInfo::GCCRegAlias ARC4TargetInfo::GCCRegAliases[] = {
    {{"r26"}, "gp"},
    {{"r27"}, "fp"},
    {{"r28"}, "sp"},
    {{"r29"}, "ilink1"},
    {{"r30"}, "ilink2"},
    {{"r31"}, "blink"},
};

ArrayRef<TargetInfo::GCCRegAlias> ARC4TargetInfo::getGCCRegAliases() const {
  return llvm::ArrayRef(GCCRegAliases);
}

void ARC4TargetInfo::getTargetDefines(const LangOptions &Opts,
                                      MacroBuilder &Builder) const {
  Builder.defineMacro("__arc4__");
  Builder.defineMacro("__ARC4__");
  Builder.defineMacro("__arc__");
}
