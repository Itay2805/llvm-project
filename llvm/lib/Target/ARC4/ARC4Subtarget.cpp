//===- ARC4Subtarget.cpp - ARC4 Subtarget Information -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4Subtarget.h"
#include "ARC4.h"
#include "ARC4SelectionDAGInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "arc4-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "ARC4GenSubtargetInfo.inc"

void ARC4Subtarget::anchor() {}

ARC4Subtarget::ARC4Subtarget(const Triple &TT, const std::string &CPU,
                             const std::string &FS, const TargetMachine &TM)
    : ARC4GenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS), InstrInfo(*this),
      FrameLowering(*this), TLInfo(TM, *this) {
  TSInfo = std::make_unique<ARC4SelectionDAGInfo>();
}

ARC4Subtarget::~ARC4Subtarget() = default;
