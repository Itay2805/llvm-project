//===- ARC4.cpp -----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// ARC4 ABI:
//   - Arguments: first 8 words in r0-r7, remainder on the stack.
//   - Scalars and structs <= 4 bytes returned in r0.
//   - 64-bit integers returned in r0:r1.
//   - Larger structs returned via hidden sret pointer in r0.
//   - r0-r12 caller-saved, r13-r25 callee-saved.
//   - r26 = gp, r27 = fp, r28 = sp.
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"

using namespace clang;
using namespace clang::CodeGen;

namespace {

class ARC4ABIInfo : public DefaultABIInfo {
public:
  ARC4ABIInfo(CodeGenTypes &CGT) : DefaultABIInfo(CGT) {}

  ABIArgInfo classifyReturnType(QualType Ty) const;

  void computeInfo(CGFunctionInfo &FI) const override {
    if (!getCXXABI().classifyReturnType(FI))
      FI.getReturnInfo() = classifyReturnType(FI.getReturnType());
    for (auto &I : FI.arguments())
      I.info = classifyArgumentType(I.type);
  }
};

ABIArgInfo ARC4ABIInfo::classifyReturnType(QualType Ty) const {
  if (Ty->isVoidType())
    return ABIArgInfo::getIgnore();

  // Structs that fit in a single 4-byte register are returned directly in r0.
  if (isAggregateTypeForABI(Ty)) {
    uint64_t Size = getContext().getTypeSize(Ty);
    if (Size <= 32)
      return ABIArgInfo::getDirect(llvm::IntegerType::get(getVMContext(), 32));
    // Larger structs use a hidden sret pointer (passed as first arg in r0).
    return getNaturalAlignIndirect(Ty, /*AddrSpace=*/0);
  }

  // Promote small integers to i32 (i1/i8/i16 → i32).
  if (isPromotableIntegerTypeForABI(Ty))
    return ABIArgInfo::getExtend(Ty);

  return ABIArgInfo::getDirect();
}

class ARC4TargetCodeGenInfo : public TargetCodeGenInfo {
public:
  ARC4TargetCodeGenInfo(CodeGenTypes &CGT)
      : TargetCodeGenInfo(std::make_unique<ARC4ABIInfo>(CGT)) {}
};

} // namespace

std::unique_ptr<TargetCodeGenInfo>
CodeGen::createARC4TargetCodeGenInfo(CodeGenModule &CGM) {
  return std::make_unique<ARC4TargetCodeGenInfo>(CGM.getTypes());
}
