//===-- ARC4MCAsmInfo.cpp - ARC4 Asm Info ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4MCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

ARC4MCAsmInfo::ARC4MCAsmInfo(const Triple &TT) {
  CommentString            = ";";
  SupportsDebugInformation = true;
  ExceptionsType           = ExceptionHandling::DwarfCFI;
  UseIntegratedAssembler   = true;
  // Alignment is expressed as power-of-two
  AlignmentIsInBytes       = false;
}
