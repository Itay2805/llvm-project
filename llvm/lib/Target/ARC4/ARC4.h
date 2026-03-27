//===-- ARC4.h - Top-level interface for ARC4 -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4_H
#define LLVM_LIB_TARGET_ARC4_ARC4_H

#include "llvm/Target/TargetMachine.h"

namespace llvm {

class ARC4TargetMachine;
class FunctionPass;

FunctionPass *createARC4ISelDag(ARC4TargetMachine &TM);

} // namespace llvm

#endif
