//===- ARC4.cpp -----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 ABI is identical to ARC: 8 register args (r0-r7), same conventions.
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"

using namespace clang;
using namespace clang::CodeGen;

namespace {

class ARC4TargetCodeGenInfo : public TargetCodeGenInfo {
public:
  ARC4TargetCodeGenInfo(CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<DefaultABIInfo>(CGT)) {}
};

} // namespace

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createARC4TargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<ARC4TargetCodeGenInfo>(CGM.getTypes());
}
