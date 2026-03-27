//===--- ARC4.cpp - Implement ARC4 target feature support -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4.h"
#include "clang/Basic/MacroBuilder.h"

using namespace clang;
using namespace clang::targets;

void ARC4TargetInfo::getTargetDefines(const LangOptions &Opts,
                                      MacroBuilder &Builder) const {
  Builder.defineMacro("__arc4__");
}
