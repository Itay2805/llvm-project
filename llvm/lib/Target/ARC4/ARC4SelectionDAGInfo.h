//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4SELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_ARC4_ARC4SELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

#define GET_SDNODE_ENUM
#include "ARC4GenSDNodeInfo.inc"

namespace llvm {

class ARC4SelectionDAGInfo : public SelectionDAGGenTargetInfo {
public:
  ARC4SelectionDAGInfo();
  ~ARC4SelectionDAGInfo() override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4SELECTIONDAGINFO_H
