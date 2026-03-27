//===-- ARC4TargetInfo.cpp - ARC4 Target Implementation -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

Target &llvm::getTheARC4Target() {
  static Target TheARC4Target;
  return TheARC4Target;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeARC4TargetInfo() {
  RegisterTarget<Triple::arc4, /*HasJIT=*/false> X(
      getTheARC4Target(), "arc4", "ARCtangent-A4", "ARC4");
}
