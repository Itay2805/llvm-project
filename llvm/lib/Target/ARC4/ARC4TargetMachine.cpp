//===- ARC4TargetMachine.cpp - ARC4 Target Machine Implementation ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Stub target machine for ARCtangent-A4 (ARC4).
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeARC4Target() {}
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeARC4TargetMC() {}
