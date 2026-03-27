//===-- ARC4Subtarget.cpp - ARC4 Subtarget Information ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4Subtarget.h"

using namespace llvm;

#define DEBUG_TYPE "arc4-subtarget"

#define GET_SUBTARGETINFO_CTOR
#define GET_SUBTARGETINFO_TARGET_DESC
#include "ARC4GenSubtargetInfo.inc"

ARC4Subtarget::ARC4Subtarget(const Triple &TT, StringRef CPU, StringRef FS,
                             const TargetMachine &TM)
    : ARC4GenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS),
      InstrInfo(*this), FrameLowering(), TLInfo(TM, *this), TSInfo() {}
