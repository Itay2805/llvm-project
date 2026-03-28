//===-- ARC4MCAsmInfo.cpp - ARC4 asm properties ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4MCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

void ARC4MCAsmInfo::anchor() {}

ARC4MCAsmInfo::ARC4MCAsmInfo(const Triple & /*TheTriple*/,
                             const MCTargetOptions &Options) {
  IsLittleEndian = true;
  PrivateGlobalPrefix = ".L";
  WeakRefDirective = "\t.weak\t";
  CommentString = ";";
  UsesELFSectionDirectiveForBSS = true;
  SupportsDebugInformation = true;
  MinInstAlignment = 4;
  ExceptionsType = ExceptionHandling::DwarfCFI;
}
