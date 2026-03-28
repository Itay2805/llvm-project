//===--- ARC4.cpp - ARC4 ToolChain Implementations ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4.h"
#include "clang/Options/Options.h"
#include "llvm/Option/ArgList.h"

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;

void ARC4ToolChain::addClangTargetOptions(
    const ArgList &DriverArgs, ArgStringList &CC1Args,
    Action::OffloadKind DeviceOffloadKind) const {
  // ARC4 is a bare-metal embedded target: disable RTTI by default unless the
  // user explicitly requests it. C++ exception defaults are controlled via
  // addExceptionArgs in Clang.cpp (arc4 is excluded from CXXExceptionsEnabled).
  if (!DriverArgs.hasFlag(options::OPT_frtti, options::OPT_fno_rtti, false))
    CC1Args.push_back("-fno-rtti");
}
